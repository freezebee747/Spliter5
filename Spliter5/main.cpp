#include <iostream>
#include <string>

#include "analyzer.h"
#include "directive.h"
#include "error.h"
#include "file.h"
#include "function.h"

// 사용법: Spliter5 [makefile 경로] [prerequisite 파일을 찾을 디렉토리]
// 객체를 만들고 연결하는 일(조립)은 여기서만 한다.
int main(int argc, char* argv[]) {
	std::string makefile = (argc > 1) ? argv[1] : "test.txt";
	std::string search_dir = (argc > 2) ? argv[2] : ".";

	DirSingleton::GetInstance().SetDir(search_dir);
	FileManagement files;
	if (!files.IsValidDir()) {
		std::cerr << "유효하지 않은 디렉터리입니다: " << search_dir << "\n";
	}

	FunctionRegistry functions = FunctionRegistry::WithBuiltins();
	DirectiveRegistry directives = DirectiveRegistry::WithBuiltins();
	MakefileAnalyzer analyzer(files, functions, directives);

	ErrorCollector errors = analyzer.AnalyzeFile(makefile);
	ErrorReporter().Print(errors, std::cout);
	return errors.HasErrors() ? 1 : 0;
}
