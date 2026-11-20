#pragma once
#include <string>
#include <vector>

// 상태가 없는 문자열 도우미 함수 모음.
// 상태가 없으므로 클래스로 감싸지 않고 네임스페이스로 묶는다.
namespace strutil {

std::string LTrim(const std::string& s);
std::string RTrim(const std::string& s);
std::string Trim(const std::string& s);

std::vector<std::string> SplitSpace(const std::string& s);
std::string Join(const std::vector<std::string>& words, const std::string& delimiter);

size_t CountChar(const std::string& s, char c);
bool StartsWith(const std::string& s, const std::string& prefix);
bool EndsWith(const std::string& s, const std::string& suffix);

// make의 '%' 패턴 매칭. pattern에 '%'가 없으면 완전 일치를 검사한다.
// 일치하면 stem(‘%’에 대응하는 부분)을 채우고 true를 반환한다.
bool MatchPattern(const std::string& pattern, const std::string& word, std::string* stem = nullptr);
// pattern의 첫 번째 '%'를 stem으로 바꾼다. '%'가 없으면 pattern을 그대로 반환한다.
std::string ReplacePercent(const std::string& pattern, const std::string& stem);

}  // namespace strutil
