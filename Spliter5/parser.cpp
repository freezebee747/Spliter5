#include "parser.h"
#include "function.h"

std::unordered_set<std::string> Parser::target_names;

//$(var)들을 각각 나눈다
//예로 "$(var1) $(var2)" 문자열이 입력으로 들어왔다면, $(var1)과 $(var2)로 나누고 그것의 집합을 리턴한다.
//함수의 경우 마지막 값이 입력값이다.
std::vector<std::string> SplitValues(const std::string& target) {
	int count = 0;
	int depth = 0;
	std::vector<std::string> result;
	std::vector<int> loc;

	while (count < target.size()) {
		if (target[count] == '$') {
			count = count + 2;
			depth++;
			loc.push_back(count);
		}
		else if (target[count] == ')') {
			//$( ) 보존
			std::string str = target.substr(loc.back() - 2, count - loc.back() + 3);
			result.push_back(str);
			if (!loc.empty()) {
				loc.pop_back();
			}
			count++;
		}
		else count++;
	}
	return result;
}


std::vector<std::string>ExpendPatternRule(const std::string& pattern, const std::unordered_set<std::string>& filenames) {
	if (SeparatorCounter(pattern, '%') != 1) {
		return std::vector<std::string>();
	}
	size_t sep = pattern.find("%");
	std::string prefix = safe_substr(pattern, 0, sep);
	std::string suffix = pattern.substr(sep + 1, pattern.size() - sep);
	std::vector<std::string> result;

	for (const auto& i : filenames) {
		if (i.substr(0, prefix.size()) == prefix && i.substr(i.size() - suffix.size(), suffix.size()) == suffix) {
			result.push_back(i);
		}
	}
	return result;
}
void variable_expend(std::vector<std::string>& targets, std::unordered_map<std::string, std::string>& variables, ErrorCollector& errors) {
	for (auto& i : targets) {
		std::vector<std::string> vect = SplitValues(i);
		if (IsFunction(i)) {
			if (!vect.empty()) {
				vect.pop_back();
			}
			variable_expend(vect, variables, errors);
			std::string temp = i.substr(2, i.find_first_of(" ") - 2);
			i = Active_function(temp, vect);
		}
		else {
			for (auto& j : vect) {
				if (j.size() >= 4 && j[0] == '$' && j[1] == '(' && j.back() == ')') {
					std::string temp = j.substr(2, j.size() - 3);
					auto it = variables.find(temp);
					if (it != variables.end()) {
						i.replace(i.find(j), j.size(), it->second);
						additional_variable_expend(i, variables, errors);
					}
				}
			}
		}
	}
}

void additional_variable_expend(std::string& value, std::unordered_map<std::string, std::string>& variables, ErrorCollector& errors) {
	if (value.size() >= 4 && value[0] == '$' && value[1] == '(' && value.back() == ')') {
		std::string temp = value.substr(2, value.size() - 3);
		auto it = variables.find(temp);
		if (it != variables.end()) {
			value = it->second;
			additional_variable_expend(value, variables, errors);
		}
	}
}


std::unordered_set<std::string>& Parser::GetTargets()
{
	return target_names;
}

