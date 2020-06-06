#ifndef _glo_H_
#define _glo_H_

#include <iostream>
#include <unistd.h>

enum FlagTypes {
    ADDRESS,
    RESOURCE,
    PORT,
    METADATA,
    TIMEOUT
};

using Flag = std::pair<std::string, FlagTypes>;
static const std::string YES = "yes";
static const std::string NO = "no";
static const char LF = '\n';
static const char CR = '\r';
static const char ZERO_CHAR = '0';
static const std::string CRLF = "\r\n";
static const std::string EMPTY = "";
static const uint MAX_METADATA_LENGTH = 4080;
static const uint16_t DEFAULT_BUFF_SIZE = 65535;
static const int METADATA_LENGTH_MULTIPLIER = 16;
static const std::string DEFAULT_TIMEOUT = "5";

static const Flag HOST_FLAG = {"-h", FlagTypes::ADDRESS};
static const Flag RESOURCE_FLAG = {"-r", FlagTypes::RESOURCE};
static const Flag PORT_FLAG_A = {"-p", FlagTypes::PORT};
static const Flag GIVE_METADATA_FLAG = {"-m", FlagTypes::METADATA};
static const Flag TIMEOUT_FLAG_A = {"-t", FlagTypes::TIMEOUT};
static const Flag PORT_FLAG_B = {"-P", FlagTypes::PORT};
static const Flag TIMEOUT_FLAG_B = {"-T", FlagTypes::TIMEOUT};
static const Flag MULTI_FLAG = {"-B", FlagTypes::ADDRESS};

static const Flag HOST_FLAG_C = {"-H", FlagTypes::ADDRESS};
static const Flag PORT_FLAG_C_UDP = {"-P", FlagTypes::PORT};
static const Flag PORT_FLAG_C_TCP = {"-p", FlagTypes::PORT};
static const Flag TIMEOUT_FLAG_C = {"-T", FlagTypes::TIMEOUT};

#endif //_glo_H_
