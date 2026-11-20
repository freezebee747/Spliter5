#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "file.h"

enum class Flavor {
	Recursive,  // = : 사용할 때마다 확장
	Simple      // := : 정의할 때 한 번 확장
};

struct VariableEntry {
	std::string value;
	Flavor flavor = Flavor::Recursive;
};

class VariableTable {
public:
	const VariableEntry* Find(const std::string& name) const;
	bool Contains(const std::string& name) const { return Find(name) != nullptr; }
	void Set(const std::string& name, VariableEntry entry);
	void Erase(const std::string& name);

private:
	std::unordered_map<std::string, VariableEntry> table_;
};

// 하나의 Makefile(및 그것이 include한 파일들)을 분석하는 동안 공유되는 상태.
// 예전에는 Parser::target_names, SyntaxChecker::variable_map 같은 static 멤버였다.
// 이제는 분석 한 번마다 객체를 만들어 필요한 곳에 주입한다.
struct MakefileContext {
	explicit MakefileContext(const IFileLookup& file_lookup) : files(file_lookup) {}

	VariableTable variables;
	std::unordered_set<std::string> declared_targets;  // 파서가 본 타깃 이름(전방 참조 허용용)
	std::unordered_set<std::string> defined_targets;   // 검사기가 확장해서 등록한 타깃(중복 검사용)
	const IFileLookup& files;
};
