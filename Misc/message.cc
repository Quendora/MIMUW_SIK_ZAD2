#include <netinet/in.h>
#include <cerrno>

#include "message.h"

Message::Message(uint16_t messlen) {
    this->messLength = messlen + MESS_HEADER_LENGTH;
    mess = std::shared_ptr<char[]>(new char[messLength]);

    ((uint16_t*) mess.get())[1] = htons(messlen);
}

Message::Message(uint16_t type, uint16_t messlen):Message(messlen) {
    ((uint16_t*) mess.get())[0] = htons(type);
}

enum mess_status Message::getMessage(int sock) {
    socklen_t addrlen = (socklen_t) sizeof(src_addr);

    ssize_t rcvlen = recvfrom(sock, mess.get(), messLength, 0, &src_addr, &addrlen);

    if (rcvlen < 0 && errno == EINTR) return NONE;
    if (rcvlen < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return NONE;
    if (rcvlen < 0 && (errno == EBADF || errno == ENOTSOCK))
        syserr("reading on stream socket");

    return (enum mess_status) checkValidity(rcvlen);
}

void Message::sendMessage(int sock, struct sockaddr destAddr) {
    socklen_t addrlen = (socklen_t) sizeof(destAddr);
    ssize_t sndlen = sendto(sock, mess.get(), messLength, 0, &destAddr, addrlen);

    if (sndlen < 0 && errno == EINTR) return;
    if (sndlen < 0 && (errno == EBADF || errno == ENOTSOCK))
        syserr("reading on stream socket");
}

sockaddr Message::getSrcAddr() {
    return src_addr;
}

uint16_t Message::getType() {
    return ntohs(*((uint16_t*) mess.get()));
}

uint16_t Message::getLength() {
    return ntohs(*((uint16_t*) (mess.get() + 2)));
}

std::shared_ptr<char[]> Message::getBuff() {
    return mess;
}

void Message::setMessLength(uint16_t messLen) {
    ((uint16_t*) mess.get())[1] = htons(messLen);
    messLength = messLen + MESS_HEADER_LENGTH;
}




