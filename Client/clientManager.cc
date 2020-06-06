#include <sys/time.h>
#include <cerrno>

#include "clientManager.h"

static const int KEEPALIVE_TIMEOUT = 3500;
static const int TTL_VALUE = 64;

ClientManager::ClientManager(const std::shared_ptr<ParserClient>& parser) {
    this->radioTimeout = stoi(parser->getValueOfFlag(TIMEOUT_FLAG_B)) * 1000;
    this->keepaliveTimeout = KEEPALIVE_TIMEOUT;
    this->address = parser->getValueOfFlag(HOST_FLAG_C);
    this->port = stoi(parser->getValueOfFlag(PORT_FLAG_C_UDP));
    this->radioDiscover = std::make_shared<MessageClient>(DISCOVER_T, 0);
    this->keepAliveMess = std::make_shared<MessageClient>(KEEPALIVE_T, 0);
    this->userInterface = std::make_shared<UserInterface>();

    fds[0].fd = -1;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    fds[1].fd = -1;
    fds[1].events = POLLIN;
    fds[1].revents = 0;

    makeServerSocket();
    makeTelnetSocket(parser);

    fds[2].fd = telnetSock;
    fds[2].events = POLLIN;
    fds[2].revents = 0;
}

void ClientManager::makeServerSocket() {
    struct addrinfo addr_hints;
    struct addrinfo *addr_result;

    serverSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSock < 0) syserr("socket");

    addr_hints.ai_family = AF_INET;
    addr_hints.ai_socktype = SOCK_DGRAM;
    addr_hints.ai_protocol = IPPROTO_UDP;
    addr_hints.ai_flags = 0;
    addr_hints.ai_addrlen = 0;
    addr_hints.ai_addr = NULL;
    addr_hints.ai_canonname = NULL;
    addr_hints.ai_next = NULL;

    if (getaddrinfo(address.c_str(), NULL, &addr_hints, &addr_result) != 0) {
        close(serverSock);
        syserr("getaddrinfo");
    }

    struct sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_addr.s_addr = ((struct sockaddr_in *) (addr_result->ai_addr))->sin_addr.s_addr;
    addr_in.sin_port = htons((uint16_t) port);
    freeaddrinfo(addr_result);

    addr = *((struct sockaddr *) &addr_in);

    if (fcntl(serverSock, F_SETFL, O_NONBLOCK) < 0) {
        close(serverSock);
        syserr("fcntl");
    }

    int optval = 1;
    if (setsockopt(serverSock, SOL_SOCKET, SO_BROADCAST, (void *) &optval, sizeof optval) < 0) {
        close(serverSock);
        syserr("setsockopt broadcast");
    }

    optval = TTL_VALUE;
    if (setsockopt(serverSock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&optval, sizeof optval) < 0) {
        close(serverSock);
        syserr("setsockopt multicast");
    }
}

void ClientManager::makeTelnetSocket(std::shared_ptr<ParserClient> parser) {
    telnetSock = socket(PF_INET, SOCK_STREAM, 0);
    if (telnetSock < 0) syserr("socket");

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(stoi(parser->getValueOfFlag(PORT_FLAG_C_TCP)));

    if (bind(telnetSock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        closeSockets();
        syserr("bind");
    }

    if (listen(telnetSock, 1) < 0) {
        closeSockets();
        syserr("listen");
    }
}

void ClientManager::handleTelnetError() {
    telnet->closeSocket();

    fds[1].fd = -1;
    fds[2].fd = telnetSock;
}

void ClientManager::makeTelnetSetup() {
    telnet = std::make_shared<TelnetManager>();
    if (!telnet->makeSocket(telnetSock)) {
        closeSockets();
        syserr("telnet socket");
    }

    userInterface->setTelnet(telnet);
    if (!userInterface->sendInterface()) {
        closeSockets();
        syserr("telnet socket");
    }

    fds[1].fd = telnet->getSocket();
    fds[2].fd = -1;
}

void ClientManager::handleConnections(int *finish) {
    userAction userAction;
    int pollWaitTime, ret, updateInterface;
    double startPollTime;

    while (!(*finish)) {
        pollWaitTime = userInterface->getPicked() ?
            std::min(radioCurrTimeout, keepaliveTimeout) : -1;
        startPollTime = getCurrTime();
        userAction = VOID;
        updateInterface = false;

        ret = poll(fds, 3, pollWaitTime);

        if (ret == -1) {
            if (ret < 0 && errno == EINTR) break;

            closeSockets();
            syserr("poll error");
        }

        if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            closeSockets();
            syserr("server socket");
        }
        if (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            handleTelnetError();
            continue;
        }
        if (fds[2].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            closeSockets();
            syserr("telnet socket");
        }

        if (ret == 0) {
            if (pollWaitTime == radioCurrTimeout) {
                userInterface->deletePickedServer();
                updateInterface = true;
                updateTimeouts(startPollTime);
                userAction = DEFAULT;
            } else {
                updateKeepalive(startPollTime);
                continue;
            }
        } else {
            if (userInterface->getPicked()) updateTimeouts(startPollTime);

            if (fds[0].revents & POLLIN) {
                fds[0].revents = 0;

                updateInterface = getDataFromServer();
            }
            if (fds[1].revents & POLLIN) {
                fds[1].revents = 0;

                userAction = handleTelnetInput();
                if (userAction == END) break;
                else if (userAction == TELNET_ERROR) continue;
            }
            if (fds[2].revents & POLLIN) {
                fds[2].revents = 0;

                makeTelnetSetup();
            }
        }

        if (userAction != VOID || updateInterface) {
            if (!userInterface->sendInterface()) handleTelnetError();
        }
    }

    closeSockets();
}

