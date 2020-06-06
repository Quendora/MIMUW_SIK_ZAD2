#ifndef _messageServer_H_
#define _messageServer_H_

#include "../Misc/message.h"

class MessageServer: public Message {
 public:
    explicit MessageServer(uint16_t messlen):Message(messlen){};
    explicit MessageServer(uint16_t type, uint16_t messlen):Message(type, messlen){};

 private:
    bool checkValidity(ssize_t rcvlen);
};

#endif //_messageServer_H_
