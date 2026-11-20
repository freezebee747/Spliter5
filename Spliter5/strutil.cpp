#include "strutil.h"

#include <algorithm>
#include <sstream>

namespace strutil {

namespace {
const char* const kWhitespace = " \t\r\n";
}

std::string LTrim(const std::string& s) {
	size_t begin = s.find_first_not_of(kWhitespace);
	return (begin == std::string::npos) ? "" : s.substr(begin);
}

std::string RTrim(const std::string& s) {
	size_t end = s.find_last_not_of(kWhitespace);
	return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string Trim(const std::string& s) {
	return RTrim(LTrim(s));
}

std::vector<std::string> SplitSpace(const std::string& s) {
	std::istringstream iss(s);
	std::vector<std::string> words;
	std::string word;
	while (iss >> word) {
		words.push_back(word);
	}
	return words;
}

std::string Join(const std::vector<std::string>& words, const std::string& delimiter) {
	std::string result;
	for (size_t i = 0; i < words.size(); ++i) {
		if (i != 0) result += delimiter;
		result += words[i];
	}
	return result;
}

size_t CountChar(const std::string& s, char c) {
	return static_cast<size_t>(std::count(s.begin(), s.end(), c));
}

bool StartsWith(const std::string& s, const std::string& prefix) {
	return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

bool EndsWith(const std::string& s, const std::string& suffix) {
	return s.size() >= suffix.size() &&
		s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool MatchPattern(const std::string& pattern, const std::string& word, std::string* stem) {
	size_t percent = pattern.find('%');
	if (percent == std::string::npos) {
		if (stem) stem->clear();
		return pattern == word;
	}
	std::string prefix = pattern.substr(0, percent);
	std::string suffix = pattern.substr(percent + 1);
	if (word.size() < prefix.size() + suffix.size()) return false;
	if (!StartsWith(word, prefix) || !EndsWith(word, suffix)) return false;
	if (stem) *stem = word.substr(prefix.size(), word.size() - prefix.size() - suffix.size());
	return true;
}

std::string ReplacePercent(const std::string& pattern, const std::string& stem) {
	size_t percent = pattern.find('%');
	if (percent == std::string::npos) return pattern;
	std::string result = pattern;
	result.replace(percent, 1, stem);
	return result;
}

}  // namespace strutil
