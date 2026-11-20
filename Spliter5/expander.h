#pragma once
#include <string>
#include <vector>

#include "context.h"
#include "function.h"

// $(var), ${var}, $(func args...) 를 확장한다.
// 예전에는 variable_expend_ex / function_argument_expended / additional_variable_expend_ex
// 세 함수에 비슷한 코드가 흩어져 있었는데, 재귀 하강 하나로 합쳤다.
class VariableExpander {
public:
	VariableExpander(const VariableTable& variables, const FunctionRegistry& functions, const IFileLookup& files);

	std::string Expand(const std::string& text);
	// 마지막 Expand 도중 변수 순환 참조를 만났는가
	bool CycleDetected() const { return cycle_detected_; }

private:
	std::string ExpandText(const std::string& text);
	std::string ExpandReference(const std::string& inner);
	std::string ExpandVariable(const std::string& name);
	static std::vector<std::string> SplitArguments(const std::string& text, size_t max_args);

	const VariableTable& variables_;
	const FunctionRegistry& functions_;
	const IFileLookup& files_;
	std::vector<std::string> expanding_;  // 현재 확장 중인 변수 이름 (순환 감지용)
	bool cycle_detected_ = false;
};
