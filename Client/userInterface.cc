#include <utility>

#include "userInterface.h"
#include "../Misc/glo.h"

static const std::string LOOK_FOR_SERVERS = "Szukaj pośrednika\n\r";
static const std::string END_CONNECTION = "Koniec\n\r";
static const std::string STAR = " *";
static const std::string NL = "\n\r";
static const std::string DEFAULT_COLOR = {27, '[', '3', '7', 'm'};
static const std::string PICKED_COLOR = {27, '[', '3', '3', 'm'};
static const std::string CLEAR = {27, '[', 'H', 27, '[', '0', 'J'};


size_t hashSockaddr(sockaddr s) {
    return ((uint64_t)s.sa_family<<48) + ((uint64_t)s.sa_data[0]<<40) +
        ((uint64_t)s.sa_data[1]<<32) + ((uint64_t)s.sa_data[2]<<24) +
        ((uint64_t)s.sa_data[3]<<16) + ((uint64_t)s.sa_data[4]<<8) +
        ((uint64_t)s.sa_data[5]<<8);
}

bool operator==(const sockaddr& lhs, const sockaddr& rhs) {
    return hashSockaddr(lhs) == hashSockaddr(rhs);
}

bool operator!=(const sockaddr& lhs, const sockaddr& rhs) {
    return hashSockaddr(lhs) != hashSockaddr(rhs);
}

UserInterface::UserInterface() {
    cursorPosition = 0;
    optionsNum = 2;
    picked = false;
}

void UserInterface::addServer(const std::shared_ptr<Server>& server) {
    if (!checkIfServerAlreadyExists(server->addr)) {
        if (cursorPosition == optionsNum - 1) cursorPosition++;
        servers.push_back(server);
        optionsNum++;
    } else changeServerName(server);
}

void UserInterface::deletePickedServer() {
    picked = false;
    radioMetadata = EMPTY;

    for (size_t i = 0; i < servers.size(); i++) {
        if (servers[i]->addr == pickedRadioAddr) {
            if (cursorPosition - 1 >= i) cursorPosition--;

            servers.erase(servers.begin() + i);
            optionsNum--;
            break;
        }
    }
}

bool UserInterface::checkIfServerAlreadyExists(const sockaddr& addr) {
    for (const std::shared_ptr<Server>& server: servers) {
        if (server->addr == addr) return true;
    }
    return false;
}

void UserInterface::changeServerName(const std::shared_ptr<Server>& server) {
    for (const std::shared_ptr<Server>& s: servers) {
        if (s->addr == server->addr) {
            s->radioName = server->radioName;
            break;
        }
    }
}

bool UserInterface::sendInterface() {
    if (!telnet->send(CLEAR + DEFAULT_COLOR)) return false;

    if (cursorPosition == 0) {
        if (!telnet->send(PICKED_COLOR)) return false;
    }
    if (!telnet->send(LOOK_FOR_SERVERS)) return false;
    if (cursorPosition == 0) {
        if (!telnet->send(DEFAULT_COLOR)) return false;
    }

    for (size_t i = 1; i <= servers.size(); i++) {
        std::shared_ptr<Server> server = servers[i - 1];

        if (cursorPosition == i) {
            if (!telnet->send(PICKED_COLOR)) return false;
        }
        if (!telnet->send(server->radioName)) return false;
        if (picked && pickedRadioAddr == server->addr) {
            if (!telnet->send(STAR)) return false;
        }

        if (cursorPosition == i) {
            if (!telnet->send(DEFAULT_COLOR)) return false;
        }
        if (!telnet->send(NL)) return false;
    }

    if (cursorPosition == servers.size() + 1) {
        if (!telnet->send(PICKED_COLOR)) return false;
    }
    if (!telnet->send(END_CONNECTION)) return false;
    if (cursorPosition == servers.size() + 1) {
        if (!telnet->send(DEFAULT_COLOR)) return false;
    }

    if (radioMetadata != EMPTY) {
        if (!telnet->send(radioMetadata + NL)) return false;
    }

    return true;
}

userAction UserInterface::getInput() {
    telnetAction action = telnet->readInput();

    if (action == ERROR) return TELNET_ERROR;
    if (action == TELNET_OUT_OF_INPUT) return OUT_OF_INPUT;

    if (action == UP) {
        if (cursorPosition == 0) cursorPosition = optionsNum - 1;
        else cursorPosition--;
        return DEFAULT;
    } else if (action == DOWN) {
        if (cursorPosition == optionsNum - 1) cursorPosition = 0;
        else cursorPosition++;
        return DEFAULT;
    } else if (action == SELECT) {
        if (cursorPosition == 0) {
            clearUnusedServers();
            return SEARCH;
        } else if (cursorPosition == optionsNum - 1) {
            return END;
        } else {
            sockaddr newPickedAddr = servers[cursorPosition - 1]->addr;

            if (!picked || pickedRadioAddr != newPickedAddr) {
                picked = true;
                pickedRadioAddr = newPickedAddr;
                return PICK_RADIO;
            }
        }
    }

    return VOID;
}

sockaddr UserInterface::getPickedRadioAddr() {
    return pickedRadioAddr;
}

void UserInterface::setTelnet(std::shared_ptr<TelnetManager> telnet) {
    this->telnet = telnet;
}

void UserInterface::setMetadata(std::string metadata) {
    static std::regex regexMetadata("^StreamTitle='(.*?)'.*$",
        std::regex_constants::ECMAScript | std::regex_constants::icase);
    std::smatch match;

    if (regex_match(metadata, match, regexMetadata)) {
        this->radioMetadata = match[1];
    } else this->radioMetadata = std::move(metadata);
}

void UserInterface::clearUnusedServers() {
    std::shared_ptr<Server> pickedServer;
    optionsNum = 2;

    if (picked) {
        for (const std::shared_ptr<Server>& server: servers) {
            if (server->addr == pickedRadioAddr)
                pickedServer = server;
        }
        optionsNum++;
    }

    servers.clear();
    if (picked) servers.push_back(pickedServer);
}

bool UserInterface::getPicked() {
    return picked;
}