void Parser::parsing(const std::string& filename){
	raw_file = ReadFileWithLineNumbers(filename);
	std::vector<std::pair<unsigned, std::string>> combine_line = JoinSplitLine(raw_file);
	std::vector<Block> blocks = SplitByBlock(combine_line);

	for (const auto& block : blocks) {
		if (block.type == BlockType::variable) {
			Variable var;
			var.line = block.var_line.first;
			int Sep = block.var_line.second.find('=');
			//찾지 못했을 경우

			std::string name = trim(safe_substr(block.var_line.second, 0, Sep - 1));
			std::string value = trim(safe_substr(block.var_line.second, Sep + 1, block.var_line.second.size()));
			var.name = name;
			var.value = value;

			if (block.var_line.second.find(":=") != std::string::npos) {
				var.type = VariableType::Immediate;
			}
			else if (block.var_line.second.find("?=") != std::string::npos) {
				var.type = VariableType::Conditional;
			}
			else if (block.var_line.second.find("+=") != std::string::npos) {
				var.type = VariableType::Append;
			}
			else var.type = VariableType::Simple;
			nodes.push_back(std::make_shared<Variable>(var));
		}
		else if (block.type == BlockType::rule) {
			std::string first_line = block._lines[0].second;
			if (SeparatorCounter(first_line, ':') == 1) {
				size_t colon_pos = first_line.find(':');

				if (trim(safe_substr(first_line, colon_pos + 1, first_line.size() - colon_pos - 1)) == "") {
					Phony_Target pt;
					std::string phony_name = trim(safe_substr(first_line, 0, colon_pos));
					std::vector<std::pair<unsigned, std::string>> phony_recipe;
					for (size_t i = 1; i < block._lines.size(); i++) {
						phony_recipe.push_back({ block._lines[i].first,  trim(block._lines[i].second) });
					}
					pt.line = block._lines[0].first;
					pt.target = phony_name;
					nodes.push_back(std::make_shared<Phony_Target>(pt));
					continue;
				}
				else if (first_line.find("%") != std::string::npos) {
					Pattern_Rule pr;
					pr.line = block._lines[0].first;
					pr.target_pattern = trim(safe_substr(first_line, 0, colon_pos));

					size_t intTemp = (first_line.find(';') != std::string::npos) ? first_line.find(';') : first_line.size();

					pr.prerequisite_pattern = trim(safe_substr(first_line, colon_pos + 1, intTemp - colon_pos - 1));

					if (first_line.find(';') != std::string::npos) {
						std::string rec = safe_substr(first_line, intTemp, first_line.size() - intTemp);
						if (rec != "") {
							pr.recipes.push_back({ block._lines[0].first, rec });
						}
					}

					for (size_t i = 1; i < block._lines.size(); i++) {
						pr.recipes.push_back({ block._lines[i].first, trim(block._lines[i].second) });
					}
					nodes.push_back(std::make_shared<Pattern_Rule>(pr));
					continue;
				}
				else {
					Explicit_Rule ex;
					ex.line = block._lines[0].first;
					ex.target = SplitSpace(trim(safe_substr(first_line, 0, colon_pos)));
					std::vector<std::string> _targets = SplitSpace(trim(safe_substr(first_line, 0, colon_pos)));
					for (const auto& name : _targets) {
						target_names.emplace(name);  
					}
					size_t intTemp = (first_line.find(';') != std::string::npos) ? first_line.find(';') : first_line.size();
					
					ex.prerequisite = SplitSpace(trim(safe_substr(first_line, colon_pos + 1, intTemp - colon_pos - 1)));

					if (first_line.find(';') != std::string::npos) {
						std::string rec = safe_substr(first_line, intTemp, first_line.size() - intTemp);
						if (rec != "") {
							ex.recipes.push_back({ block._lines[0].first, rec });
						}
					}
					for (size_t i = 1; i < block._lines.size(); i++) {
						ex.recipes.push_back({ block._lines[i].first, trim(block._lines[i].second) });
					}
					nodes.push_back(std::make_shared<Explicit_Rule>(ex));
					continue;
				}
			}
			else if (SeparatorCounter(first_line, ':') == 2) {
				Static_Pattern_Rule spr;
				int colon_pos = first_line.find(':');
				spr.target = SplitSpace(trim(safe_substr(first_line, 0, colon_pos)));

				int next_colon = first_line.find(':', colon_pos + 1);

				spr.target_pattern = trim(safe_substr(first_line, colon_pos + 1, next_colon - colon_pos - 1));
				spr.prerequisite_pattern = trim(safe_substr(first_line, next_colon + 1, first_line.size() - next_colon - 1));

				nodes.push_back(std::make_shared<Static_Pattern_Rule>(spr));
			}
	
		}
	}
}

std::vector<Block> Parser::SplitByBlock(std::vector<std::pair<unsigned, std::string>>& file){
	std::vector<Block> blocks;
	bool recipe_flag = false;

	// 백슬래시로 이어지는 라인들을 합치기 위한 코드
	std::vector<std::pair<unsigned, std::string>> combinedLines;
	for (size_t i = 0; i < raw_file.size(); ++i) {
		const auto& [lineNum, lineText] = raw_file[i];
		std::string combined = lineText;
		unsigned originalLine = lineNum;

		while (!combined.empty() && combined.back() == '\\') {
			combined.pop_back();
			if (++i < raw_file.size()) {
				combined += trim(raw_file[i].second);
			}
			else {
				break;
			}
		}
		combinedLines.emplace_back(originalLine, combined);
	}

	std::unique_ptr<Block> block;
	block.reset();

	for (auto it = combinedLines.begin(); it != combinedLines.end(); ++it) {
		const auto& [line, str] = *it;

		//만일 빈 라인이면 일단 넘긴다.
		if (str.empty())continue;
		//변수 처리
		//만일 tab으로 시작하지 않으면서 = 가 포함되어있다면 변수이므로 한 블록으로 처리한다..
		//일단 즉시 확장 변수이던 누적 변수이던 변수로 취급하자
		if (str.find("=") != std::string::npos && str[0] != '\t') {
			block = std::make_unique<Block>();
			block->type = BlockType::variable;
			block->var_line = { line, str };
			blocks.push_back(*block);
			block.reset();
			continue;
		}

		//규칙 처리
		//만일 tab으로 시작하지 않으면서 : 가 포함되어 있다면 규칙이므로 그 블록을 저장한다.
		//다음 문장이 tab으로 시작하지 않을 때 까지 recipe 로 취급한다.
		//tab이 포함된 빈 줄은 블록에 포함하고, tab이 포함되지 않은 빈 줄은 버린다.
		if (str.find(":") != std::string::npos && str[0] != '\t') {
			block = std::make_unique<Block>();
			block->type = BlockType::rule;
			block->_lines.push_back({ line, str });
		}
		if (block && str[0] == '\t') {
			block->_lines.push_back({ line, str });
			//만일 다음 라인이(빈 라인은 첫번째 조건문에서 버려진다.) 탭으로 시작하지 않는다면 block를 저장한 후 클리어한다.
			auto next_it = std::next(it);
			if (next_it != combinedLines.end() && next_it->second[0] != '\t') {
				blocks.push_back(*block);
				block.reset();
			}
			continue;
		}

	}

	if (block) {
		blocks.push_back(*block);
	}
	return blocks;
}

std::vector<std::shared_ptr<ASTNode>> Parser::Getnodes(){
	return nodes;
}

ErrorCollector& Parser::GetError(){
	return ec;
}
