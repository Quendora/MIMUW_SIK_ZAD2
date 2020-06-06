#ifndef _messageClient_H_
#define _messageClient_H_

#include "../Misc/message.h"

class MessageClient: public Message {
 public:
    explicit MessageClient(uint16_t messlen):Message(messlen){};
    explicit MessageClient(uint16_t type, uint16_t messlen):Message(type, messlen){};

 private:
    bool checkValidity(ssize_t rcvlen);
};

#endif //_messageClient_H_
