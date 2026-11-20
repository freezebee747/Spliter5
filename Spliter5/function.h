#pragma once
#include <cstddef>
#include <functional>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include "file.h"

// 함수가 실행될 때 필요한 외부 정보. 전역 대신 이 객체로 전달한다.
struct FunctionContext {
	const IFileLookup& files;
};

using FunctionArgs = std::vector<std::string>;
using FunctionHandler = std::function<std::string(const FunctionArgs&, const FunctionContext&)>;

struct FunctionSpec {
	static constexpr size_t kVariadic = std::numeric_limits<size_t>::max();

	size_t min_args = 0;
	// 이 개수를 넘는 쉼표는 마지막 인자의 일부로 본다. 예: $(words a,b c) → 인자 1개 "a,b c"
	size_t max_args = kVariadic;
	FunctionHandler handler;  // 비어 있으면 '인식은 하지만 아직 구현하지 않은' 함수
};

// make 내장 함수 이름 → 구현 표.
// 새 함수는 Register 한 번으로 추가된다(개방-폐쇄 원칙).
class FunctionRegistry {
public:
	static FunctionRegistry WithBuiltins();

	void Register(const std::string& name, size_t min_args, size_t max_args, FunctionHandler handler);
	void Reserve(const std::string& name);

	const FunctionSpec* Find(const std::string& name) const;
	std::string Call(const std::string& name, const FunctionArgs& args, const FunctionContext& context) const;

private:
	std::unordered_map<std::string, FunctionSpec> functions_;
};

// 내장 함수 구현. 순수 함수라 단위 테스트하기 쉽다.
namespace builtin {
std::string Subst(const std::string& from, const std::string& to, const std::string& text);
std::string Patsubst(const std::string& pattern, const std::string& replacement, const std::string& text);
std::string Strip(const std::string& text);
std::string Findstring(const std::string& find, const std::string& in);
std::string Filter(const std::string& patterns, const std::string& text);
std::string Sort(const std::string& list);
std::string Word(const std::string& n, const std::string& text);
std::string Wordlist(const std::string& start, const std::string& end, const std::string& text);
std::string Words(const std::string& text);
std::string Firstword(const std::string& names);
std::string Lastword(const std::string& names);
std::string Dir(const std::string& names);
std::string Notdir(const std::string& names);
std::string Suffix(const std::string& names);
std::string Basename(const std::string& names);
std::string Addsuffix(const std::string& suffix, const std::string& names);
std::string Addprefix(const std::string& prefix, const std::string& names);
std::string Join(const std::string& list1, const std::string& list2);
std::string Wildcard(const std::string& patterns, const IFileLookup& files);
std::string If(const std::string& condition, const std::string& then_part, const std::string& else_part);
std::string Or(const FunctionArgs& conditions);
}  // namespace builtin
