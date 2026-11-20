// Spliter5 단위 테스트. 외부 라이브러리 없이 동작하는 최소한의 테스트 러너.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>

#include "analyzer.h"
#include "expander.h"
#include "function.h"
#include "parser.h"
#include "read.h"
#include "strutil.h"

namespace {

int g_passed = 0;
int g_failed = 0;

#define CHECK_EQ(actual, expected)                                                       \
	do {                                                                                 \
		auto actual_value = (actual);                                                    \
		auto expected_value = (expected);                                                \
		if (actual_value == expected_value) {                                            \
			++g_passed;                                                                  \
		} else {                                                                         \
			++g_failed;                                                                  \
			std::cerr << __FILE__ << ":" << __LINE__ << ": " #actual "\n"                \
					  << "  실제: [" << actual_value << "]\n"                            \
					  << "  기대: [" << expected_value << "]\n";                         \
		}                                                                                \
	} while (0)

// 테스트용 가짜 파일 목록 (IFileLookup 구현)
class FakeFiles : public IFileLookup {
public:
	FakeFiles(std::initializer_list<std::string> names) : names_(names) {}
	const std::unordered_set<std::string>& Filenames() const override { return names_; }

private:
	std::unordered_set<std::string> names_;
};

// 오류 목록을 "줄:코드" 형태로 이어 붙인다. include된 파일의 오류는 "파일명:줄:코드".
std::string Codes(const ErrorCollector& errors) {
	std::string out;
	for (const auto& e : errors.GetAll()) {
		if (!out.empty()) out += " ";
		if (!e.filename.empty()) out += std::filesystem::path(e.filename).filename().string() + ":";
		out += std::to_string(e.line) + ":" + e.code;
	}
	return out;
}

struct Fixture {
	explicit Fixture(std::initializer_list<std::string> files = {})
		: files(files),
		  functions(FunctionRegistry::WithBuiltins()),
		  directives(DirectiveRegistry::WithBuiltins()),
		  analyzer(this->files, functions, directives) {}

	std::string Run(const std::string& text) { return Codes(analyzer.AnalyzeText(text)); }
	std::string Var(const std::string& name) const {
		const VariableEntry* e = analyzer.Context().variables.Find(name);
		return e ? e->value : "<undefined>";
	}

