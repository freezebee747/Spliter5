#pragma once
#include "read.h"
#include "AST.h"
#include "error.h"

enum class BlockType { variable, rule, none };

std::vector<std::string> ExpendPatternRule(const std::string& pattern, const std::unordered_set<std::string>& filenames);
std::vector<std::string> SplitValues(const std::string& target);
void variable_expend(std::vector<std::string>& targets, std::unordered_map<std::string, std::string>& variables, ErrorCollector& errors);
void additional_variable_expend(std::string& value, std::unordered_map<std::string, std::string>& variables, ErrorCollector& errors);

struct Block {
	BlockType type;
	std::pair<int, std::string> var_line;
	std::vector<std::pair<int, std::string>> _lines;
};

class Parser {
private:
	std::vector<std::shared_ptr<ASTNode>> nodes;
	std::vector<std::pair<unsigned, std::string>> raw_file;
	ErrorCollector ec;
public:
	void parsing(const std::string& filename);
	std::vector<Block> SplitByBlock(std::vector<std::pair<unsigned, std::string>>& file);
	std::vector<std::shared_ptr<ASTNode>> Getnodes();
};