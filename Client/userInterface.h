#ifndef _userInterface_H_
#define _userInterface_H_

#include <vector>
#include <csignal>
#include <sys/socket.h>
#include <memory>
#include <regex>

#include "telnetManager.h"

enum userAction {
    TELNET_ERROR,
    SEARCH,
    END,
    PICK_RADIO,
    VOID,
    OUT_OF_INPUT,
    DEFAULT
};

class Server {
 public:
    sockaddr addr;
    std::string radioName;
};

size_t hashSockaddr(sockaddr s);
bool operator==(const sockaddr& lhs, const sockaddr& rhs);

class UserInterface {
 public:
    explicit UserInterface();
    void addServer(const std::shared_ptr<Server>& server);
    void deletePickedServer();
    bool sendInterface();
    void setTelnet(std::shared_ptr<TelnetManager> telnet);
    void setMetadata(std::string metadata);
    userAction getInput();
    sockaddr getPickedRadioAddr();
    bool getPicked();

 private:
    std::shared_ptr<TelnetManager> telnet;
    std::string radioMetadata;
    std::vector<std::shared_ptr<Server>> servers;
    unsigned cursorPosition;
    unsigned optionsNum;
    bool picked;
    sockaddr pickedRadioAddr;
    void changeServerName(const std::shared_ptr<Server>& server);
    bool checkIfServerAlreadyExists(const sockaddr& addr);
    void clearUnusedServers();
};

#endif //_userInterface_H_
