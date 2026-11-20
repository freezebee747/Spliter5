#pragma once
#include <functional>
#include <string>
#include <vector>

#include "AST.h"
#include "context.h"
#include "directive.h"
#include "error.h"
#include "expander.h"
#include "function.h"

// AST를 방문하며 의미 검사를 한다.
// 필요한 것(문맥, 오류 수집기, 함수/지시어 표, include 동작)은 모두 생성자로 주입받는다.
class SyntaxChecker : public ASTVisitor {
public:
	using IncludeHandler = std::function<void(const std::string& path, unsigned line, bool optional)>;

	SyntaxChecker(MakefileContext& context, ErrorCollector& errors,
				  const FunctionRegistry& functions, const DirectiveRegistry& directives,
				  IncludeHandler include = nullptr);

	void Check(const NodeList& nodes);

	void Visit(const Directive& node) override;
	void Visit(const Variable& node) override;
	void Visit(const ExplicitRule& node) override;
	void Visit(const PatternRule& node) override;
	void Visit(const StaticPatternRule& node) override;
	void Visit(const PhonyTarget& node) override;

private:
	static bool IsValidVariableName(const std::string& name);
	void DefineRecursive(const std::string& name, const std::string& value, unsigned line);
	bool CheckTargetPattern(const std::string& pattern, unsigned line);

	std::vector<std::string> ExpandWords(const std::vector<std::string>& words);
	std::vector<std::string> ExpandWords(const std::string& text);
	void CheckTargets(const std::vector<std::string>& targets, unsigned line);
	void CheckPrerequisites(const std::vector<std::string>& prerequisites, unsigned line);
	bool IsKnownPrerequisite(const std::string& prerequisite) const;

	MakefileContext& context_;
	ErrorCollector& errors_;
	VariableExpander expander_;
	const DirectiveRegistry& directives_;
	IncludeHandler include_;
};
