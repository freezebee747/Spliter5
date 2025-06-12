#pragma once
#include <unordered_set>
#include <unordered_map>
#include <string>
#include "syntax.h"

inline const std::unordered_set<std::string> directive_set = {
	"include", "-include", "vpath", "override"
};

struct DirectiveContext {
	SyntaxChecker& checker;
	std::string filename;
	std::vector<std::string> variables;
	// 향후 확장을 위한 공간
};

void include_directive(DirectiveContext& ctx);
void sinclude_directive(DirectiveContext& ctx);
