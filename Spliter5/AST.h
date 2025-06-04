#pragma once
#include <string>
#include <vector>

enum class VariableType {
    Simple,    // =
    Immediate, // :=
    Append,    // +=
    Conditional // ?=
};

struct ASTNode {
    virtual ~ASTNode() = default;
};

struct Variable :ASTNode {
    unsigned line;
    std::string name;
    std::string value;
    VariableType type;
};

struct Explicit_Rule : ASTNode {
    unsigned line;
    std::vector<std::string> target;
    std::vector<std::string> prerequisite;
    std::vector<std::pair<unsigned, std::string>> recipes;
};

struct Pattern_Rule : ASTNode {
    unsigned line;
    std::string target_pattern;
    std::string prerequisite_pattern;
    std::vector<std::pair<unsigned, std::string>> recipes;
};

struct Static_Pattern_Rule : ASTNode {
    unsigned line;
    std::vector<std::string> target;
    std::string target_pattern;
    std::string prerequisite_pattern;
    std::vector<std::pair<unsigned, std::string>> recipes;
};

struct Phony_Target : ASTNode {
    unsigned line;
    std::string target;
    std::vector<std::pair<unsigned, std::string>> recipes;
};

