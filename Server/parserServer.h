#ifndef _parserServer_H_
#define _parserServer_H_

#include "../Misc/parser.h"

class ParserServer: public Parser {
public:
    explicit ParserServer();
    int parseInput(int argc, char **args);
    const std::string &GetRequest() const;

 private:
    std::string request;

    void makeRequest();
    void addHeader(const std::string& header);
};

#endif //_parserServer_H_
