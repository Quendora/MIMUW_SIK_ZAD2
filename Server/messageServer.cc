#include "messageServer.h"

bool MessageServer::checkValidity(ssize_t rcvlen) {
    return (getType() == DISCOVER_T || getType() == KEEPALIVE_T) &&
        getLength() == 0 && rcvlen == MESS_HEADER_LENGTH;
}
