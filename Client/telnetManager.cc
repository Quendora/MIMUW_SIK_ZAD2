#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include "telnetManager.h"

bool TelnetManager::makeSocket(int sock) {
    struct sockaddr_in client_address;
    socklen_t client_address_len;
    client_address_len = sizeof(client_address);

    this->telnetSock = accept(sock, (struct sockaddr *) &client_address, &client_address_len);
    if (telnetSock < 0) return false;

    if (fcntl(telnetSock, F_SETFL, O_NONBLOCK) < 0) return false;

    return initTelnetConfig();
}

bool TelnetManager::initTelnetConfig() {
    unsigned char doLinemode[] = {255, 253, 34};
    if (write(telnetSock, doLinemode, sizeof(doLinemode)) != sizeof(doLinemode)) return false;

    unsigned char linemodeOptions[] = {255, 250, 34, 1, 0, 255, 240};
    if (write(telnetSock, linemodeOptions, sizeof(linemodeOptions)) != sizeof(linemodeOptions)) return false;;

    unsigned char willEcho[] = {255, 251, 1};
    if (write(telnetSock, willEcho, sizeof(willEcho)) != sizeof(willEcho)) return false;

    unsigned char noCursor[] = {27, '[', '?', '2', '5', 'l', 27, '[', 's'};
    if (write(telnetSock, noCursor, sizeof(noCursor)) != sizeof(noCursor)) return false;

    return true;
}

bool TelnetManager::send(const std::string& mess) {
    return write(telnetSock, mess.c_str(), mess.length()) == (int) mess.length();
}

telnetAction TelnetManager::readInput() {
    uint8_t c;
    ssize_t readLen;

    if ((readLen = read(telnetSock, &c, 1)) <= 0) return resolveError();
    else if (c == 27) {
        if ((readLen = read(telnetSock, &c, 1)) <= 0) return resolveError();
        else if (c == 91) {
            if ((readLen = read(telnetSock, &c, 1)) <= 0) return resolveError();
            else {
                if (c == 65) return UP;
                if (c == 66) return DOWN;
            }
        }
    } else if (c == 13) {
        if ((readLen = read(telnetSock, &c, 1)) <= 0) return resolveError();
        else if (c == 0) return SELECT;
    }

    return OTHER;
}

telnetAction TelnetManager::resolveError() {
    if (errno == EWOULDBLOCK || errno == EAGAIN) return TELNET_OUT_OF_INPUT;
    else return ERROR;
}

void TelnetManager::closeSocket() {
    close(telnetSock);
}

int TelnetManager::getSocket() {
    return telnetSock;
}




