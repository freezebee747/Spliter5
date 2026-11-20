#pragma once
#include <string>
#include <vector>

#include "AST.h"

// Makefile 텍스트를 '논리적인 줄' 단위로 바꾼다.
// 상태가 없는 변환이라 클래스 대신 네임스페이스 함수로 둔다.
namespace makefile_reader {

// 파일을 줄 번호와 함께 읽는다. 열지 못하면 ok에 false를 넣는다.
std::vector<Line> ReadPhysicalLines(const std::string& path, bool* ok = nullptr);
// 문자열을 줄 단위로 나눈다(테스트용).
std::vector<Line> SplitPhysicalLines(const std::string& text);

// 주석 제거 + 백슬래시(\)로 이어진 줄 합치기
std::vector<Line> ToLogicalLines(const std::vector<Line>& physical);

}  // namespace makefile_reader
