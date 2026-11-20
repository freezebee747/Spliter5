#include "read.h"

#include <fstream>
#include <sstream>

#include "strutil.h"

namespace makefile_reader {

namespace {

bool IsRecipeLine(const std::string& text) {
	return !text.empty() && text[0] == '\t';
}

// recipe 줄의 '#'은 셸에 그대로 넘어가므로 지우지 않는다.
std::string StripComment(const std::string& text) {
	if (IsRecipeLine(text)) return text;
	size_t hash = text.find('#');
	return (hash == std::string::npos) ? text : text.substr(0, hash);
}

std::vector<Line> ReadLines(std::istream& in) {
	std::vector<Line> lines;
	std::string text;
	unsigned number = 1;
	while (std::getline(in, text)) {
		if (!text.empty() && text.back() == '\r') text.pop_back();  // CRLF 파일 대응
		lines.push_back({ number++, text });
	}
	return lines;
}

}  // namespace

std::vector<Line> ReadPhysicalLines(const std::string& path, bool* ok) {
	std::ifstream in(path);
	if (ok) *ok = in.is_open();
	if (!in.is_open()) return {};
	return ReadLines(in);
}

std::vector<Line> SplitPhysicalLines(const std::string& text) {
	std::istringstream in(text);
	return ReadLines(in);
}

std::vector<Line> ToLogicalLines(const std::vector<Line>& physical) {
	std::vector<Line> logical;
	for (size_t i = 0; i < physical.size(); ++i) {
		Line current{ physical[i].number, StripComment(physical[i].text) };

		while (!current.text.empty() && current.text.back() == '\\') {
			current.text.pop_back();
			if (i + 1 >= physical.size()) break;
			++i;
			current.text = strutil::RTrim(current.text) + " " + strutil::LTrim(StripComment(physical[i].text));
		}
		logical.push_back(current);
	}
	return logical;
}

}  // namespace makefile_reader
