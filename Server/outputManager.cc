#include "outputManager.h"

void OutputManager::sendStreamToStdout(int timeout,
    std::shared_ptr<ShoutcastManager> shoutcast, int *finish) {
    this->metaInt = shoutcast->GetMetaInt();
    this->sock = shoutcast->GetSock();
    this->finish = finish;
    this->timeout = timeout * 1000;

    fds[0].fd = sock;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    if (metaInt != -1) sendStreamWithMetadata(shoutcast);
    else sendStreamWithoutMetadata(shoutcast);
}

void OutputManager::sendStreamWithMetadata(const std::shared_ptr<ShoutcastManager>& shoutcast) {
    uint8_t buffer[metaInt];
    uint8_t metaDataLengthBuffer;
    uint8_t metaDataBuffer[MAX_METADATA_LENGTH];
    int bytesToRead, bytesRead;

    while (!(*finish)) {
        bytesToRead = metaInt;

        while (bytesToRead > 0) {
            bytesRead = getData(bytesToRead, buffer, shoutcast);
            bytesToRead -= bytesRead;
            if (write(STDOUT_FILENO, buffer, bytesRead) < 0) {
                shoutcast->closeSocket();
                syserr("write on stdout");
            }
        }

        bytesRead = 0;
        while (bytesRead == 0) {
            bytesRead = getData(1, &metaDataLengthBuffer, shoutcast);
        }
        metaDataLengthBuffer *= METADATA_LENGTH_MULTIPLIER;

        if (metaDataLengthBuffer > 0) {
            bytesToRead = metaDataLengthBuffer;
            while (bytesToRead > 0) {
                bytesRead = getData(bytesToRead, metaDataBuffer, shoutcast);
                bytesToRead -= bytesRead;
                if (write(STDERR_FILENO, metaDataBuffer, bytesRead) < 0) {
                    shoutcast->closeSocket();
                    syserr("write on stderr");
                }
            }
        }
    }
}

void OutputManager::sendStreamWithoutMetadata(const std::shared_ptr<ShoutcastManager>& shoutcast) {
    uint8_t buffer[DEFAULT_BUFF_SIZE];
    int bytesRead;

    while (!(*finish)) {
        bytesRead = getData(DEFAULT_BUFF_SIZE, buffer, shoutcast);
        if (write(STDOUT_FILENO, buffer, bytesRead) < 0) {
            shoutcast->closeSocket();
            syserr("write on stdout");
        }
    }
}

int OutputManager::getData(int bytesToRead, uint8_t *buffer,
    const std::shared_ptr<ShoutcastManager>& shoutcast) {
    int ret = poll(fds, 1, timeout);
    if (ret <= 0) {
        shoutcast->closeSocket();
        syserr("timeout shoutcast");
    }

    int bytesRead = shoutcast->loadData(bytesToRead, buffer);
    if (bytesRead == -1) {
        shoutcast->closeSocket();
        syserr("shoutcast socket");
    }

    return bytesRead;
}