	FakeFiles files;
	FunctionRegistry functions;
	DirectiveRegistry directives;
	MakefileAnalyzer analyzer;
};

std::string Expand(const std::string& text, const VariableTable& vars = VariableTable(), const IFileLookup& files = FakeFiles{}) {
	FunctionRegistry functions = FunctionRegistry::WithBuiltins();
	VariableExpander expander(vars, functions, files);
	return expander.Expand(text);
}

const char* const kOriginalTestTxt =
	"CC = gcc\n"
	"\n"
	"objects = main.o kbd.o command.o display.o \\\n"
	"\ttest.o whats.o\n"
	"\n"
	"all: main.o util.o\n"
	"\t$(CC) -o all $<\n"
	"\n"
	"main.o: main.c defs.h\n"
	"\t$(CC) -c main.c\n"
	"\n"
	"*.c: test.h\n"
	"\t$(CC) -c test.h\n"
	"\n"
	"$(objects): test.c defs.c\n"
	"\t$(CC) -o all $<\n"
	"\n"
	"util.o: util.c defs.h\n"
	"\t$(CC) -c util.c\n"
	"\t$(CC) -c defs.h\n"
	"\n"
	"kor.o: util.c defs.h ; $(CC) -c util.c\n"
	"\n"
	"clean:\n"
	"\trm -f *.o all\n";

void TestOriginalBehaviorPreserved() {
	// 리팩토링 전 프로그램이 test.txt에 대해 낸 결과와 같아야 한다.
	Fixture f{ "main.c", "defs.h", "util.c", "test.c", "defs.c", "test.h" };
	CHECK_EQ(f.Run(kOriginalTestTxt), std::string("12:E101 15:E103"));
	CHECK_EQ(f.Var("objects"), std::string("main.o kbd.o command.o display.o test.o whats.o"));
}

void TestStrutil() {
	CHECK_EQ(strutil::Trim("  a b \t"), std::string("a b"));
	CHECK_EQ(strutil::Join(strutil::SplitSpace(" a  b\tc "), ","), std::string("a,b,c"));
	std::string stem;
	CHECK_EQ(strutil::MatchPattern("src/%.c", "src/main.c", &stem), true);
	CHECK_EQ(stem, std::string("main"));
	CHECK_EQ(strutil::MatchPattern("%.c", "main.h"), false);
	CHECK_EQ(strutil::ReplacePercent("%.o", "main"), std::string("main.o"));
}

void TestBuiltins() {
	using namespace builtin;
	CHECK_EQ(Subst("ee", "EE", "feet on the street"), std::string("fEEt on the strEEt"));
	CHECK_EQ(Patsubst("%.c", "%.o", "a.c b.c c.h"), std::string("a.o b.o c.h"));
	CHECK_EQ(Strip("  a   b  "), std::string("a b"));
	CHECK_EQ(Findstring("a", "abc"), std::string("a"));
	CHECK_EQ(Findstring("x", "abc"), std::string(""));  // 예전: if (in.find(find)) 로 잘못 판정
	CHECK_EQ(Filter("%.c %.h", "a.c b.o c.h"), std::string("a.c c.h"));
	CHECK_EQ(Sort("c b a b"), std::string("a b c"));
	CHECK_EQ(Word("2", "a b c"), std::string("b"));
	CHECK_EQ(Word("0", "a b c"), std::string(""));  // 예전: texts[-1] 접근
	CHECK_EQ(Word("9", "a b c"), std::string(""));
	CHECK_EQ(Wordlist("2", "9", "a b c"), std::string("b c"));  // 예전: 범위 밖 접근
	CHECK_EQ(Words("a b c"), std::string("3"));  // 예전: capacity() 사용
	CHECK_EQ(Firstword("a b c"), std::string("a"));
	CHECK_EQ(Lastword("a b c"), std::string("c"));
	CHECK_EQ(Firstword(""), std::string(""));
	CHECK_EQ(Dir("src/foo.c hacks"), std::string("src/ ./"));
	CHECK_EQ(Notdir("src/foo.c hacks"), std::string("foo.c hacks"));  // 예전: "/foo.c"
	CHECK_EQ(Suffix("src/foo.c bar.h hacks"), std::string(".c .h"));  // 예전: '/'가 있을 때만 동작
	CHECK_EQ(Basename("src/foo.c hacks"), std::string("src/foo hacks"));
	CHECK_EQ(Addsuffix(".c", "foo bar"), std::string("foo.c bar.c"));
	CHECK_EQ(Addprefix("src/", "foo bar"), std::string("src/foo src/bar"));
	CHECK_EQ(Join("a b c", "1 2"), std::string("a1 b2 c"));
	CHECK_EQ(If("x", "yes", "no"), std::string("yes"));
	CHECK_EQ(If(" ", "yes", "no"), std::string("no"));
	CHECK_EQ(Or({ "", "b", "c" }), std::string("b"));
}

void TestExpander() {
	VariableTable vars;
	vars.Set("SRCS", { "a.c b.c", Flavor::Recursive });
	vars.Set("OBJS", { "$(patsubst %.c,%.o,$(SRCS))", Flavor::Recursive });
	vars.Set("PREFIX", { "SRC", Flavor::Recursive });
	vars.Set("EMPTY", { "", Flavor::Recursive });

	CHECK_EQ(Expand("$(OBJS)", vars), std::string("a.o b.o"));
	CHECK_EQ(Expand("${SRCS}", vars), std::string("a.c b.c"));
	CHECK_EQ(Expand("$($(PREFIX)S)", vars), std::string("a.c b.c"));  // 계산된 변수 이름
	CHECK_EQ(Expand("$(words $(SRCS) $(OBJS))", vars), std::string("4"));
	CHECK_EQ(Expand("$(words a,b c)", vars), std::string("2"));  // 인자 1개짜리 함수는 쉼표를 나누지 않음
	CHECK_EQ(Expand("$(if $(EMPTY),yes,no)", vars), std::string("no"));
	CHECK_EQ(Expand("$(lastword $(SRCS))", vars), std::string("b.c"));  // 예전: firstword를 호출
	CHECK_EQ(Expand("$(shell ls)", vars), std::string(""));  // 미구현 함수
	CHECK_EQ(Expand("$(UNDEFINED)x", vars), std::string("x"));
	CHECK_EQ(Expand("$@ $< $$", vars), std::string("$@ $< $"));

	FakeFiles files{ "a.c", "b.c", "c.h" };
	CHECK_EQ(Expand("$(wildcard *.c)", vars, files), std::string("a.c b.c"));  // 예전: basename을 호출
}

void TestVariables() {
	{
		Fixture f;
		CHECK_EQ(f.Run("A = x\nB := $(A) y\nA2 = z"), std::string(""));
		CHECK_EQ(f.Var("B"), std::string("x y"));  // 예전: 변수 이름 "B"가 값으로 저장됨
	}
	{
		Fixture f;
		CHECK_EQ(f.Run("CC=gcc"), std::string(""));
		CHECK_EQ(f.Var("CC"), std::string("gcc"));  // 예전: 이름이 "C"로 잘림
	}
	{
		Fixture f;
		CHECK_EQ(f.Run("OBJS = a.o\nOBJS += b.o\nX = 1\nX ?= 2\nY ?= 3"), std::string(""));  // 예전: += 를 중복(E012)으로 판정
		CHECK_EQ(f.Var("OBJS"), std::string("a.o b.o"));
		CHECK_EQ(f.Var("X"), std::string("1"));
		CHECK_EQ(f.Var("Y"), std::string("3"));
	}
	CHECK_EQ(Fixture().Run("A = 1\nA = 2"), std::string("2:E012"));
	CHECK_EQ(Fixture().Run("A = $(B)\nB = $(A)"), std::string("2:E013"));
	CHECK_EQ(Fixture().Run("C = $(C) x"), std::string("1:E013"));
	CHECK_EQ(Fixture().Run("1A = x\nCC2 = y"), std::string("1:E011"));
}

void TestParsing() {
	CHECK_EQ(Fixture().Run("# hello: world\nCC = gcc # comment"), std::string(""));  // 예전: 첫 줄 주석이 지워지지 않음
	CHECK_EQ(Fixture().Run("hello world"), std::string("1:E001"));
	CHECK_EQ(Fixture().Run("\techo hi"), std::string("1:E001"));
	CHECK_EQ(Fixture({ "util.c" }).Run("kor.o: util.c ; CC=gcc"), std::string(""));  // 예전: 변수로 오인

	DirectiveRegistry directives = DirectiveRegistry::WithBuiltins();
	Parser parser(directives);
	ParseResult result = parser.Parse(makefile_reader::ToLogicalLines(makefile_reader::SplitPhysicalLines(
		"clean:\n\trm -f *.o\n$(OBJS): %.o: %.c\n\tcc -c $<\n\techo done")));
	CHECK_EQ(result.nodes.size(), size_t(2));
	CHECK_EQ(result.default_target, std::string("clean"));
	auto* phony = dynamic_cast<PhonyTarget*>(result.nodes[0].get());
	auto* spr = dynamic_cast<StaticPatternRule*>(result.nodes[1].get());
	CHECK_EQ(phony != nullptr && phony->recipes.size() == 1, true);  // 예전: recipe를 버림
	CHECK_EQ(spr != nullptr && spr->recipes.size() == 2, true);      // 예전: recipe를 파싱하지 않음
}

void TestRules() {
	CHECK_EQ(Fixture({ "a.c" }).Run("OBJS = a.o b.o\n$(OBJS): %.o: %.c"), std::string("2:E151"));  // b.c 없음
	CHECK_EQ(Fixture().Run("x.txt: %.o: %.c"), std::string("1:E203"));
	CHECK_EQ(Fixture().Run("%.o %.p: %.c"), std::string("1:E202"));
	CHECK_EQ(Fixture().Run("foo.o: %.c"), std::string("1:E201"));
	CHECK_EQ(Fixture().Run("all: later\nlater:\n\techo"), std::string(""));  // 뒤에 정의된 타깃도 인정
	CHECK_EQ(Fixture().Run("a$$b: \nc=d: x"), std::string("1:E102"));
}

void TestInclude() {
	namespace fs = std::filesystem;
	fs::path dir = fs::temp_directory_path() / "spliter5_tests";
	fs::create_directories(dir);
	auto write = [&](const std::string& name, const std::string& text) {
		std::ofstream(dir / name) << text;
	};
	write("inc.mk", "INC_VAR = 1\nbad line\n");
	write("main.mk", "include inc.mk\nX := $(INC_VAR)\n-include nope.mk\ninclude nope.mk\n");
	write("a.mk", "include b.mk\n");
	write("b.mk", "include a.mk\n");

	{
		Fixture f;
		CHECK_EQ(Codes(f.analyzer.AnalyzeFile((dir / "main.mk").string())), std::string("inc.mk:2:E001 4:E301"));
		CHECK_EQ(f.Var("X"), std::string("1"));  // 포함된 파일의 변수가 공유됨
	}
	{
		Fixture f;
		CHECK_EQ(Codes(f.analyzer.AnalyzeFile((dir / "a.mk").string())), std::string("b.mk:1:E302"));
	}
	{
		Fixture f;
		CHECK_EQ(Codes(f.analyzer.AnalyzeFile((dir / "missing.mk").string())), std::string("0:E300"));
	}
	fs::remove_all(dir);
}

}  // namespace

int main() {
	TestOriginalBehaviorPreserved();
	TestStrutil();
	TestBuiltins();
	TestExpander();
	TestVariables();
	TestParsing();
	TestRules();
	TestInclude();

	std::cout << "통과: " << g_passed << ", 실패: " << g_failed << "\n";
	return g_failed == 0 ? 0 : 1;
}
