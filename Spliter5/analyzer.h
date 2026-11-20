#pragma once
#include <string>
#include <vector>

#include "AST.h"
#include "context.h"
#include "directive.h"
#include "error.h"
#include "function.h"
#include "parser.h"

// 읽기 → 파싱 → 검사 흐름을 조립하는 진입점(Facade).
// include를 만나면 같은 MakefileContext로 하위 파일을 분석하므로
// 포함된 파일의 변수와 타깃이 메인 파일과 공유된다.
class MakefileAnalyzer {
public:
	MakefileAnalyzer(const IFileLookup& files, const FunctionRegistry& functions, const DirectiveRegistry& directives);

	ErrorCollector AnalyzeFile(const std::string& path);
	ErrorCollector AnalyzeText(const std::string& text);

	const MakefileContext& Context() const { return context_; }

private:
	void Analyze(const std::vector<Line>& physical_lines, ErrorCollector& errors);
	void Include(const std::string& path, unsigned line, bool optional, ErrorCollector& parent);
	std::string ResolveInclude(const std::string& path) const;

	MakefileContext context_;
	const FunctionRegistry& functions_;
	const DirectiveRegistry& directives_;
	Parser parser_;
	std::vector<std::string> file_stack_;  // 분석 중인 파일 (include 순환 감지용)
};
