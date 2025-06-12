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

    //2����-���� ��Ģ ����
    messages["E201"] = "pattern rule�� target�� pattern�� �ƴմϴ�.";
    messages["E202"] = "pattern rule�� target�� �ΰ� �̻��� % �� �ֽ��ϴ�.";
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
