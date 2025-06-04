#include "error.h"


const ErrorMessage& ErrorMessage::GetInstance() {
    static ErrorMessage instance;
    return instance;
}

ErrorMessage::ErrorMessage() {
    //0번지-변수 에러
    messages["E001"] = "변수 이름이 잘못되었습니다.";
    messages["E002"] = "변수 이름이 중복되었습네다.";
    messages["E003"] = "변수가 순환정의되어 있습니다.";

    //1번지-명시적 규칙 에러
    messages["E101"] = "target에 wildcard를 사용했습니다.";
    messages["E102"] = "target에 특수문자를 사용했습니다.";
    messages["E103"] = "target 이름이 중복되었습니다.";
    // ...
}
void ErrorCollector::AddError(const std::string code, unsigned lines, Severity sev) {
    Error err;
    err.code = code;
    err.line_number = lines;
    err.severity = sev;
    errors.push_back(err);
}

std::string ErrorMessage::GetMessage(const std::string& code) const {
    auto it = messages.find(code);
    return (it != messages.end()) ? it->second : "Unknown error";
}
