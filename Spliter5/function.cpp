#include "function.h"

#include <algorithm>

#include "strutil.h"

using strutil::SplitSpace;

namespace {

std::string JoinWords(const std::vector<std::string>& words) {
	return strutil::Join(words, " ");
}

// 단어마다 f를 적용하고, 빈 결과는 버린 뒤 공백으로 이어 붙인다.
template <typename F>
std::string MapWords(const std::string& text, F f) {
	std::vector<std::string> result;
	for (const auto& word : SplitSpace(text)) {
		std::string mapped = f(word);
		if (!mapped.empty()) result.push_back(mapped);
	}
	return JoinWords(result);
}

bool ParseNumber(const std::string& text, long& out) {
	std::string trimmed = strutil::Trim(text);
	try {
		size_t used = 0;
		out = std::stol(trimmed, &used);
		return used == trimmed.size();
	}
	catch (const std::exception&) {
		return false;
	}
}

// 마지막 '/' 뒤에 있는 마지막 '.'의 위치. 없으면 npos.
size_t SuffixPosition(const std::string& word) {
	size_t dot = word.rfind('.');
	size_t slash = word.rfind('/');
	if (dot == std::string::npos) return std::string::npos;
	if (slash != std::string::npos && dot < slash) return std::string::npos;
	return dot;
}

// 인자 개수가 고정된 순수 함수를 FunctionHandler로 감싼다.
using Fn1 = std::string (*)(const std::string&);
using Fn2 = std::string (*)(const std::string&, const std::string&);
using Fn3 = std::string (*)(const std::string&, const std::string&, const std::string&);

FunctionHandler Wrap(Fn1 f) {
	return [f](const FunctionArgs& a, const FunctionContext&) { return f(a[0]); };
}
FunctionHandler Wrap(Fn2 f) {
	return [f](const FunctionArgs& a, const FunctionContext&) { return f(a[0], a[1]); };
}
FunctionHandler Wrap(Fn3 f) {
	return [f](const FunctionArgs& a, const FunctionContext&) { return f(a[0], a[1], a[2]); };
}

}  // namespace

FunctionRegistry FunctionRegistry::WithBuiltins() {
	FunctionRegistry r;
	r.Register("subst", 3, 3, Wrap(builtin::Subst));
	r.Register("patsubst", 3, 3, Wrap(builtin::Patsubst));
	r.Register("strip", 1, 1, Wrap(builtin::Strip));
	r.Register("findstring", 2, 2, Wrap(builtin::Findstring));
	r.Register("filter", 2, 2, Wrap(builtin::Filter));
	r.Register("sort", 1, 1, Wrap(builtin::Sort));
	r.Register("word", 2, 2, Wrap(builtin::Word));
	r.Register("wordlist", 3, 3, Wrap(builtin::Wordlist));
	r.Register("words", 1, 1, Wrap(builtin::Words));
	r.Register("firstword", 1, 1, Wrap(builtin::Firstword));
	r.Register("lastword", 1, 1, Wrap(builtin::Lastword));
	r.Register("dir", 1, 1, Wrap(builtin::Dir));
	r.Register("notdir", 1, 1, Wrap(builtin::Notdir));
	r.Register("suffix", 1, 1, Wrap(builtin::Suffix));
	r.Register("basename", 1, 1, Wrap(builtin::Basename));
	r.Register("addsuffix", 2, 2, Wrap(builtin::Addsuffix));
	r.Register("addprefix", 2, 2, Wrap(builtin::Addprefix));
	r.Register("join", 2, 2, Wrap(builtin::Join));
	r.Register("wildcard", 1, 1, [](const FunctionArgs& a, const FunctionContext& ctx) {
		return builtin::Wildcard(a[0], ctx.files);
	});
	r.Register("if", 2, 3, [](const FunctionArgs& a, const FunctionContext&) {
		return builtin::If(a[0], a[1], a.size() > 2 ? a[2] : "");
	});
	r.Register("or", 1, FunctionSpec::kVariadic, [](const FunctionArgs& a, const FunctionContext&) {
		return builtin::Or(a);
	});

	// 함수로 인식은 하지만 아직 구현하지 않은 것들 (결과는 빈 문자열)
	for (const char* name : { "let", "foreach", "file", "call", "value", "eval", "origin",
							  "flavor", "shell", "guile", "and", "intcmp", "error", "warning", "info" }) {
		r.Reserve(name);
	}
	return r;
}

void FunctionRegistry::Register(const std::string& name, size_t min_args, size_t max_args, FunctionHandler handler) {
	functions_[name] = FunctionSpec{ min_args, max_args, std::move(handler) };
}

void FunctionRegistry::Reserve(const std::string& name) {
	functions_[name] = FunctionSpec{};
}

const FunctionSpec* FunctionRegistry::Find(const std::string& name) const {
	auto it = functions_.find(name);
	return (it != functions_.end()) ? &it->second : nullptr;
}

std::string FunctionRegistry::Call(const std::string& name, const FunctionArgs& args, const FunctionContext& context) const {
	const FunctionSpec* spec = Find(name);
	if (!spec || !spec->handler || args.size() < spec->min_args) return "";
	return spec->handler(args, context);
}

