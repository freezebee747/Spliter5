#pragma once
#include "read.h"
#include "AST.h"
#include "error.h"


enum class BlockType { variable, rule, directive, none };

std::vector<std::string> ExpendPatternRule(const std::string& pattern, const std::unordered_set<std::string>& filenames);
std::vector<std::string> SplitValues(const std::string& target);

std::string variable_expend_ex(std::vector<std::string>& variable, std::unordered_map<std::string, std::string>& variables, ErrorCollector& errors);
std::vector<std::string> function_argument_expended(std::vector<std::string>& args, std::unordered_map<std::string, std::string>& variables, ErrorCollector& errors);
std::string additional_variable_expend_ex(std::string& value, std::unordered_map<std::string, std::string>& variables, ErrorCollector& errors);

struct Block {
	BlockType type;
	std::pair<int, std::string> var_line;
	std::vector<std::pair<int, std::string>> _lines;
};

class Parser {
private:
	std::vector<std::shared_ptr<ASTNode>> nodes;
	std::vector<std::pair<unsigned, std::string>> raw_file;
	static std::unordered_set<std::string> target_names;
	std::string default_target = "";
	ErrorCollector ec;
public:
	static std::unordered_set<std::string>& GetTargets();
	std::vector<std::shared_ptr<ASTNode>> Getnodes();
	ErrorCollector& GetError();
	void parsing(const std::string& filename);
	std::vector<Block> SplitByBlock(std::vector<std::pair<unsigned, std::string>>& file);

};