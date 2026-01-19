#include "hvlm/Lexer.hpp"

namespace hvlm {

std::vector<Token> Lexer::tokenize(const std::string &src){
    std::vector<Token> out;
    Token t; t.text = src;
    out.push_back(t);
    return out;
}

}
