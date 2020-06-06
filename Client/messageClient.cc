#include "messageClient.h"

bool MessageClient::checkValidity(ssize_t rcvlen) {
    return (getType() == IAM_T || getType() == AUDIO_T || getType() == METADATA_T) &&
        rcvlen == getLength() + MESS_HEADER_LENGTH;
}
