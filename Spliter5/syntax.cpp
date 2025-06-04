#include "syntax.h"

std::unordered_map<std::string, std::string> SyntaxChecker::variable_map;

SyntaxChecker::SyntaxChecker(std::vector<std::shared_ptr<ASTNode>>& node, ErrorCollector& _ec) {
	nodes = node;
    ec = _ec;
}

const std::unordered_map<std::string, std::string>& SyntaxChecker::GetVariables(){
	return variable_map;
}

void SyntaxChecker::SyntaxCheck(){
	for (const auto& node : nodes) {
        if (auto var = dynamic_cast<Variable*>(node.get())) {
            VariableCheck(*var);
        }
        else if (auto ex = dynamic_cast<Explicit_Rule*>(node.get())) {
            ExplicitRuleCheck(*ex);
        }
        else if (auto pattern = dynamic_cast<Pattern_Rule*>(node.get())) {
            // Pattern Rule 처리
        }
	}
}

void SyntaxChecker::VariableCheck(Variable& var){
	//변수 이름 검사
	for (int i = 0; i < var.name.size(); i++) {
		if (!((var.name[i] >= 'a' && var.name[i] <= 'z') ||
			(var.name[i] >= 'A' && var.name[i] <= 'Z') ||
			var.name[i] == '_')) {
			//에러 처리
			ec.AddError("E001", var.line, Severity::Error);
			return;
		}
	}

	//변수 이름 중복 검사
	auto finder = variable_map.find(var.value);
	if (finder != variable_map.end()) {
		//에러 처리
		ec.AddError("E002", var.line, Severity::Error);
		return;
	}

	//변수 순환 검사
	{
		std::vector<std::string> name_stack;
		name_stack.push_back(var.name);
		std::string checker = var.value;

		variable_map.emplace(var.name, var.value);

		while (true) {
			if (!(checker.size() >= 4 && checker[0] == '$' && checker[1] == '(' && checker.back() == ')')) break;
			checker = checker.substr(2, checker.size() - 3);
			auto finder = variable_map.find(checker);
			if (finder != variable_map.end()) {
				std::string tester = finder->second.substr(2, finder->second.size() - 3);
				auto next = std::find(name_stack.begin(), name_stack.end(), tester);
				if (next != name_stack.end()) {
					ec.AddError("E003", var.line, Severity::Error);
					variable_map.erase(var.name);
					return;
				}
				name_stack.push_back(checker);
				checker = finder->second;
			}
		}

		//즉시 확장 변수 처리
		if (var.type == VariableType::Immediate) {
			variable_map.erase(var.name);
			std::vector<std::string> temp;
			temp.push_back(var.name);
			variable_expend(temp, variable_map, ec);
			std::string value = temp[0];
			variable_map.emplace(var.name, value);
		}
	}
}

void SyntaxChecker::ExplicitRuleCheck(Explicit_Rule& ex){
	//target 검사
	for (const auto& target : ex.target) {
		//1.변수 검사
		//먼저 변수인지 검사
		unsigned var_count = VariableCounter(target);
		if (var_count == 1) {
			//1.1 변수가 하나일 때
			//즉시 확장후 target_set에 저장
			std::vector<std::string> temp;
			temp.push_back(target);
			variable_expend(temp, variable_map, ec);
			target_map.insert(temp[0]);
			return;
		}
		else if (var_count > 1) {
			std::string temp = target;
			std::vector<std::string> variables;
			variables = SplitValues(target);
			variable_expend(variables, variable_map, ec);
			temp = ReplaceVariable(variables, temp);
			target_map.insert(temp);
			return;
		}


		//2. 와일드카드가 사용되었는가?
		//사용된 경우 에러로 처리한다. 와일드카드는 target에서 사용될 수 없다.
		size_t finder = target.find('*');
		if (finder != std::string::npos) {
			ec.AddError("E101", ex.line, Severity::Error);
			return;
		}

		//3. 타겟 이름이 올바른가?
		finder = target.find_first_of("$=");
		if (finder != std::string::npos) {
			//target에 $, = 가 쓰였으면 주의를 줌(에러가 아님)
			ec.AddError("E102", ex.line, Severity::Warning);
		}

		//4. target이름이 중복되었는가?
		if (target_map.find(target) != target_map.end()) {
			//중복사용된 경우 에러로 처리한다.
			ec.AddError("E103", ex.line, Severity::Error);
		}
	}
}

