#include "syntax.h"

std::unordered_map<std::string, std::string> SyntaxChecker::variable_map;

SyntaxChecker::SyntaxChecker(std::vector<std::shared_ptr<ASTNode>> node, ErrorCollector& _ec) {
	nodes = node;
    ec = _ec;
	fm.SaveFilename();
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
			PatternRuleCheck(*pattern);
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
			ec.AddError("E011", var.line, Severity::Error);
			return;
		}
	}

	//변수 이름 중복 검사
	auto finder = variable_map.find(var.name);
	if (finder != variable_map.end()) {
		//에러 처리
		ec.AddError("E012", var.line, Severity::Error);
		return;
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
					ec.AddError("E013", var.line, Severity::Error);
					variable_map.erase(var.name);
					return;
				}
				name_stack.push_back(checker);
				checker = finder->second;
			}
		}

	}
}

void SyntaxChecker::ExplicitRuleCheck(Explicit_Rule& ex){

	auto variable_check_lambda = [&](std::string var) -> bool {
		//1.변수 검사
		//먼저 변수인지 검사
		unsigned var_count = VariableCounter(var);
		if (var_count == 1) {
			//1.1 변수가 하나일 때
			//즉시 확장후 target_set에 저장
			std::vector<std::string> temp;
			temp.push_back(var);
			variable_expend(temp, variable_map, ec);
			target_map.insert(temp[0]);
			return true;
		}
		else if (var_count > 1) {
			std::string temp = var;
			std::vector<std::string> variables;
			variables = SplitValues(var);
			variable_expend(variables, variable_map, ec);
			temp = ReplaceVariable(variables, temp);
			target_map.insert(temp);
			return true;
		}
	};
	Explicit_ex e_x;
	std::vector<Explicit_ex> ex_checker;

	for (auto& i : ex.target) {
		//target은 즉시확장

		unsigned var_count = VariableCounter(i);
		if (var_count == 1) {
			//1.1 변수가 하나일 때
			std::vector<std::string> temp;
			temp.push_back(i);
			variable_expend(temp, variable_map, ec);
			ex.target = SplitSpace(temp[0]);
		}
		else if (var_count > 1) {
			std::string temp = i;
			std::vector<std::string> variables;
			variables = SplitValues(i);
			variable_expend(variables, variable_map, ec);
			temp = ReplaceVariable(variables, temp);
		}
	}

	for (auto& i : ex.prerequisite) {
		//prerequisite는 즉시 확장
		unsigned var_count = VariableCounter(i);
		if (var_count == 1) {
			//1.1 변수가 하나일 때
			std::vector<std::string> temp;
			temp.push_back(i);
			variable_expend(temp, variable_map, ec);
			ex.prerequisite = SplitSpace(temp[0]);
		}
		else if (var_count > 1) {
			std::string temp = i;
			std::vector<std::string> variables;
			variables = SplitValues(i);
			variable_expend(variables, variable_map, ec);
			temp = ReplaceVariable(variables, temp);
		}
	}

	if (ex.target.size() > 1) {
		for (const auto& target : ex.target) {
			e_x.line = ex.line;
			e_x.target = target;
			e_x.prerequisite = ex.prerequisite;
			e_x.recipes = ex.recipes;
			ex_checker.push_back(e_x);
		}
	}
	else if (ex.target.size() == 1) {
		e_x.line = ex.line;
		e_x.target = ex.target[0];
		e_x.prerequisite = ex.prerequisite;
		e_x.recipes = ex.recipes;
		ex_checker.push_back(e_x);
	}

	for (const auto& ex_rule : ex_checker) {

		size_t finder = ex_rule.target.find('*');
		if (finder != std::string::npos) {
			ec.AddError("E101", ex.line, Severity::Error);
			continue;
		}

		finder = ex_rule.target.find_first_of("$=");
		if (finder != std::string::npos) {
			//target에 $, = 가 쓰였으면 주의를 줌(에러가 아님)
			ec.AddError("E102", ex_rule.line, Severity::Warning);
		}

		//4. target이름이 중복되었는가?
		// 중복 검사 시 확장된 이름 기준
		if (target_map.find(ex_rule.target) != target_map.end()) {
			ec.AddError("E103", ex.line, Severity::Error);
		}
		else {
			target_map.insert(ex_rule.target);
		}


		for (auto& preq : ex_rule.prerequisite) {
			std::unordered_set<std::string>& targets = Parser::GetTargets();
			
			//2. 와일드카드가 사용되었는가?
			size_t finder = preq.find('*');
			bool matched = false;
			std::regex rx(glob_to_regex(preq));

			for (const auto& t : targets) {
				if (std::regex_match(t, rx)) {
					matched = true;
					break;
				}
			}
			if (!matched) {
				// 파일 시스템에서 검색
				std::unordered_set<std::string> files = fm.SearchFilenames();
				for (const auto& f : files) {
					if (std::regex_match(f, rx)) {
						matched = true;
						break;
					}
				}
			}
			if (!matched) {
				ec.AddError("E151", ex.line, Severity::Error);  // 아무것도 매칭 안 되면 진짜로 에러
			}

		}

		//레시피 검사
	}
}

