#include "error.h"

const ErrorMessage& ErrorMessage::GetInstance() {
	static const ErrorMessage instance;
	return instance;
}

ErrorMessage::ErrorMessage() {
	//0번지-구문/변수 에러
	messages_["E001"] = "정확한 구문을 찾을 수 없습니다.";
	messages_["E011"] = "변수 이름이 잘못되었습니다.";
	messages_["E012"] = "변수 이름이 중복되었습니다.";
	messages_["E013"] = "변수가 순환정의되어 있습니다.";

	//1번지-명시적 규칙 에러
	messages_["E101"] = "target에 wildcard를 사용했습니다.";
	messages_["E102"] = "target에 특수문자를 사용했습니다.";
	messages_["E103"] = "target 이름이 중복되었습니다.";

	messages_["E151"] = "prerequisite과 일치하는 target 또는 파일이 존재하지 않습니다.";

	//2번지-패턴 규칙 에러
	messages_["E201"] = "pattern rule의 target이 pattern이 아닙니다.";
	messages_["E202"] = "pattern rule의 target에 두개 이상의 % 가 있습니다.";
	messages_["E203"] = "static pattern rule의 target이 target pattern과 일치하지 않습니다.";

	//3번지-파일/지시어 에러
	messages_["E300"] = "Makefile을 열 수 없습니다.";
	messages_["E301"] = "include 파일을 찾을 수 없습니다.";
	messages_["E302"] = "include가 순환 참조되어 있습니다.";
}

std::string ErrorMessage::GetMessage(const std::string& code) const {
	auto it = messages_.find(code);
	return (it != messages_.end()) ? it->second : "Unknown error";
}

void ErrorCollector::Add(const std::string& code, unsigned line, Severity severity) {
	Error error;
	error.code = code;
	error.line = line;
	error.severity = severity;
	errors_.push_back(error);
}

void ErrorCollector::Append(const ErrorCollector& other, const std::string& filename) {
	for (Error error : other.errors_) {
		if (error.filename.empty()) error.filename = filename;
		errors_.push_back(error);
	}
}

bool ErrorCollector::HasErrors() const {
	for (const auto& error : errors_) {
		if (error.severity != Severity::Warning) return true;
	}
	return false;
}

ErrorReporter::ErrorReporter(const ErrorMessage& messages) : messages_(messages) {}

void ErrorReporter::Print(const ErrorCollector& errors, std::ostream& out) const {
	for (const auto& error : errors.GetAll()) {
		if (!error.filename.empty()) out << error.filename << ":";
		out << error.line << ". [" << error.code << "] : " << messages_.GetMessage(error.code) << "\n";
	}
}