namespace builtin {

std::string Subst(const std::string& from, const std::string& to, const std::string& text) {
	if (from.empty()) return text;
	std::string result;
	size_t pos = 0;
	size_t found;
	while ((found = text.find(from, pos)) != std::string::npos) {
		result.append(text, pos, found - pos);
		result += to;
		pos = found + from.size();
	}
	result.append(text, pos, std::string::npos);
	return result;
}

std::string Patsubst(const std::string& pattern, const std::string& replacement, const std::string& text) {
	return MapWords(text, [&](const std::string& word) {
		std::string stem;
		if (strutil::MatchPattern(pattern, word, &stem)) return strutil::ReplacePercent(replacement, stem);
		return word;
	});
}

std::string Strip(const std::string& text) {
	return JoinWords(SplitSpace(text));
}

std::string Findstring(const std::string& find, const std::string& in) {
	return (in.find(find) != std::string::npos) ? find : "";
}

std::string Filter(const std::string& patterns, const std::string& text) {
	std::vector<std::string> pattern_list = SplitSpace(patterns);
	return MapWords(text, [&](const std::string& word) {
		for (const auto& pattern : pattern_list) {
			if (strutil::MatchPattern(pattern, word)) return word;
		}
		return std::string();
	});
}

std::string Sort(const std::string& list) {
	std::vector<std::string> words = SplitSpace(list);
	std::sort(words.begin(), words.end());
	words.erase(std::unique(words.begin(), words.end()), words.end());  // make의 sort는 중복도 제거한다
	return JoinWords(words);
}

std::string Word(const std::string& n, const std::string& text) {
	long index = 0;
	std::vector<std::string> words = SplitSpace(text);
	if (!ParseNumber(n, index) || index < 1 || index > static_cast<long>(words.size())) return "";
	return words[index - 1];
}

std::string Wordlist(const std::string& start, const std::string& end, const std::string& text) {
	long first = 0;
	long last = 0;
	if (!ParseNumber(start, first) || !ParseNumber(end, last) || first < 1) return "";
	std::vector<std::string> words = SplitSpace(text);
	last = std::min(last, static_cast<long>(words.size()));
	if (first > last) return "";
	return JoinWords(std::vector<std::string>(words.begin() + (first - 1), words.begin() + last));
}

std::string Words(const std::string& text) {
	return std::to_string(SplitSpace(text).size());
}

std::string Firstword(const std::string& names) {
	std::vector<std::string> words = SplitSpace(names);
	return words.empty() ? "" : words.front();
}

std::string Lastword(const std::string& names) {
	std::vector<std::string> words = SplitSpace(names);
	return words.empty() ? "" : words.back();
}

std::string Dir(const std::string& names) {
	return MapWords(names, [](const std::string& word) {
		size_t slash = word.rfind('/');
		return (slash == std::string::npos) ? std::string("./") : word.substr(0, slash + 1);
	});
}

std::string Notdir(const std::string& names) {
	return MapWords(names, [](const std::string& word) {
		size_t slash = word.rfind('/');
		return (slash == std::string::npos) ? word : word.substr(slash + 1);
	});
}

std::string Suffix(const std::string& names) {
	return MapWords(names, [](const std::string& word) {
		size_t dot = SuffixPosition(word);
		return (dot == std::string::npos) ? std::string() : word.substr(dot);
	});
}

std::string Basename(const std::string& names) {
	return MapWords(names, [](const std::string& word) {
		size_t dot = SuffixPosition(word);
		return (dot == std::string::npos) ? word : word.substr(0, dot);
	});
}

std::string Addsuffix(const std::string& suffix, const std::string& names) {
	return MapWords(names, [&](const std::string& word) { return word + suffix; });
}

std::string Addprefix(const std::string& prefix, const std::string& names) {
	return MapWords(names, [&](const std::string& word) { return prefix + word; });
}

std::string Join(const std::string& list1, const std::string& list2) {
	std::vector<std::string> first = SplitSpace(list1);
	std::vector<std::string> second = SplitSpace(list2);
	std::vector<std::string> result;
	for (size_t i = 0; i < std::max(first.size(), second.size()); ++i) {
		std::string joined;
		if (i < first.size()) joined += first[i];
		if (i < second.size()) joined += second[i];
		result.push_back(joined);
	}
	return JoinWords(result);
}

std::string Wildcard(const std::string& patterns, const IFileLookup& files) {
	std::vector<std::string> result;
	for (const auto& pattern : SplitSpace(patterns)) {
		std::vector<std::string> matched = files.Glob(pattern);
		result.insert(result.end(), matched.begin(), matched.end());
	}
	return JoinWords(result);
}

std::string If(const std::string& condition, const std::string& then_part, const std::string& else_part) {
	return strutil::Trim(condition).empty() ? else_part : then_part;
}

std::string Or(const FunctionArgs& conditions) {
	for (const auto& condition : conditions) {
		if (!strutil::Trim(condition).empty()) return condition;
	}
	return "";
}

}  // namespace builtin
