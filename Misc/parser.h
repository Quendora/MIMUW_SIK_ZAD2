#ifndef _parser_H_
#define _parser_H_

#include <map>
#include <vector>

#include "glo.h"

class Parser {
 public:
    explicit Parser(const Flag validFlags[], uint8_t validNum,
                    const Flag obligatoryFlags[], uint8_t obligatoryNum);

    virtual int parseInput(int argc, char **args) = 0;
    std::string getValueOfFlag(const Flag& flag);
    bool checkIfFlagExists(const Flag& flag);

 protected:
    std::vector<Flag> validFlags;
    std::vector<Flag> obligatoryFlags;
    std::map<Flag, std::string> options;

    bool checkIfFlagIsValid(const std::string& option, const std::string& value);
    static bool checkIfPositiveNumber(std::string s);
};

#endif //_parser_H_
