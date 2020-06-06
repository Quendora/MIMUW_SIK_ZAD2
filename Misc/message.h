#ifndef _message_H_
#define _message_H_

#include <sys/socket.h>
#include <memory>

#include "err.h"

enum type {
    DISCOVER_T = 1,
    IAM_T = 2,
    KEEPALIVE_T = 3,
    AUDIO_T = 4,
    METADATA_T = 6
};

enum mess_status {
    INVALID = 0,
    OK = 1,
    NONE = 2,
};

static const int BUFFER_SIZE = 16348;
static const int MESS_HEADER_LENGTH = 4;
static const enum type types[5] = {DISCOVER_T, IAM_T, KEEPALIVE_T,
    AUDIO_T, METADATA_T};

class Message {
 public:
    explicit Message(uint16_t messlen);
    explicit Message(uint16_t type, uint16_t messlen);
    enum mess_status getMessage(int sock);
    void sendMessage(int sock, struct sockaddr destAddr);
    sockaddr getSrcAddr();
    uint16_t getLength();
    uint16_t getType();
    std::shared_ptr<char[]> getBuff();
    void setMessLength(uint16_t messLen);

 protected:
    unsigned long  messLength;
    std::shared_ptr<char[]> mess;
    struct sockaddr src_addr;

    virtual bool checkValidity(ssize_t rcvlen) = 0;
};

#endif //_message_H_
