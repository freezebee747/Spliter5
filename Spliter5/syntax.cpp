#include "syntax.h"

#include <cctype>
#include <optional>
#include <regex>

#include "strutil.h"

SyntaxChecker::SyntaxChecker(MakefileContext& context, ErrorCollector& errors,
							 const FunctionRegistry& functions, const DirectiveRegistry& directives,
							 IncludeHandler include)
	: context_(context),
	  errors_(errors),
	  expander_(context.variables, functions, context.files),
	  directives_(directives),
	  include_(std::move(include)) {}

void SyntaxChecker::Check(const NodeList& nodes) {
	for (const auto& node : nodes) {
		node->Accept(*this);
	}
}

void SyntaxChecker::Visit(const Directive& node) {
	DirectiveCall call{ node, ExpandWords(node.arguments), nullptr };
	if (include_) {
		call.include = [this, &node](const std::string& path, bool optional) {
			include_(path, node.line, optional);
		};
	}
	directives_.Run(call);
}

void SyntaxChecker::Visit(const Variable& node) {
	//변수 이름 검사
	if (!IsValidVariableName(node.name)) {
		errors_.Add("E011", node.line, Severity::Error);
		return;
	}

	const VariableEntry* existing = context_.variables.Find(node.name);
	switch (node.type) {
	case VariableType::Conditional:  // ?= : 이미 있으면 무시
		if (!existing) DefineRecursive(node.name, node.value, node.line);
		return;

	case VariableType::Append:  // += : 기존 값 뒤에 붙인다 (중복 정의 아님)
		if (!existing) {
			DefineRecursive(node.name, node.value, node.line);
		}
		else if (existing->flavor == Flavor::Simple) {
			std::string appended = strutil::Trim(existing->value + " " + expander_.Expand(node.value));
			context_.variables.Set(node.name, { appended, Flavor::Simple });
		}
		else {
			DefineRecursive(node.name, strutil::Trim(existing->value + " " + node.value), node.line);
		}
		return;

	case VariableType::Simple:
	case VariableType::Immediate:
		//변수 이름 중복 검사
		if (existing) {
			errors_.Add("E012", node.line, Severity::Error);
			return;
		}
		if (node.type == VariableType::Immediate) {
			//즉시 확장 변수는 정의 시점에 값을 확장해서 저장한다
			context_.variables.Set(node.name, { strutil::Trim(expander_.Expand(node.value)), Flavor::Simple });
		}
		else {
			DefineRecursive(node.name, node.value, node.line);
		}
		return;
	}
}

// 재귀 확장 변수를 정의하고, 순환 참조가 생기면 정의를 되돌린다.
void SyntaxChecker::DefineRecursive(const std::string& name, const std::string& value, unsigned line) {
	const VariableEntry* existing = context_.variables.Find(name);
	std::optional<VariableEntry> previous;
	if (existing) previous = *existing;

	context_.variables.Set(name, { value, Flavor::Recursive });
	expander_.Expand("$(" + name + ")");
	if (expander_.CycleDetected()) {
		errors_.Add("E013", line, Severity::Error);
		if (previous) {
			context_.variables.Set(name, *previous);
		}
		else {
			context_.variables.Erase(name);
		}
	}
}

bool SyntaxChecker::IsValidVariableName(const std::string& name) {
	if (name.empty()) return false;
	if (!(std::isalpha(static_cast<unsigned char>(name[0])) || name[0] == '_')) return false;
	for (char c : name) {
		if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
	}
	return true;
}

void SyntaxChecker::Visit(const ExplicitRule& node) {
	//target과 prerequisite는 즉시 확장
	CheckTargets(ExpandWords(node.targets), node.line);
	CheckPrerequisites(ExpandWords(node.prerequisites), node.line);
}

void SyntaxChecker::Visit(const PhonyTarget& node) {
	CheckTargets(ExpandWords(node.target), node.line);
}

void SyntaxChecker::Visit(const PatternRule& node) {
	CheckTargetPattern(strutil::Trim(expander_.Expand(node.target_pattern)), node.line);
}

void SyntaxChecker::Visit(const StaticPatternRule& node) {
	std::string target_pattern = strutil::Trim(expander_.Expand(node.target_pattern));
	if (!CheckTargetPattern(target_pattern, node.line)) return;

	std::vector<std::string> prerequisite_patterns = ExpandWords(node.prerequisite_pattern);
	for (const auto& target : ExpandWords(node.targets)) {
		std::string stem;
		if (!strutil::MatchPattern(target_pattern, target, &stem)) {
			errors_.Add("E203", node.line, Severity::Error);
			continue;
		}
		std::vector<std::string> prerequisites;
		for (const auto& pattern : prerequisite_patterns) {
			prerequisites.push_back(strutil::ReplacePercent(pattern, stem));
		}
		CheckTargets({ target }, node.line);
		CheckPrerequisites(prerequisites, node.line);
	}
}

bool SyntaxChecker::CheckTargetPattern(const std::string& pattern, unsigned line) {
	size_t percent_count = strutil::CountChar(pattern, '%');
	if (percent_count == 0) {
		errors_.Add("E201", line, Severity::Error);
		return false;
	}
	if (percent_count > 1) {
		errors_.Add("E202", line, Severity::Error);
		return false;
	}
	return true;
}

std::vector<std::string> SyntaxChecker::ExpandWords(const std::vector<std::string>& words) {
	return ExpandWords(strutil::Join(words, " "));
}

std::vector<std::string> SyntaxChecker::ExpandWords(const std::string& text) {
	return strutil::SplitSpace(expander_.Expand(text));
}

void SyntaxChecker::CheckTargets(const std::vector<std::string>& targets, unsigned line) {
	for (const auto& target : targets) {
		if (target.find('*') != std::string::npos) {
			errors_.Add("E101", line, Severity::Error);
			continue;
		}
		//target에 $, = 가 쓰였으면 주의를 줌(에러가 아님)
		if (target.find_first_of("$=") != std::string::npos) {
			errors_.Add("E102", line, Severity::Warning);
		}
		//중복 검사는 확장된 이름 기준
		if (!context_.defined_targets.insert(target).second) {
			errors_.Add("E103", line, Severity::Error);
		}
	}
}

void SyntaxChecker::CheckPrerequisites(const std::vector<std::string>& prerequisites, unsigned line) {
	for (const auto& prerequisite : prerequisites) {
		if (prerequisite.find('$') != std::string::npos) continue;  // 자동 변수 등은 실행 시점에 정해짐
		if (!IsKnownPrerequisite(prerequisite)) {
			errors_.Add("E151", line, Severity::Error);
		}
	}
}

bool SyntaxChecker::IsKnownPrerequisite(const std::string& prerequisite) const {
	if (context_.declared_targets.count(prerequisite) || context_.defined_targets.count(prerequisite)) return true;
	if (context_.files.Exists(prerequisite)) return true;
	if (!HasGlobChars(prerequisite)) return false;

	//와일드카드가 쓰였으면 파일과 타깃 모두에서 찾아본다
	if (!context_.files.Glob(prerequisite).empty()) return true;
	std::regex rx(GlobToRegex(prerequisite));
	for (const auto& target : context_.defined_targets) {
		if (std::regex_match(target, rx)) return true;
	}
	return false;
}
