#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace hvlm {

struct Property {
    std::string key;
    std::string value;
};

struct ObjectDecl {
    std::string name;
    std::vector<Property> props;
};

struct SceneDecl {
    std::string name;
    std::vector<ObjectDecl> objects;
};

using AST = SceneDecl;

} // namespace hvlm
