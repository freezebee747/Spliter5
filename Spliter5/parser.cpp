#include "parser.h"

#include <optional>

#include "strutil.h"

using strutil::SplitSpace;
using strutil::Trim;

namespace {

void DeclareTargets(const std::vector<std::string>& targets, ParseResult& result) {
	for (const auto& target : targets) {
		result.declared_targets.insert(target);
	}
	if (result.default_target.empty() && !targets.empty()) {
		result.default_target = targets.front();
	}
}

}  // namespace

Parser::Parser(const DirectiveRegistry& directives) : directives_(directives) {}

ParseResult Parser::Parse(const std::vector<Line>& logical_lines) const {
	ParseResult result;
	std::optional<RuleBlock> rule;

	auto flush_rule = [&]() {
		if (!rule) return;
		if (auto node = ParseRule(*rule, result)) result.nodes.push_back(std::move(node));
		rule.reset();
	};

	for (const Line& line : logical_lines) {
		bool is_recipe = !line.text.empty() && line.text[0] == '\t';
		std::string text = Trim(line.text);
		if (text.empty()) continue;

		//tab으로 시작하는 줄은 직전 규칙의 recipe다.
		if (is_recipe) {
			if (rule) {
				rule->recipes.push_back({ line.number, text });
			}
			else {
				result.errors.Add("E001", line.number, Severity::Error);  // 규칙 없이 recipe가 나옴
			}
			continue;
		}

		flush_rule();
		switch (Classify(text)) {
		case LineKind::Directive:
			result.nodes.push_back(ParseDirective({ line.number, text }));
			break;
		case LineKind::Variable:
			result.nodes.push_back(ParseVariable({ line.number, text }));
			break;
		case LineKind::Rule:
			rule = RuleBlock{ { line.number, text }, {} };
			break;
		case LineKind::Unknown:
			result.errors.Add("E001", line.number, Severity::Error);
			break;
		}
	}
	flush_rule();
	return result;
}

Parser::LineKind Parser::Classify(const std::string& text) const {
	if (directives_.IsDirective(SplitSpace(text).front())) return LineKind::Directive;

	//':'와 '=' 중 먼저 나오는 쪽으로 판단한다.
	//예전에는 '='가 어디에든 있으면 변수로 봤기 때문에 "a: b ; CC=gcc" 같은 규칙을 변수로 오인했다.
	size_t pos = text.find_first_of(":=");
	if (pos == std::string::npos) return LineKind::Unknown;
	if (text[pos] == '=') return LineKind::Variable;                       // =, ?=, +=
	if (text.compare(pos, 2, ":=") == 0) return LineKind::Variable;        // :=
	if (text.compare(pos, 3, "::=") == 0) return LineKind::Variable;       // ::=
	return LineKind::Rule;
}

std::unique_ptr<ASTNode> Parser::ParseDirective(const Line& line) const {
	auto node = std::make_unique<Directive>();
	node->line = line.number;
	std::vector<std::string> words = SplitSpace(line.text);
	node->name = words.front();
	node->arguments.assign(words.begin() + 1, words.end());
	return node;
}

std::unique_ptr<ASTNode> Parser::ParseVariable(const Line& line) const {
	auto node = std::make_unique<Variable>();
	node->line = line.number;

	const std::string& text = line.text;
	size_t equal = text.find('=');
	size_t name_end = equal;
	node->type = VariableType::Simple;
	if (equal > 0) {
		switch (text[equal - 1]) {
		case ':':
			node->type = VariableType::Immediate;
			name_end = equal - 1;
			if (name_end > 0 && text[name_end - 1] == ':') --name_end;  // ::=
			break;
		case '?':
			node->type = VariableType::Conditional;
			name_end = equal - 1;
			break;
		case '+':
			node->type = VariableType::Append;
			name_end = equal - 1;
			break;
		default:
			break;
		}
	}
	node->name = Trim(text.substr(0, name_end));
	node->value = Trim(text.substr(equal + 1));
	return node;
}

std::unique_ptr<ASTNode> Parser::ParseRule(const RuleBlock& block, ParseResult& result) const {
	const std::string& head_text = block.head.text;
	unsigned line = block.head.number;

	//"target: prereq ; recipe" 형태: ';' 뒤는 recipe다.
	size_t semicolon = head_text.find(';');
	std::string head = head_text.substr(0, semicolon);

	std::vector<Line> recipes;
	if (semicolon != std::string::npos) {
		std::string inline_recipe = Trim(head_text.substr(semicolon + 1));
		if (!inline_recipe.empty()) recipes.push_back({ line, inline_recipe });
	}
	recipes.insert(recipes.end(), block.recipes.begin(), block.recipes.end());

	size_t colon_count = strutil::CountChar(head, ':');
	size_t colon = head.find(':');

	if (colon_count == 1) {
		std::string target_part = Trim(head.substr(0, colon));
		std::string prereq_part = Trim(head.substr(colon + 1));

		if (head.find('%') != std::string::npos) {
			auto node = std::make_unique<PatternRule>();
			node->line = line;
			node->recipes = recipes;
			node->target_pattern = target_part;
			node->prerequisite_pattern = prereq_part;
			return node;
		}
		if (prereq_part.empty()) {
			auto node = std::make_unique<PhonyTarget>();
			node->line = line;
			node->recipes = recipes;
			node->target = target_part;
			DeclareTargets({ target_part }, result);
			return node;
		}
		auto node = std::make_unique<ExplicitRule>();
		node->line = line;
		node->recipes = recipes;
		node->targets = SplitSpace(target_part);
		node->prerequisites = SplitSpace(prereq_part);
		DeclareTargets(node->targets, result);
		return node;
	}

	if (colon_count == 2) {
		size_t next_colon = head.find(':', colon + 1);
		auto node = std::make_unique<StaticPatternRule>();
		node->line = line;
		node->recipes = recipes;
		node->targets = SplitSpace(head.substr(0, colon));
		node->target_pattern = Trim(head.substr(colon + 1, next_colon - colon - 1));
		node->prerequisite_pattern = Trim(head.substr(next_colon + 1));
		return node;
	}

	result.errors.Add("E001", line, Severity::Error);
	return nullptr;
}
