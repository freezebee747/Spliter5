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
		else if (auto dir = dynamic_cast<Directive*>(node.get())) {
			DirectiveChecker(*dir);
		}
        else if (auto ex = dynamic_cast<Explicit_Rule*>(node.get())) {
            ExplicitRuleCheck(*ex);
        }
        else if (auto pattern = dynamic_cast<Pattern_Rule*>(node.get())) {
			PatternRuleCheck(*pattern);
        }
		else if (auto spattern = dynamic_cast<Static_Pattern_Rule*>(node.get())) {
			StaticPatternRuleCheck(*spattern);
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
		std::string value = variable_expend_ex(temp, variable_map, ec);
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

void SyntaxChecker::DirectiveChecker(Directive& dir){
	//나중에 정의
}

void SyntaxChecker::ExplicitRuleCheck(Explicit_Rule& ex){
	Explicit_ex e_x;
	std::vector<Explicit_ex> ex_checker;
	std::vector<std::string> variables_temp;

	for (auto& i : ex.target) {
		//target은 즉시확장
		std::vector<std::string> temp;
		std::string temp_string;
		temp.push_back(i);
		temp_string = variable_expend_ex(temp, variable_map, ec);
		temp = SplitSpace(trim(temp_string));
		variables_temp.insert(variables_temp.end(), temp.begin(), temp.end());
	}
	ex.target = variables_temp;

	variables_temp.clear();
	for (auto& i : ex.prerequisite) {
		//prerequisite는 즉시 확장
		std::vector<std::string> temp;
		std::string temp_string;
		temp.push_back(i);
		temp_string = variable_expend_ex(temp, variable_map, ec);
		temp = SplitSpace(trim(temp_string));
		variables_temp.insert(variables_temp.end(), temp.begin(), temp.end());
	}
	ex.prerequisite = variables_temp;

	variables_temp.clear();

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

void SyntaxChecker::PatternRuleCheck(Pattern_Rule& pr) {
	std::vector<std::string> prerequisites;
	if (pr.prerequisite_pattern.find("%") != std::string::npos) {
		prerequisites = ExpendPatternRule(pr.prerequisite_pattern, fm.SearchFilenames());
	}
	if (pr.target_pattern.find("%") == std::string::npos) {
		ec.AddError("E201", pr.line, Severity::Error);
		return;
	}
	if (SeparatorCounter(pr.target_pattern, '%') != 1) {
		ec.AddError("E202", pr.line, Severity::Error);
		return;
	}

	size_t point = pr.prerequisite_pattern.find("%");
	std::string prefix = pr.prerequisite_pattern.substr(0, point);
	std::string suffix = pr.prerequisite_pattern.substr(point+1, pr.prerequisite_pattern.size() - point);
	for (const auto& preq : prerequisites) {

		std::string target = pr.target_pattern;
		Explicit_Rule ex;
		std::string raw_preq = preq.substr(prefix.size(), preq.find(suffix) - prefix.size());
		target.replace(target.find("%"), 1, raw_preq);
		ex.target.push_back(target);
		ex.prerequisite.push_back(preq);
		ex.recipes = pr.recipes;
		ExplicitRuleCheck(ex);
	}
}

void SyntaxChecker::StaticPatternRuleCheck(Static_Pattern_Rule& spr){
	std::vector<std::string> targets;
	for (const auto& target : spr.target) {
		if (target[0] == '$' && target[1] == '(' && target[target.size()] == ')') {
			std::vector<std::string> variable;
			variable.push_back(target);
			std::string expend = variable_expend_ex(variable, variable_map, ec);
			variable = SplitSpace(trim(expend));
			targets.insert(targets.end(), variable.begin(), variable.end());
		}
		else targets.push_back(target);
	}
	if (spr.target_pattern.find("%") == std::string::npos) {
		ec.AddError("E201", spr.line, Severity::Error);
		return;
	}
	if (SeparatorCounter(spr.prerequisite_pattern, '%') != 1) {
		return;
	}
	size_t point = spr.target_pattern.find("%");
	std::string prefix = spr.target_pattern.substr(0, point);
	std::string suffix = spr.target_pattern.substr(point + 1, spr.target_pattern.size() - point);
	for (const auto& t : targets) {
		std::string preq = spr.prerequisite_pattern;
		Explicit_Rule ex;
		std::string raw_target = t.substr(prefix.size(), t.find(suffix) - prefix.size());
		ex.target.push_back(t);
		preq.replace(preq.find('%'), 1, raw_target);
		ex.prerequisite.push_back(preq);
		ex.recipes = spr.recipes;
		ExplicitRuleCheck(ex);
	}
}

void SyntaxChecker::IncludeOtherSyntax(SyntaxChecker& syn, const std::string& filename){
	std::unordered_set<std::string> outer_target;
	target_map.insert(std::move(outer_target.begin()), std::move(outer_target.end()));
	syn.ec.SetExternalErrors(filename);
	ec.AppendErrorCollector(syn.GetErrorCollector());
}

void SyntaxChecker::IncludeOtherSyntax(SyntaxChecker& syn){
	std::unordered_set<std::string> outer_target;
	target_map.insert(std::move(outer_target.begin()), std::move(outer_target.end()));
	ec.AppendErrorCollector(syn.GetErrorCollector());
}

ErrorCollector& SyntaxChecker::GetErrorCollector(){
	return ec;
}

void SyntaxChecker::PrintErrors(){
	std::vector<Error> err = ec.GetAll();
	std::string messages;
	for (const auto& i : err) {
		messages = ErrorMessage::GetInstance().GetMessage(i.code);
		std::cout << i.line_number << ". [" << i.code << "] : " << messages << "\n";
	}
}

