#include "csv.h"

using namespace std;

void CSV::scan(wstring& wfile)
{
    size_t pos1, pos2, posnl1, posnl2;
    wstring temp1, temp2;
    QString qtemp;
    wchar_t math;
    int spaces = 0;
    int indentation = 0;
    int brindex = 0;
    int inum;
    double dnum;
    string exception;

    // Extract the text variables.
    pos1 = 0;
    pos2 = 0;
    while (1)
    {
        pos1 = wfile.find(L'=', pos1 + 1);                               // All of the CSV file's text variables
        if (pos1 > wfile.size()) { break; }                              // are given using the equality symbol.
        math = wfile[pos1 - 1];
        if (math == L'<' || math == L'>')
        {
            continue;
        }
        text_variables.push_back(QVector<QString>());

        pos2 = wfile.rfind(L'"', pos1); pos2++;
        temp1 = wfile.substr(pos2, pos1 - pos2);
        clean(temp1, 0);
        temp2 = L"\"" + temp1 + L"\"";
        qtemp = QString::fromStdWString(temp2);
        text_variables[text_variables.size() - 1].push_back(qtemp);

        pos2 = wfile.find(L'"', pos1);
        temp1 = wfile.substr(pos1 + 1, pos2 - pos1 - 1);
        clean(temp1, 1);
        temp2 = L"\'" + temp1 + L"\'";
        qtemp = QString::fromStdWString(temp2);
        text_variables[text_variables.size() - 1].push_back(qtemp);
    }

    // Extract the column titles.
    posnl1 = wfile.find(L'\n', pos2);
    posnl2 = wfile.find(L'\n', posnl1 + 1);
    pos1 = wfile.find(L'"', posnl1);
    while (pos1 < posnl2)
    {
        pos2 = wfile.find(L'"', pos1 + 1);
        temp1 = wfile.substr(pos1 + 1, pos2 - pos1 - 1);
        clean(temp1, 0);
        qtemp = QString::fromStdWString(temp1);
        column_titles.append(qtemp);
        pos1 = wfile.find(L'"', pos2 + 1);
    }
    if (column_titles.size() == 1) // Sometimes a CSV will not have column titles...
    {
        column_titles[0] = "Descriptions";
        column_titles.append("Values");
        posnl2 = posnl1;
    }

    while (1)
    {
        ROW row;
        posnl1 = posnl2;
        posnl2 = wfile.find(L'\n', posnl1 + 1);
        if (posnl2 >= wfile.size()) { break; }
        pos1 = wfile.find(L'"', posnl1);
        pos2 = wfile.find(L'"', pos1 + 1);
        temp1 = wfile.substr(pos1 + 1, pos2 - pos1 - 1);
        spaces = clean(temp1, 0);
        if (temp1 == L"Note") { break; }
        row.title = QString::fromStdWString(temp1);

        if (spaces == 0)

        pos2 = wfile.find_first_not_of(L"1234567890.", pos1 + 1);
        temp1 = wfile.substr(pos1 + 1, pos2 - pos1 - 1);
        clean(temp1, 0);
        pos1 = temp1.find(L'.');
        if (pos1 < temp1.size())
        {
            try
            {
                dnum = stod(temp1);
            }
            catch (invalid_argument& ia)
            {
                exception.append(ia.what());
                err8("stod exception in csv.scan: " + exception);
            }
            is_int.append({0});
        }
        else
        {
            try
            {
                inum = stoi(temp1);
            }
            catch (invalid_argument& ia)
            {
                exception.append(ia.what());
                err8("stoi exception in csv.scan: " + exception);
            }
            is_int.append({1});
        }
    }

    // Extract the row titles and values.

}
