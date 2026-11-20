#pragma once
#include <memory>
#include <string>
#include <vector>

// 파일의 한 줄. 줄 번호와 내용을 함께 들고 다닌다.
struct Line {
	unsigned number = 0;
	std::string text;
};

enum class VariableType {
	Simple,      // =
	Immediate,   // :=
	Append,      // +=
	Conditional  // ?=
};

struct Directive;
struct Variable;
struct ExplicitRule;
struct PatternRule;
struct StaticPatternRule;
struct PhonyTarget;

// Visitor 패턴: 노드 종류별 처리를 dynamic_cast 분기 대신 이중 디스패치로 해결한다.
// 새 노드를 추가하면 여기에 Visit을 추가하는 순간 컴파일러가 구현이 빠진 곳을 알려준다.
class ASTVisitor {
public:
	virtual ~ASTVisitor() = default;
	virtual void Visit(const Directive& node) = 0;
	virtual void Visit(const Variable& node) = 0;
	virtual void Visit(const ExplicitRule& node) = 0;
	virtual void Visit(const PatternRule& node) = 0;
	virtual void Visit(const StaticPatternRule& node) = 0;
	virtual void Visit(const PhonyTarget& node) = 0;
};

// 모든 노드가 공통으로 갖는 필드(line)는 기반 클래스로 올린다.
struct ASTNode {
	unsigned line = 0;
	virtual ~ASTNode() = default;
	virtual void Accept(ASTVisitor& visitor) const = 0;
};

using NodeList = std::vector<std::unique_ptr<ASTNode>>;

struct Directive : ASTNode {
	std::string name;
	std::vector<std::string> arguments;
	void Accept(ASTVisitor& visitor) const override { visitor.Visit(*this); }
};

struct Variable : ASTNode {
	std::string name;
	std::string value;
	VariableType type = VariableType::Simple;
	void Accept(ASTVisitor& visitor) const override { visitor.Visit(*this); }
};

// 규칙 노드들의 공통 부분(recipe)
struct Rule : ASTNode {
	std::vector<Line> recipes;
};

struct ExplicitRule : Rule {
	std::vector<std::string> targets;
	std::vector<std::string> prerequisites;
	void Accept(ASTVisitor& visitor) const override { visitor.Visit(*this); }
};

struct PatternRule : Rule {
	std::string target_pattern;
	std::string prerequisite_pattern;
	void Accept(ASTVisitor& visitor) const override { visitor.Visit(*this); }
};

struct StaticPatternRule : Rule {
	std::vector<std::string> targets;
	std::string target_pattern;
	std::string prerequisite_pattern;
	void Accept(ASTVisitor& visitor) const override { visitor.Visit(*this); }
};

// prerequisite가 없는 규칙 (예: clean:)
struct PhonyTarget : Rule {
	std::string target;
	void Accept(ASTVisitor& visitor) const override { visitor.Visit(*this); }
};
