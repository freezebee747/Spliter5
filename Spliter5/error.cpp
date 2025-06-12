#include "error.h"


const ErrorMessage& ErrorMessage::GetInstance() {
    static ErrorMessage instance;
    return instance;
}

ErrorMessage::ErrorMessage() {
    //0번지-변수 에러
    messages["E001"] = "정확한 구문을 찾을 수 없습니다.";
    messages["E011"] = "변수 이름이 잘못되었습니다.";
    messages["E012"] = "변수 이름이 중복되었습니다.";
    messages["E013"] = "변수가 순환정의되어 있습니다.";

    //1번지-명시적 규칙 에러
    messages["E101"] = "target에 wildcard를 사용했습니다.";
    messages["E102"] = "target에 특수문자를 사용했습니다.";
    messages["E103"] = "target 이름이 중복되었습니다.";

    messages["E151"] = "prerequisite과 일치하는 target 또는 파일이 존재하지 않습니다.";

    //2번지-패턴 규칙 에러
    messages["E201"] = "pattern rule의 target이 pattern이 아닙니다.";
    messages["E202"] = "pattern rule의 target에 두개 이상의 % 가 있습니다.";
    // ...
}
void ErrorCollector::AddError(const std::string code, unsigned lines, Severity sev) {
    Error err;
    err.code = code;
    err.line_number = lines;
    err.severity = sev;
    errors.push_back(err);
}


void ErrorCollector::SetExternalErrors(const std::string& filename){
    for (auto& i : errors) {
        i.external_filename = filename;
    }
}

void ErrorCollector::AppendErrorCollector(ErrorCollector& ec){
    std::vector<Error> temp = ec.GetAll();
    errors.insert(errors.end(), temp.begin(), temp.end());
}

std::string ErrorMessage::GetMessage(const std::string& code) const {
    auto it = messages.find(code);
    return (it != messages.end()) ? it->second : "Unknown error";
}
