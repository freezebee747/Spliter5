#include "analyzer.h"

#include <algorithm>
#include <filesystem>
#include <system_error>

#include "read.h"
#include "syntax.h"

namespace fs = std::filesystem;

namespace {

std::string CanonicalKey(const std::string& path) {
	std::error_code ec;
	fs::path canonical = fs::weakly_canonical(path, ec);
	return ec ? path : canonical.string();
}

}  // namespace

MakefileAnalyzer::MakefileAnalyzer(const IFileLookup& files, const FunctionRegistry& functions, const DirectiveRegistry& directives)
	: context_(files), functions_(functions), directives_(directives), parser_(directives) {}

ErrorCollector MakefileAnalyzer::AnalyzeFile(const std::string& path) {
	ErrorCollector errors;
	bool ok = false;
	std::vector<Line> lines = makefile_reader::ReadPhysicalLines(path, &ok);
	if (!ok) {
		errors.Add("E300", 0, Severity::Fatal);
		return errors;
	}

	file_stack_.push_back(CanonicalKey(path));
	Analyze(lines, errors);
	file_stack_.pop_back();
	return errors;
}

ErrorCollector MakefileAnalyzer::AnalyzeText(const std::string& text) {
	ErrorCollector errors;
	Analyze(makefile_reader::SplitPhysicalLines(text), errors);
	return errors;
}

void MakefileAnalyzer::Analyze(const std::vector<Line>& physical_lines, ErrorCollector& errors) {
	ParseResult parsed = parser_.Parse(makefile_reader::ToLogicalLines(physical_lines));
	errors.Append(parsed.errors);
	context_.declared_targets.insert(parsed.declared_targets.begin(), parsed.declared_targets.end());

	SyntaxChecker checker(context_, errors, functions_, directives_,
		[this, &errors](const std::string& path, unsigned line, bool optional) {
			Include(path, line, optional, errors);
		});
	checker.Check(parsed.nodes);
}

void MakefileAnalyzer::Include(const std::string& path, unsigned line, bool optional, ErrorCollector& parent) {
	std::string resolved = ResolveInclude(path);
	if (resolved.empty()) {
		if (!optional) parent.Add("E301", line, Severity::Error);
		return;
	}
	if (std::find(file_stack_.begin(), file_stack_.end(), CanonicalKey(resolved)) != file_stack_.end()) {
		parent.Add("E302", line, Severity::Error);
		return;
	}
	parent.Append(AnalyzeFile(resolved), path);
}

// include 경로는 그대로, 그다음 현재 분석 중인 파일의 디렉토리 기준으로 찾는다.
std::string MakefileAnalyzer::ResolveInclude(const std::string& path) const {
	std::error_code ec;
	if (fs::is_regular_file(path, ec)) return path;
	if (!file_stack_.empty()) {
		fs::path candidate = fs::path(file_stack_.back()).parent_path() / path;
		if (fs::is_regular_file(candidate, ec)) return candidate.string();
	}
	return "";
}
