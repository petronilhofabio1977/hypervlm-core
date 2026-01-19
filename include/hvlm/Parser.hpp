#pragma once
#include <string>
#include "hvlm/AST.hpp"

namespace hvlm {

class Parser {
public:
    static bool parse(const std::string& src, AST& out, std::string& err);
};

} // namespace hvlm
