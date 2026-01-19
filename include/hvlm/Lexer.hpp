#pragma once
#include <string>
#include <vector>

namespace hvlm {

struct Token {
    std::string text;
};

class Lexer {
public:
    std::vector<Token> tokenize(const std::string &src);
};

}
