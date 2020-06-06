#include <sys/time.h>

#include "serverManager.h"

static const uint16_t MAX_SEND_MESS_SIZE = 4080;

size_t hashSockaddr(sockaddr s) {
    return ((uint64_t)s.sa_family<<48) + ((uint64_t)s.sa_data[0]<<40) +
        ((uint64_t)s.sa_data[1]<<32) + ((uint64_t)s.sa_data[2]<<24) +
        ((uint64_t)s.sa_data[3]<<16) + ((uint64_t)s.sa_data[4]<<8) +
        ((uint64_t)s.sa_data[5]<<8);
}

bool operator==(const sockaddr& lhs, const sockaddr& rhs) {
    return hashSockaddr(lhs) == hashSockaddr(rhs);
}

ServerManager::ServerManager(const std::shared_ptr<ParserServer>& parser,
    const std::shared_ptr<ShoutcastManager>& shoutcast) {
    this->metaInt = shoutcast->GetMetaInt();
    this->clientTimeout = stoi(parser->getValueOfFlag(TIMEOUT_FLAG_B)) * 1000;
    this->radioTimeout = stoi(parser->getValueOfFlag(TIMEOUT_FLAG_A)) * 1000;
    this->radioCurrTimeout = radioTimeout;
    this->shoutcast = shoutcast;
    this->multi = parser->checkIfFlagExists(MULTI_FLAG);
    this->radioMetadata = new MessageServer(METADATA_T, MAX_METADATA_LENGTH);
    this->radioSendAudio = true;
    this->audioBytesLeft = metaInt > 0 ? metaInt : MAX_SEND_MESS_SIZE;
    this->metadataBytesLeft = 0;
    this->radioAudio = new MessageServer(AUDIO_T, audioBytesLeft);
    this->alreadyReadMetadataByte = false;

    setRadioName(shoutcast->GetRadioName(),
        std::min(shoutcast->GetRadioName().length(), (unsigned long) MAX_SEND_MESS_SIZE));

    if (multi) makeSocket(parser->getValueOfFlag(MULTI_FLAG), parser->getValueOfFlag(PORT_FLAG_B));
    else makeSocket(parser->getValueOfFlag(PORT_FLAG_B));

    if (fcntl(clientSock, F_SETFL, O_NONBLOCK) < 0) {
        handleError();
        syserr("fcntl");
    }

    fds[0].fd = shoutcast->GetSock();
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = clientSock;
    fds[1].events = POLLIN;
    fds[1].revents = 0;
}

void ServerManager::setRadioName(std::string name, unsigned long length) {
    this->radioIAM = new MessageServer(IAM_T, length);
    char *buffer = radioIAM->getBuff().get() + 4;

    for (unsigned long i = 0; i < length; i++)
        buffer[i] = name[i];
}

void ServerManager::dropMembership() {
    if (setsockopt(clientSock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *) &ip_mreq, sizeof ip_mreq) < 0) {
        close(clientSock);
        syserr("drop membership");
    }

    close(clientSock);
}

void ServerManager::makeSocket(const std::string& address, const std::string& port) {
    in_port_t local_port = (in_port_t) stol(port);
    struct sockaddr_in local_address;

    clientSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSock < 0) syserr("socket");

    ip_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (inet_aton(address.c_str(), &ip_mreq.imr_multiaddr) == 0) {
        close(clientSock);
        syserr("ERROR: inet_aton - invalid multicast address\n");
    }

    if (setsockopt(clientSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *) &ip_mreq, sizeof ip_mreq) < 0) {
        close(clientSock);
        syserr("setsockopt");
    }

    local_address.sin_family = AF_INET;
    local_address.sin_addr.s_addr = htonl(INADDR_ANY);
    local_address.sin_port = htons(local_port);

    if (bind(clientSock, (struct sockaddr *) &local_address, sizeof local_address) < 0) {
        handleError();
        syserr("bind");
    }
}

void ServerManager::makeSocket(const std::string& port) {
    in_port_t local_port = (in_port_t) stol(port);
    struct sockaddr_in local_address;

    clientSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSock < 0) {
        shoutcast->closeSocket();
        syserr("socket");
    }

    local_address.sin_family = AF_INET;
    local_address.sin_addr.s_addr = htonl(INADDR_ANY);
    local_address.sin_port = htons(local_port);

    if (bind(clientSock, (struct sockaddr *) &local_address, sizeof local_address) < 0) {
        handleError();
        syserr("bind");
    }
}

