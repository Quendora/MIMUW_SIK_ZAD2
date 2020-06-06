#ifndef _clientManager_H_
#define _clientManager_H_

#include <arpa/inet.h>
#include <unordered_map>
#include <poll.h>

#include "shoutcastManager.h"
#include "messageServer.h"

size_t hashSockaddr(sockaddr s);

bool operator==(const sockaddr& lhs, const sockaddr& rhs);

template<>
struct std::hash<struct sockaddr> {
    std::size_t operator()(struct sockaddr const &s) const noexcept {
        return hashSockaddr(s);
    }
};

enum radioState {
    NO_CHANGE_NO_SEND,
    CHANGE_SEND,
    CHANGE_NO_SEND,
    NO_CHANGE_SEND
};

using Client = int;
using ClientSockaddr = struct sockaddr;
using ClientsMap = std::unordered_map<ClientSockaddr, Client>;

class ServerManager {
 public:
    explicit ServerManager(const std::shared_ptr<ParserServer>& parser,
        const std::shared_ptr<ShoutcastManager>& shoutcast);
    void handleClients(int *finish);

 private:
    int metaInt;
    int clientSock;
    int clientTimeout;
    int radioTimeout;
    int radioCurrTimeout;
    struct ip_mreq ip_mreq;
    ClientsMap clients;
    Message *radioAudio;
    Message *radioMetadata;
    Message *radioIAM;
    std::shared_ptr<ShoutcastManager> shoutcast;
    bool multi;
    bool radioSendAudio;
    bool alreadyReadMetadataByte;
    uint16_t audioBytesLeft;
    uint16_t metadataBytesLeft;
    struct pollfd fds[2];

    void makeSocket(const std::string& address, const std::string& port);
    void makeSocket(const std::string& port);
    void dropMembership();
    void handleGetMessages();
    radioState handleRadio();
    void handleSendMessages();
    void updateClient(Message *mess);
    bool checkTimeout(int lastTime, int timeout);
    int getCurrTime();
    radioState getAudio();
    radioState getMetadata();
    void changeRadioState();
    void setRadioName(std::string name, unsigned long length);
    void handleError();
};

#endif //_clientManager_H_