void ClientManager::updateKeepalive(double lastTime) {
    keepAliveMess->sendMessage(serverSock,
        userInterface->getPickedRadioAddr());
    updateTimeouts(lastTime);
    keepaliveTimeout = KEEPALIVE_TIMEOUT;
}

void ClientManager::updateTimeouts(double lastTime) {
    double timeDifference = getCurrTime() - lastTime;
    radioCurrTimeout = std::max(0.0, radioCurrTimeout - timeDifference);
    keepaliveTimeout = std::max(0.0, keepaliveTimeout - timeDifference);
}

userAction ClientManager::handleTelnetInput() {
    userAction action = userInterface->getInput();

    switch (action) {
        case TELNET_ERROR:
            handleTelnetError();
            break;
        case SEARCH:
            lookForServers();
            break;
        case PICK_RADIO:
            pickRadio();
            break;
        default:
            break;
    }

    return action;
}

void ClientManager::pickRadio() {
    radioDiscover->sendMessage(serverSock, userInterface->getPickedRadioAddr());
    userInterface->setMetadata(EMPTY);
    this->radioCurrTimeout = radioTimeout;
    this->keepaliveTimeout = KEEPALIVE_TIMEOUT;
}

bool ClientManager::getDataFromServer() {
    std::shared_ptr<MessageClient> mess = std::make_shared<MessageClient>(DEFAULT_BUFF_SIZE);
    enum mess_status status = mess->getMessage(serverSock);

    if (status == OK) {
        switch (mess->getType()) {
            case IAM_T:
                addNewServer(mess);
                return true;
            case AUDIO_T:
                validateAndWriteOutput(mess);
                break;
            case METADATA_T:
                return validateAndSetMetadata(mess);
        }
    }

    return false;
}

void ClientManager::addNewServer(const std::shared_ptr<MessageClient>& mess) {
    std::shared_ptr<Server> server = std::make_shared<Server>();
    server->addr = mess->getSrcAddr();

    char* buffer = reinterpret_cast<char*>((mess->getBuff()).get()) + 4;
    server->radioName = std::string(buffer, mess->getLength());

    userInterface->addServer(server);
}

void ClientManager::validateAndWriteOutput(const std::shared_ptr<MessageClient>& mess) {
    if (userInterface->getPicked() &&
        mess->getSrcAddr() == userInterface->getPickedRadioAddr()) {

        uint8_t * buffer = reinterpret_cast<uint8_t*>((mess->getBuff()).get()) + 4;
        if (write(STDOUT_FILENO, buffer, mess->getLength()) < 0 && errno != EINTR) {
            closeSockets();
            syserr("write on stdout");
        }

        radioCurrTimeout = radioTimeout;
    }
}

bool ClientManager::validateAndSetMetadata(const std::shared_ptr<MessageClient>& mess) {
    if (userInterface->getPicked() &&
        mess->getSrcAddr() == userInterface->getPickedRadioAddr()) {

        char* buffer = reinterpret_cast<char*>((mess->getBuff()).get()) + 4;
        userInterface->setMetadata(std::string(buffer, mess->getLength()));
        radioCurrTimeout = radioTimeout;

        return true;
    }
    return false;
}

void ClientManager::closeSockets() {
    if (fds[1].fd != -1) telnet->closeSocket();
    close(telnetSock);
    close(serverSock);
}

void ClientManager::lookForServers() {
    radioDiscover->sendMessage(serverSock, addr);
    fds[0].fd = serverSock;
}

int ClientManager::getCurrTime() {
    struct timeval time;
    gettimeofday(&time, NULL);

    return (int) (time.tv_sec + (double)time.tv_usec * .000001) * 1000;
}








