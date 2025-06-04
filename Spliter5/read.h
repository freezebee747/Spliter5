#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_set>

inline const std::unordered_set<std::string> functions = {
	"let", "foreach", "file", "call", "value", "eval",
	"origin", "flaver", "shell", "guile", "patsubst",
	"subst", "filter", "sort", "word", "wordlist", "words",
	"firstword", "lastword", "dir", "notdir", "suffix",
	"basename", "addsuffix", "addprefix", "join", "wildcard",
	"if"
};

int SeparatorCounter(const std::string& target, char sep);
// ���� ���� ����
std::string ltrim(const std::string& s);
// ������ ���� ����
std::string rtrim(const std::string& s);
// ���� ���� ����
std::string trim(const std::string& s);
//��ū ������ ������ ���ڿ��� ���� ��ū ������ ������ ����
int calc_space(const std::string& str, char token, int pos);
std::string safe_substr(const std::string& str, size_t pos, size_t count);
std::vector<std::string> SplitSpace(const std::string& target);
std::vector<std::string> SplitComma(const std::string& target);
bool hasWhitespace(const std::string& str);
std::string join(const std::vector<std::string>& vec, const std::string& delimiter);

bool IsVariable(const std::string& str);
bool IsFunction(const std::string& str);

unsigned VariableCounter(const std::string& var);

std::string ReplaceVariable(std::vector<std::string>& rep, const std::string& target);

std::vector<std::pair<unsigned, std::string>> ReadFileWithLineNumbers(const std::string& filename);
std::vector<std::pair<unsigned, std::string>> JoinSplitLine(std::vector<std::pair<unsigned, std::string>>& raw);

