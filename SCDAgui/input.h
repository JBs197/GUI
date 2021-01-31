#ifndef INPUT_H
#define INPUT_H

#include <string>
#include <vector>
#include <QString>

using namespace std;

wstring db_root = L"F:";
string db_root8 = "F:";
wstring db_name = L"SCDA";
QString db_path = QString::fromStdWString(db_root + L"\\" + db_name + L".db");
string proj_root = "$${_PRO_FILE_PWD_}";
vector<wstring> domains = { L".com", L".net", L".org", L".edu", L".ca" };

#endif // INPUT_H
