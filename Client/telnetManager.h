#ifndef _telnetManager_H_
#define _telnetManager_H_

#include <string>

enum telnetAction {
    ERROR,
    UP,
    DOWN,
    SELECT,
    OTHER,
    TELNET_OUT_OF_INPUT
};


class TelnetManager {
 public:
    bool makeSocket(int sock);
    bool send(const std::string& mess);
    telnetAction readInput();
    void closeSocket();
    int getSocket();

 private:
    int telnetSock;
    bool initTelnetConfig();
    telnetAction resolveError();
};

#endif //_telnetManager_H_
