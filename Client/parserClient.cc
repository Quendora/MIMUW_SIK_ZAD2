#include "parserClient.h"

static const uint8_t NO_FLAGS = 4;
static const uint8_t NO_OBLIGATORY_FLAGS = 3;

static const Flag validFlagsArr[NO_FLAGS] =
    {HOST_FLAG_C, PORT_FLAG_C_UDP, PORT_FLAG_C_TCP, TIMEOUT_FLAG_C};

static const Flag obligatoryFlagsArr[NO_OBLIGATORY_FLAGS] =
    {HOST_FLAG_C, PORT_FLAG_C_UDP, PORT_FLAG_C_TCP};

ParserClient::ParserClient():
    Parser(validFlagsArr, NO_FLAGS, obligatoryFlagsArr, NO_OBLIGATORY_FLAGS){}

int ParserClient::parseInput(int argc, char **args) {
    if (argc % 2 == 0) return -1;

    for (int i = 1; i < argc; i += 2) {
        if (!checkIfFlagIsValid(args[i], args[i+1]))
            return -1;
    }

    for (const Flag& oFlag: obligatoryFlags)
        if (options.find(oFlag) == options.end()) return -1;

    if (options.find(TIMEOUT_FLAG_C) == options.end())
        options.emplace(TIMEOUT_FLAG_C, DEFAULT_TIMEOUT);

    return 0;
}