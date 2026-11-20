#include "expander.h"

#include <algorithm>

#include "strutil.h"

namespace {

// text[open_pos]의 여는 괄호와 짝이 맞는 닫는 괄호 위치. 없으면 npos.
size_t FindClosing(const std::string& text, size_t open_pos) {
	char open = text[open_pos];
	char close = (open == '(') ? ')' : '}';
	int depth = 0;
	for (size_t i = open_pos; i < text.size(); ++i) {
		if (text[i] == open) {
			++depth;
		}
		else if (text[i] == close && --depth == 0) {
			return i;
		}
	}
	return std::string::npos;
}

}  // namespace

VariableExpander::VariableExpander(const VariableTable& variables, const FunctionRegistry& functions, const IFileLookup& files)
	: variables_(variables), functions_(functions), files_(files) {}

std::string VariableExpander::Expand(const std::string& text) {
	if (expanding_.empty()) cycle_detected_ = false;
	return ExpandText(text);
}

std::string VariableExpander::ExpandText(const std::string& text) {
	std::string out;
	size_t i = 0;
	while (i < text.size()) {
		if (text[i] != '$' || i + 1 >= text.size()) {
			out += text[i++];
			continue;
		}

		char next = text[i + 1];
		if (next == '(' || next == '{') {
			size_t close = FindClosing(text, i + 1);
			if (close == std::string::npos) {  // 괄호가 닫히지 않음: 그대로 둔다
				out.append(text, i, std::string::npos);
				break;
			}
			out += ExpandReference(text.substr(i + 2, close - i - 2));
			i = close + 1;
		}
		else if (next == '$') {
			out += '$';
			i += 2;
		}
		else {
			// 자동 변수($@, $< 등)는 실행 시점에 정해지므로 그대로 둔다
			out.append(text, i, 2);
			i += 2;
		}
	}
	return out;
}

std::string VariableExpander::ExpandReference(const std::string& inner) {
	size_t space = inner.find_first_of(" \t");
	if (space != std::string::npos) {
		std::string name = inner.substr(0, space);
		if (const FunctionSpec* spec = functions_.Find(name)) {
			FunctionArgs args;
			for (const auto& raw : SplitArguments(inner.substr(space + 1), spec->max_args)) {
				args.push_back(strutil::Trim(ExpandText(raw)));
			}
			return functions_.Call(name, args, FunctionContext{ files_ });
		}
	}
	// 변수 이름 자체에 변수가 들어갈 수 있다: $($(prefix)_FLAGS)
	return ExpandVariable(strutil::Trim(ExpandText(inner)));
}

std::string VariableExpander::ExpandVariable(const std::string& name) {
	if (name.empty()) return "";
	if (std::find(expanding_.begin(), expanding_.end(), name) != expanding_.end()) {
		cycle_detected_ = true;
		return "";
	}

	const VariableEntry* entry = variables_.Find(name);
	if (!entry) return "";  // 정의되지 않은 변수는 빈 문자열 (make와 동일)
	if (entry->flavor == Flavor::Simple) return entry->value;

	expanding_.push_back(name);
	std::string value = ExpandText(entry->value);
	expanding_.pop_back();
	return value;
}

// 최상위 쉼표로 인자를 나눈다. 괄호 안의 쉼표는 나누지 않는다.
std::vector<std::string> VariableExpander::SplitArguments(const std::string& text, size_t max_args) {
	std::vector<std::string> parts;
	std::string current;
	int depth = 0;
	for (char c : text) {
		if (c == '(' || c == '{') {
			++depth;
		}
		else if ((c == ')' || c == '}') && depth > 0) {
			--depth;
		}

		if (c == ',' && depth == 0 && parts.size() + 1 < max_args) {
			parts.push_back(current);
			current.clear();
		}
		else {
			current += c;
		}
	}
	parts.push_back(current);
	return parts;
}
