#ifndef _outputManager_H_
#define _outputManager_H_

#include <poll.h>

#include "../Misc/glo.h"
#include "../Misc/err.h"
#include "shoutcastManager.h"

class OutputManager {
 public:
    void sendStreamToStdout(int timeout, std::shared_ptr<ShoutcastManager> shoutcast, int *finish);

 private:
    int metaInt;
    int sock;
    int *finish;
    int timeout;
    struct pollfd fds[1];

    void sendStreamWithMetadata(const std::shared_ptr<ShoutcastManager>& shoutcast);
    void sendStreamWithoutMetadata(const std::shared_ptr<ShoutcastManager>& shoutcast);
    int getData(int bytesToRead, uint8_t* buffer, const std::shared_ptr<ShoutcastManager>& shoutcast);
};

#endif //_outputManager_H_
