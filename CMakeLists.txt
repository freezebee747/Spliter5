cmake_minimum_required(VERSION 3.15)
project(Spliter5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(spliter5_core STATIC
	Spliter5/analyzer.cpp
	Spliter5/context.cpp
	Spliter5/directive.cpp
	Spliter5/error.cpp
	Spliter5/expander.cpp
	Spliter5/file.cpp
	Spliter5/function.cpp
	Spliter5/parser.cpp
	Spliter5/read.cpp
	Spliter5/strutil.cpp
	Spliter5/syntax.cpp
)
target_include_directories(spliter5_core PUBLIC Spliter5)

if(MSVC)
	target_compile_options(spliter5_core PUBLIC /W4)
else()
	target_compile_options(spliter5_core PUBLIC -Wall -Wextra)
endif()

add_executable(spliter5 Spliter5/main.cpp)
target_link_libraries(spliter5 PRIVATE spliter5_core)

enable_testing()
add_executable(spliter5_tests tests/tests.cpp)
target_link_libraries(spliter5_tests PRIVATE spliter5_core)
add_test(NAME spliter5_tests COMMAND spliter5_tests)
