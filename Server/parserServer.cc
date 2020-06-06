#include "parserServer.h"

static const uint8_t NO_FLAGS = 8;
static const uint8_t NO_OBLIGATORY_FLAGS = 3;

static const Flag validFlagsArr[NO_FLAGS] =
    {HOST_FLAG, RESOURCE_FLAG, PORT_FLAG_A, GIVE_METADATA_FLAG, TIMEOUT_FLAG_A,
        PORT_FLAG_B, TIMEOUT_FLAG_B, MULTI_FLAG};

static const Flag obligatoryFlagsArr[NO_OBLIGATORY_FLAGS] =
    {HOST_FLAG, RESOURCE_FLAG, PORT_FLAG_A};

ParserServer::ParserServer():
    Parser(validFlagsArr, NO_FLAGS, obligatoryFlagsArr, NO_OBLIGATORY_FLAGS){}

int ParserServer::parseInput(int argc, char **args) {
    if (argc % 2 == 0) return -1;

    for (int i = 1; i < argc; i += 2) {
        if (!checkIfFlagIsValid(args[i], args[i+1]))
            return -1;
    }

    for (const Flag& oFlag: obligatoryFlags)
        if (options.find(oFlag) == options.end()) return -1;

    if (options.find(TIMEOUT_FLAG_A) == options.end())
        options.emplace(TIMEOUT_FLAG_A, DEFAULT_TIMEOUT);

    if (options.find(TIMEOUT_FLAG_B) == options.end())
        options.emplace(TIMEOUT_FLAG_B, DEFAULT_TIMEOUT);

    makeRequest();
    return 0;
}

void ParserServer::makeRequest() {
    request = "";
    addHeader("GET " + getValueOfFlag(RESOURCE_FLAG) +  " HTTP/1.1");
    addHeader("Host: " + getValueOfFlag(HOST_FLAG) + ":" + getValueOfFlag(PORT_FLAG_A));

    if (checkIfFlagExists(GIVE_METADATA_FLAG) && getValueOfFlag(GIVE_METADATA_FLAG) == YES)
        addHeader("Icy-MetaData: 1");

    request += CRLF;
}

void ParserServer::addHeader(const std::string& header) {
    request += header + CRLF;
}

const std::string &ParserServer::GetRequest() const {
    return request;
}
