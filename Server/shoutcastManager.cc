#include "shoutcastManager.h"

void ShoutcastManager::makeConnection(const std::shared_ptr<ParserServer>& parser) {
    int rc;
    struct addrinfo addr_hints, *addr_result;

    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_flags = 0;
    addr_hints.ai_family = AF_INET;
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_protocol = IPPROTO_TCP;

    rc = getaddrinfo(parser->getValueOfFlag(HOST_FLAG).c_str(), parser->getValueOfFlag(PORT_FLAG_A).c_str(),
        &addr_hints, &addr_result);
    if (rc != 0) syserr("getaddrinfo: %s", gai_strerror(rc));

    sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(addr_result);
        syserr("socket");
    }

    if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) != 0) {
        closeSocket();
        freeaddrinfo(addr_result);
        syserr("shoutcast connect");
    }

    freeaddrinfo(addr_result);

    setSockTimeout(SO_RCVTIMEO, stoi(parser->getValueOfFlag(TIMEOUT_FLAG_A)));

    sendRequest(parser->GetRequest());
    bool giveMetadata = parser->checkIfFlagExists(GIVE_METADATA_FLAG)
        && parser->getValueOfFlag(GIVE_METADATA_FLAG) == YES;

    getResponse(giveMetadata);

    if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
        closeSocket();
        syserr("fcntl");
    }
}

void ShoutcastManager::closeSocket() {
    close(sock);
}

void ShoutcastManager::setSockTimeout(int flag, int time) {
    struct timeval timeout;
    timeout.tv_sec = time;
    timeout.tv_usec = 0;

    if (setsockopt(sock, SOL_SOCKET, flag, (void *) &timeout, sizeof(timeout)) < 0) {
        closeSocket();
        syserr("setsockopt failed");
    }
}

void ShoutcastManager::sendRequest(const std::string& request) {
    int bytesSent = 0, lenSent, toSend = request.length();

    while (toSend > 0) {
        lenSent = write(sock, request.c_str() + bytesSent, toSend);

        if (lenSent < 0 && errno == EINTR) return;
        if (lenSent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            closeSocket();
            syserr("too long waiting for wait");
        }

        if (lenSent < 0) {
            closeSocket();
            syserr("writing on stream socket");
        }

        toSend -= lenSent;
        bytesSent += lenSent;
    }
}

void ShoutcastManager::getResponse(bool giveMetadata) {
    static std::regex regexStatus("^(ICY 200 OK|HTTP/1.0 200 OK|HTTP/1.1 200 OK)\\r\\n$");
    std::string response;

    if (!regex_match(getHeader(giveMetadata), regexStatus)) {
        if (errno == EINTR) return;
        closeSocket();
        syserr("invalid response");
    }

    metaInt = -1;
    while ((response = getHeader(giveMetadata)) != CRLF);
}

std::string ShoutcastManager::getHeader(bool giveMetadata) {
    char c;
    bool lastCR = false;
    std::string result;
    int lenRead;

    while (true) {
        lenRead = read(sock, &c, 1);

        if (lenRead < 0 && errno == EINTR) break;
        if (lenRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            closeSocket();
            syserr("too long waiting for read");
        }

        if (lenRead != 1) {
            closeSocket();
            syserr("reading on stream socket");
        }

        result.push_back(c);
        if (c == LF && lastCR) {
            checkIfMetaint(result, giveMetadata);
            checkIfMetaname(result);

            return result;
        }
        lastCR = c == CR;
    }

    return EMPTY;
}

void ShoutcastManager::checkIfMetaint(std::string result, bool giveMetadata) {
    static std::regex regexMetaint("^icy-metaint: *0*([1-9][0-9]*)\\r\\n$",
        std::regex_constants::ECMAScript | std::regex_constants::icase);
    std::smatch match;

    if (regex_match(result, match, regexMetaint)) {
        if (!giveMetadata) {
            closeSocket();
            syserr("didn't expect metadata");
        }
        metaInt = stoul(match[1]);
    }
}

void ShoutcastManager::checkIfMetaname(std::string result) {
    static std::regex regexMetaname("^icy-name: *(.*)\\r\\n$",
        std::regex_constants::ECMAScript | std::regex_constants::icase);
    std::smatch match;

    if (regex_match(result, match, regexMetaname)) radioName = match[1];
}

int ShoutcastManager::loadData(size_t size, uint8_t *buffer) {
    int bytesRead = 0, lenRead, toRead = size;

    while (toRead > 0) {
        lenRead = read(sock, buffer + bytesRead, toRead);

        if (lenRead < 0 && ((errno == EAGAIN || errno == EWOULDBLOCK) || errno == EINTR))
            return bytesRead;

        if (lenRead <= 0) return -1;

        toRead -= lenRead;
        bytesRead += lenRead;
    }

    return size;
}


const std::string &ShoutcastManager::GetRadioName() const {
    return radioName;
}

unsigned long ShoutcastManager::GetMetaInt() const {
    return metaInt;
}

int ShoutcastManager::GetSock() const {
    return sock;
}
