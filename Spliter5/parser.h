#pragma once
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "AST.h"
#include "directive.h"
#include "error.h"

// 파싱 결과. 예전처럼 static 멤버에 쌓지 않고 값으로 돌려준다.
struct ParseResult {
	NodeList nodes;
	std::unordered_set<std::string> declared_targets;
	std::string default_target;  // 첫 번째 규칙의 타깃 (make의 기본 목표)
	ErrorCollector errors;
};

// 논리적인 줄 목록 → AST. 파일 읽기는 makefile_reader가, 의미 검사는 SyntaxChecker가 맡는다.
class Parser {
public:
	explicit Parser(const DirectiveRegistry& directives);
	ParseResult Parse(const std::vector<Line>& logical_lines) const;

private:
	enum class LineKind { Directive, Variable, Rule, Unknown };

	struct RuleBlock {
		Line head;
		std::vector<Line> recipes;
	};

	LineKind Classify(const std::string& text) const;
	std::unique_ptr<ASTNode> ParseDirective(const Line& line) const;
	std::unique_ptr<ASTNode> ParseVariable(const Line& line) const;
	std::unique_ptr<ASTNode> ParseRule(const RuleBlock& block, ParseResult& result) const;

	const DirectiveRegistry& directives_;
};
