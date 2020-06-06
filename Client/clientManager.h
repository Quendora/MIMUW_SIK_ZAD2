#ifndef _clientManager_H_
#define _clientManager_H_

#include <fcntl.h>
#include <netdb.h>
#include <poll.h>

#include "parserClient.h"
#include "../Misc/err.h"
#include "messageClient.h"
#include "userInterface.h"

class ClientManager {
 public:
    explicit ClientManager(const std::shared_ptr<ParserClient>& parser);
    void handleConnections(int *finish);

 private:
    std::shared_ptr<TelnetManager> telnet;
    std::shared_ptr<UserInterface> userInterface;
    std::shared_ptr<MessageClient> radioDiscover;
    std::shared_ptr<MessageClient> keepAliveMess;
    std::string address;
    struct sockaddr addr;
    struct pollfd fds[3];
    int port;
    int serverSock;
    int telnetSock;
    int radioTimeout;
    int radioCurrTimeout;
    int keepaliveTimeout;

    void makeServerSocket();
    void makeTelnetSocket(std::shared_ptr<ParserClient> parser);
    void makeTelnetSetup();
    void closeSockets();
    userAction handleTelnetInput();
    bool getDataFromServer();
    void pickRadio();
    void addNewServer(const std::shared_ptr<MessageClient>& mess);
    void validateAndWriteOutput(const std::shared_ptr<MessageClient>& mess);
    bool validateAndSetMetadata(const std::shared_ptr<MessageClient>& mess);
    void updateTimeouts(int lastTime);
    int getCurrTime();
    void updateKeepalive(int lastTime);
    void handleTelnetError();
    void lookForServers();
};

#endif //_clientManager_H_
