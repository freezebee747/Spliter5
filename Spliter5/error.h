#pragma once
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

enum class Severity { Warning, Error, Fatal };

struct Error {
	std::string code;
	unsigned line = 0;
	Severity severity = Severity::Error;
	std::string filename;  // 비어 있으면 분석을 시작한 메인 파일
};

// 오류 코드 → 메시지 표. 생성 후 바뀌지 않는 읽기 전용 데이터라 싱글톤으로 둔다.
class ErrorMessage {
public:
	static const ErrorMessage& GetInstance();
	std::string GetMessage(const std::string& code) const;

private:
	ErrorMessage();
	std::unordered_map<std::string, std::string> messages_;
};

// 오류를 모으기만 한다. 출력은 ErrorReporter의 책임이다.
class ErrorCollector {
public:
	void Add(const std::string& code, unsigned line, Severity severity);
	// 다른 수집기의 오류를 덧붙인다. filename이 주어지면 파일 이름이 없는 오류에 붙인다.
	void Append(const ErrorCollector& other, const std::string& filename = "");

	const std::vector<Error>& GetAll() const { return errors_; }
	bool Empty() const { return errors_.empty(); }
	bool HasErrors() const;  // Warning을 제외한 오류가 있는가

private:
	std::vector<Error> errors_;
};

class ErrorReporter {
public:
	explicit ErrorReporter(const ErrorMessage& messages = ErrorMessage::GetInstance());
	void Print(const ErrorCollector& errors, std::ostream& out) const;

private:
	const ErrorMessage& messages_;
};
