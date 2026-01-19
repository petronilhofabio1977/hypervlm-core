#include "hvlm/Parser.hpp"
#include <sstream>
#include <cctype>
#include <algorithm>

namespace hvlm {

static std::string trim(const std::string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a==std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b-a+1);
}

bool Parser::parse(const std::string& src, AST& out, std::string& err) {
    std::istringstream in(src);
    std::string token;

    if (!(in >> token)) { err = "empty source"; return false; }
    if (token != "scene") { err = "expected 'scene'"; return false; }

    std::string name;
    in >> std::ws;
    char c = in.peek();
    if (c == '"') {
        in.get();
        std::getline(in, name, '"');
    } else {
        in >> name;
    }
    out.name = trim(name);

    in >> std::ws;
    if (in.peek() != '{') { err = "expected '{'"; return false; }
    in.get();

    while (true) {
        in >> std::ws;
        if (in.peek() == '}') { in.get(); break; }

        std::string obj;
        while (std::isalpha(in.peek())) obj.push_back(in.get());
        obj = trim(obj);
        if (obj.empty()) {
            if (in.peek()=='}') break;
            in.get();
            continue;
        }

        ObjectDecl od;
        od.name = obj;

        in >> std::ws;
        if (in.peek() == '(') {
            in.get();

            while (true) {
                in >> std::ws;
                if (in.peek() == ')') { in.get(); break; }

                std::string key, val;
                while (isalnum(in.peek()) || in.peek()=='_') key.push_back(in.get());

                in >> std::ws;
                if (in.peek()=='=') in.get();
                in >> std::ws;

                if (in.peek()=='"') {
                    in.get();
                    std::getline(in, val, '"');
                } else {
                    while (in.peek()!=',' && in.peek()!=')' && in.peek()!=EOF)
                        val.push_back(in.get());
                }

                od.props.push_back({trim(key), trim(val)});

                in >> std::ws;
                if (in.peek()==',') in.get();
            }
        }

        out.objects.push_back(od);
        in >> std::ws;
    }

    return true;
}

} // namespace hvlm
