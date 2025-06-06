#include "error.h"


const ErrorMessage& ErrorMessage::GetInstance() {
    static ErrorMessage instance;
    return instance;
}

ErrorMessage::ErrorMessage() {
    //0����-���� ����
    messages["E001"] = "��Ȯ�� ������ ã�� �� �����ϴ�.";
    messages["E011"] = "���� �̸��� �߸��Ǿ����ϴ�.";
    messages["E012"] = "���� �̸��� �ߺ��Ǿ����ϴ�.";
    messages["E013"] = "������ ��ȯ���ǵǾ� �ֽ��ϴ�.";

    //1����-����� ��Ģ ����
    messages["E101"] = "target�� wildcard�� ����߽��ϴ�.";
    messages["E102"] = "target�� Ư�����ڸ� ����߽��ϴ�.";
    messages["E103"] = "target �̸��� �ߺ��Ǿ����ϴ�.";

    messages["E151"] = "prerequisite�� ��ġ�ϴ� target �Ǵ� ������ �������� �ʽ��ϴ�.";
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
