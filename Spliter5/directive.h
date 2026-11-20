#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "AST.h"

// 지시어 하나를 실행할 때 핸들러에 넘기는 정보.
// 핸들러는 SyntaxChecker를 몰라도 되도록 include 동작을 콜백으로 받는다.
struct DirectiveCall {
	const Directive& node;
	std::vector<std::string> arguments;  // 변수 확장이 끝난 인자
	std::function<void(const std::string& path, bool optional)> include;
};

using DirectiveHandler = std::function<void(const DirectiveCall&)>;

// 지시어 이름 → 핸들러 표.
// 파서는 '이 단어가 지시어인가'를, 검사기는 '지시어를 실행'을 이 객체에 묻는다.
class DirectiveRegistry {
public:
	static DirectiveRegistry WithBuiltins();

	void Register(const std::string& name, DirectiveHandler handler);
	bool IsDirective(const std::string& keyword) const;
	void Run(const DirectiveCall& call) const;

private:
	std::unordered_map<std::string, DirectiveHandler> handlers_;
};