void ServerManager::handleClients(int *finish) {
    int ret, startPollTime;

    while (!(*finish)) {
        radioState sendRadioMess = NO_CHANGE_NO_SEND;

        startPollTime = getCurrTime();
        ret = poll(fds, 2, radioCurrTimeout);
        if (ret <= 0) {
            handleError();
            if (ret < 0 && errno == EINTR) continue;
            if (ret < 0) syserr("poll error");
            else syserr("shoutcast timeout");
        }

        int timeDifference = getCurrTime() - startPollTime;
        radioCurrTimeout = std::max(0, radioCurrTimeout - timeDifference);

        if (fds[0].revents & POLLIN) {
            fds[0].revents = 0;

            sendRadioMess = handleRadio();
            radioCurrTimeout = radioTimeout;
        }
        if (fds[1].revents & POLLIN) {
            fds[1].revents = 0;

            handleGetMessages();
        }

        if (sendRadioMess == CHANGE_SEND || sendRadioMess == NO_CHANGE_SEND) handleSendMessages();
        if (sendRadioMess == CHANGE_SEND || sendRadioMess == CHANGE_NO_SEND) changeRadioState();
    }

    if (multi) dropMembership();
    else close(clientSock);
    shoutcast->closeSocket();
}

radioState ServerManager::handleRadio() {
    if (radioSendAudio) return getAudio();
    else return getMetadata();
}

void ServerManager::changeRadioState() {
    if (radioSendAudio) {
        if (metaInt > 0) radioSendAudio = false;
        else audioBytesLeft = MAX_SEND_MESS_SIZE;
    } else {
        radioSendAudio = true;
        audioBytesLeft = metaInt;
    }
}

radioState ServerManager::getAudio() {
    uint16_t bytesToSend = std::min(audioBytesLeft, MAX_SEND_MESS_SIZE);

    uint8_t* buffer = reinterpret_cast<uint8_t*>((radioAudio->getBuff()).get()) + 4;
    int bytesRead = shoutcast->loadData(bytesToSend, buffer);
    if (bytesRead == -1) {
        handleError();
        syserr("shoutcast socket");
    } else if (bytesRead == 0) return NO_CHANGE_NO_SEND;

    radioAudio->setMessLength(bytesRead);
    audioBytesLeft -= bytesRead;
    if (audioBytesLeft == 0) return CHANGE_SEND;
    else return NO_CHANGE_SEND;
}

radioState ServerManager::getMetadata() {
    int bytesRead;

    if (!alreadyReadMetadataByte) {
        uint8_t metaDataLengthBuffer = 0;
        bytesRead = shoutcast->loadData(1, &metaDataLengthBuffer);
        if (bytesRead == -1) {
            handleError();
            syserr("shoutcast read");
        } else if (bytesRead > 0) {
            metadataBytesLeft = metaDataLengthBuffer * METADATA_LENGTH_MULTIPLIER;
            if (metadataBytesLeft == 0) return CHANGE_NO_SEND;
            alreadyReadMetadataByte = true;
        } else return NO_CHANGE_NO_SEND;
    }

    if (alreadyReadMetadataByte && metadataBytesLeft > 0) {
        uint8_t* buffer = reinterpret_cast<uint8_t*>((radioMetadata->getBuff()).get()) + 4;
        bytesRead = shoutcast->loadData(metadataBytesLeft, buffer);
        if (bytesRead == -1) {
            handleError();
            syserr("shoutcast read");
        } else if (bytesRead == 0) return NO_CHANGE_NO_SEND;

        metadataBytesLeft -= bytesRead;
        radioMetadata->setMessLength(bytesRead);
        if (metadataBytesLeft == 0) {
            alreadyReadMetadataByte = false;
            return CHANGE_SEND;
        } else return NO_CHANGE_SEND;
    }

    return CHANGE_NO_SEND;
}

void ServerManager::handleGetMessages() {
    Message *mess = new MessageServer(0);
    enum mess_status status;

    while (true) {
        status = mess->getMessage(clientSock);

        if (status == NONE) break;
        else if (status == OK) {
            updateClient(mess);
        }
    }
}

void ServerManager::handleSendMessages() {
    std::vector<ClientSockaddr> clientsToErase;

    for (auto it: clients) {
        Client client = it.second;
        struct sockaddr addr = it.first;

        if (!checkTimeout(client, clientTimeout)) clientsToErase.push_back(addr);
        else {
            if (radioSendAudio) {
                radioAudio->sendMessage(clientSock, it.first);
            } else {
                radioMetadata->sendMessage(clientSock, it.first);
            }
        }
    }

    for (ClientSockaddr sockaddr: clientsToErase)
        clients.erase(sockaddr);
}

void ServerManager::updateClient(Message *mess) {
    ClientSockaddr clientSockaddr = mess->getSrcAddr();

    if (mess->getType() == DISCOVER_T) {
        clients[clientSockaddr] = getCurrTime();
        radioIAM->sendMessage(clientSock, clientSockaddr);
    } else if (mess->getType() == KEEPALIVE_T && clients.find(clientSockaddr) != clients.end()) {
        clients[clientSockaddr] = getCurrTime();
    }
}

bool ServerManager::checkTimeout(int lastTime, int timeout) {
    return getCurrTime() - lastTime < timeout;
}

int ServerManager::getCurrTime() {
    struct timeval time;
    gettimeofday(&time, NULL);

    return (int) (time.tv_sec + time.tv_usec * .000001) * 1000;
}

void ServerManager::handleError() {
    if (multi) dropMembership();
    else close(clientSock);
    shoutcast->closeSocket();
}



