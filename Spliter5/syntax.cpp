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
            // Pattern Rule ó��
        }
	}
}

void SyntaxChecker::VariableCheck(Variable& var){
	//���� �̸� �˻�
	for (int i = 0; i < var.name.size(); i++) {
		if (!((var.name[i] >= 'a' && var.name[i] <= 'z') ||
			(var.name[i] >= 'A' && var.name[i] <= 'Z') ||
			var.name[i] == '_')) {
			//���� ó��
			ec.AddError("E001", var.line, Severity::Error);
			return;
		}
	}

	//���� �̸� �ߺ� �˻�
	auto finder = variable_map.find(var.value);
	if (finder != variable_map.end()) {
		//���� ó��
		ec.AddError("E002", var.line, Severity::Error);
		return;
	}

	//���� ��ȯ �˻�
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

		//��� Ȯ�� ���� ó��
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
	//target üũ

	//prerequisite üũ

	//recipe üũ
}

