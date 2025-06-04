#pragma once
#include "parser.h"

#include <unordered_map>

class SyntaxChecker {
private:
	std::vector<std::shared_ptr<ASTNode>> nodes;
	static std::unordered_map<std::string, std::string> variable_map;
	ErrorCollector ec;
public:
	SyntaxChecker(std::vector<std::shared_ptr<ASTNode>>& node, ErrorCollector& _ec);
	static const std::unordered_map<std::string, std::string>& GetVariables();
	void SyntaxCheck();
	void VariableCheck(Variable& var);
	void ExplicitRuleCheck(Explicit_Rule& ex);

};