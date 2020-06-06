#include "parser.h"

Parser::Parser(const Flag validFlags[], uint8_t validNum,
    const Flag obligatoryFlags[], uint8_t obligatoryNum)
{
    for (int i = 0; i < validNum; i++)
        this->validFlags.push_back(validFlags[i]);

    for (int i = 0; i < obligatoryNum; i++)
        this->obligatoryFlags.push_back(obligatoryFlags[i]);

    this->options = std::map<Flag, std::string>();
}

bool Parser::checkIfFlagIsValid(const std::string& option, const std::string& value) {
    Flag newFlag;
    bool valid = false;

    for (const Flag& validFlag: validFlags) {
        if (validFlag.first == option) {
            newFlag = validFlag;
            valid = true;
            break;
        }
    }

    if (!valid || options.find(newFlag) != options.end()) return false;

    switch (newFlag.second) {
        case TIMEOUT:
        case PORT:
            valid = checkIfPositiveNumber(value);
            break;
        case METADATA:
            valid = value == YES || value == NO;
            break;
        default:
            break;
    }

    if (!valid) return false;
    else options.emplace(newFlag, value);

    return true;
}

bool Parser::checkIfPositiveNumber(std::string s) {
    std::string::const_iterator it = s.begin();

    while (it != s.end() && *it == ZERO_CHAR) {
        s.erase(0);
        ++it;
    }

    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end() && stoi(s) > 0;
}

std::string Parser::getValueOfFlag(const Flag &flag) {
    return options.at(flag);
}

bool Parser::checkIfFlagExists(const Flag &flag) {
    return options.find(flag) != options.end();
}