void SyntaxChecker::PatternRuleCheck(Pattern_Rule& pr){
	Explicit_Rule ex;
	ex.line = pr.line;
	ex.target = ExpendPatternRule(pr.target_pattern, fm.SearchFilenames());
	if (pr.prerequisite_pattern.find('%') != std::string::npos) {
		ex.prerequisite = ExpendPatternRule(pr.prerequisite_pattern, fm.SearchFilenames());
	}
	ex.recipes = pr.recipes;
	ExplicitRuleCheck(ex);
}

void SyntaxChecker::StaticPatternRuleCheck(Static_Pattern_Rule& spr){

	auto checker = [&](std::vector<std::string>& targets, std::vector<std::string>& result,  std::string& tp) -> bool{
		if (SeparatorCounter(tp, '%') != 1) {
			return false;
		}
		size_t sep = tp.find("%");
		std::string prefix = safe_substr(tp, 0, sep);
		std::string suffix = tp.substr(sep + 1, tp.size() - sep);

		for (const auto& t : targets) {
			std::string test_prefix = safe_substr(t, 0, prefix.size());
			std::string test_suffix = safe_substr(t, t.size() - suffix.size(), suffix.size());

			if (test_prefix != prefix) {
				return false;
			}
			if (test_suffix != suffix) {
				return false;
			}

			result.push_back(t);
		}
		return true;
	};

	//target 검사
	std::vector<std::string> targets;
	for (const auto& target : spr.target) {
		unsigned var_count = VariableCounter(target);
		if (var_count == 1) {
			std::vector<std::string> temp;
			temp.push_back(target);
			variable_expend(temp, variable_map, ec);
			targets = SplitSpace(temp[0]);
		}
		else if (var_count > 1) {
			std::string temp = target;
			std::vector<std::string> variables;
			variables = SplitValues(target);
			variable_expend(variables, variable_map, ec);
			temp = ReplaceVariable(variables, temp);
			targets = SplitSpace(temp);
		}
	}
	std::vector<std::string> TargetResult;

	if (!checker(targets, TargetResult, spr.target_pattern)) {
		//error
		return;
	}

	std::vector<std::string> PreqResult;
	if (!checker(targets, PreqResult, spr.prerequisite_pattern)) {
		//error
		return;
	}
	Explicit_Rule ex;
	ex.line = spr.line;
	ex.target = TargetResult;
	ex.prerequisite = PreqResult;
	ex.recipes = spr.recipes;

	ExplicitRuleCheck(ex);

}

void SyntaxChecker::PrintErrors(){
	std::vector<Error> err = ec.GetAll();
	std::string messages;
	for (const auto& i : err) {
		messages = ErrorMessage::GetInstance().GetMessage(i.code);
		std::cout << i.line_number << ". [" << i.code << "] : " << messages << "\n";
	}

}

