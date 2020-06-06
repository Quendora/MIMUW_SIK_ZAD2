#ifndef _shoutcastManager_H_
#define _shoutcastManager_H_

#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <regex>

#include "parserServer.h"
#include "../Misc/err.h"

class ShoutcastManager {
 public:
    void makeConnection(const std::shared_ptr<ParserServer>& parser);
    void closeSocket();
    int loadData(size_t size, uint8_t *buffer);
    const std::string &GetRadioName() const;
    unsigned long GetMetaInt() const;
    int GetSock() const;

 private:
    unsigned long metaInt;
    int sock;
    std::string radioName;

    void getResponse(bool giveMetadata);
    std::string getHeader(bool giveMetadata);
    void sendRequest(const std::string& request);
    void checkIfMetaint(std::string result, bool giveMetadata);
    void checkIfMetaname(std::string result);
    void setSockTimeout(int flag, int time);
};

#endif //_shoutcastManager_H_
