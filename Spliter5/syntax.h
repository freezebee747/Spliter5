#pragma once
#include "parser.h"
#include "file.h"

#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

struct Explicit_ex {
	unsigned line;
	std::string target;
	std::vector<std::string> prerequisite;
	std::vector<std::pair<unsigned, std::string>> recipes;

};

class SyntaxChecker {
private:
	std::vector<std::shared_ptr<ASTNode>> nodes;
	static std::unordered_map<std::string, std::string> variable_map;
	std::unordered_set<std::string> target_map;
	ErrorCollector ec;
	FileManagement fm;
public:
	SyntaxChecker(std::vector<std::shared_ptr<ASTNode>> node, ErrorCollector& _ec);
	static const std::unordered_map<std::string, std::string>& GetVariables();
	void SyntaxCheck();
	void VariableCheck(Variable& var);
	void DirectiveChecker(Directive& dir);
	void ExplicitRuleCheck(Explicit_Rule& ex);
	void PatternRuleCheck(Pattern_Rule& pr);
	void StaticPatternRuleCheck(Static_Pattern_Rule& spr);
	void IncludeOtherSyntax(SyntaxChecker& syn, const std::string& filename);
	void IncludeOtherSyntax(SyntaxChecker& syn);
	ErrorCollector& GetErrorCollector();
	void PrintErrors();
};