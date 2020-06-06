#ifndef _parserClient_H_
#define _parserClient_H_

#include "../Misc/parser.h"

class ParserClient: public Parser {
 public:
    explicit ParserClient();
    int parseInput(int argc, char **args);
};

#endif //_parserClient_H_
