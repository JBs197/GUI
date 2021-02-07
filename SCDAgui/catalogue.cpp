#include "catalogue.h"

using namespace std;

// Load on-disk folder paths for the catalogue folder.
void CATALOGUE::set_path(QString& cata_path)
{
    qpath = cata_path;
    wpath = cata_path.toStdWString();
}

// Load the catalogue's significant variable names into the object.
void CATALOGUE::initialize_table()
{
    size_t pos1, pos2;
    wstring temp1, temp2, line;
    vector<wstring> sub_columns;
    vector<wstring> temp_columns;
    vector<wstring> temp_headings = { L"\0" };

    wstring folder_search = wpath + L"\\*.csv";
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr(L"FindFirstFile-cata.initialize_table"); }
    wstring sample_name = wpath + L"\\" + info.cFileName;
    HANDLE hfile2 = CreateFileW(sample_name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile2 == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-cata.initialize_table"); }

    wfile = bin_memory(hfile2);
    pos1 = wfile.rfind(L"Catalogue Number ", wfile.size() - 4);
    pos1 += 17;
    pos2 = wfile.find(L'.', pos1);
    wname = wfile.substr(pos1, pos2 - pos1);
    qname = QString::fromStdWString(wname);
    tname = "T" + qname;
    sname = utf16to8(wname);

    pos1 = sample_name.find(L'(');
    csv_trunk = sample_name.substr(0, pos1);

    pos1 = wfile.find(L"\"", 0); pos1++;
    pos2 = wfile.find(L"\"", pos1);
    temp1 = wfile.substr(pos1, pos2 - pos1);
    pos1 = 0;
    do
    {
        pos1++;
        pos1 = temp1.find(L'(', pos1);
        if (pos1 < temp1.size())
        {
            pos2 = temp1.find(L')', pos1);
            temp1.erase(pos1, pos2 - pos1 + 1);
        }
    } while (pos1 < temp1.size());
    qdescription = QString::fromStdWString(temp1);

    int error = model_CSV(wfile);
    if (error)  // This error condition is only activated if the source CSV file itself is missing data.
    {
        while (FindNextFileW(hfile1, &info))
        {
            sample_name = wpath + L"\\" + info.cFileName;
            hfile2 = CreateFileW(sample_name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hfile2 == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-cata.initialize_table(bogus model-making)"); }

            wfile = bin_memory(hfile2);
            pos1 = wfile.rfind(L"Catalogue Number ", wfile.size() - 4);
            pos1 += 17;
            pos2 = wfile.find(L'.', pos1);
            wname = wfile.substr(pos1, pos2 - pos1);
            qname = QString::fromStdWString(wname);
            tname = "T" + qname;
            sname = utf16to8(wname);

            pos1 = wfile.find(L"\"", 0); pos1++;
            pos2 = wfile.find(L"\"", pos1);
            temp1 = wfile.substr(pos1, pos2 - pos1);
            pos1 = 0;
            do
            {
                pos1++;
                pos1 = temp1.find(L'(', pos1);
                if (pos1 < temp1.size())
                {
                    pos2 = temp1.find(L')', pos1);
                    temp1.erase(pos1, pos2 - pos1 + 1);
                }
            } while (pos1 < temp1.size());
            qdescription = QString::fromStdWString(temp1);

            error = model_CSV(wfile);
            if (error == 0) { break; }
        }
    }

    if (!FindClose(hfile1)) { warn(L"FindClose-cata.initialize_table"); }
    if (!CloseHandle(hfile2)) { warn(L"CloseHandle-cata.initialize_table"); }
    log8(sname + " initialized.\r\n");
}

// Tedious extraction of the important CSV data which is shared uniformly across the catalogue.
int CATALOGUE::model_CSV(wstring& wfile)
{
    size_t posnl1, posnl2, pos1, pos2;
    wstring line, temp1, temp2;
    vector<wstring> sub_columns;
    vector<wstring> temp_columns;
    vector<wstring> temp_headings = { L"\0" };
    wchar_t math;
    int spaces = 0;
    int indentation = 0;

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
        variables.push_back(vector<wstring>());

        pos2 = wfile.rfind(L'"', pos1); pos2++;
        temp1 = wfile.substr(pos2, pos1 - pos2);
        clean(temp1, 0);
        temp2 = L"\"" + temp1 + L"\"";
        variables[variables.size() - 1].push_back(temp2);

        pos2 = wfile.find(L'"', pos1);
        temp1 = wfile.substr(pos1 + 1, pos2 - pos1 - 1);
        if (temp1.size() == 0) { err(L"csvread - GID " + to_wstring(GID)); }
        clean(temp1, 1);
        temp2 = L"\'" + temp1 + L"\'";
        variables[variables.size() - 1].push_back(temp2);
    }

    // Extract the column titles, and the integer/decimal row types.
    bool subtable = 0;
    double dnum;
    int inum;
    string exception;
    posnl2 = wfile.find(L'\n', pos2);  // Start from the closing quotation mark of the last variable.
    while (1) // For every line in the csv file...
    {
        posnl1 = posnl2;
        posnl2 = wfile.find(L'\n', posnl1 + 1);
        if (posnl2 < wfile.size() - 1) { line = wfile.substr(posnl1, posnl2 - posnl1); }
        else { break; }  // Either make the wstring "line" for this line, or break if EoF.

        pos1 = 0;
        while (1) // For every set of quotation marks on this line...
        {
            pos1 = line.find(L'"', pos1 + 1);
            if (pos1 < line.size())
            {
                pos2 = line.find(L'"', pos1 + 1);
                temp1 = line.substr(pos1 + 1, pos2 - pos1 - 1);
                spaces = clean(temp1, 0);
                temp_columns.push_back(temp1);
                pos1 = pos2;
            }
            else { break; }
        }

        if (temp_columns.size() > 1)                 // Multiple column headers on a single line indicates
        {                                            // that the CSV table has many subheaders. In order to
            subtable = 1;                            // compress such a 2D table into a 1D row, new columns
            linearize_tree(wfile, temp_columns[0]);  // are made by iterating through the subheader list
            temp_columns.clear();                    // so that each column has a unique title.

            continue;
        }
        else if (temp_columns[0] == L"Note")         // This is effectively EoF. The data underneath the
        {                                            // "Note" line is not of interest.
            break;
        }
        else                                         // If a line has only one pair of quotation
        {                                            // marks, then it is a data row.
            if (spaces == indentation)                       // If these match, then this line is a
            {                                                // "middle link" in the chain, and we can
                temp_headings.back() = temp_columns[0];      // extract its data straightforwardly.
            }
            else if (spaces > indentation)                   // If the number of spaces increases, then
            {                                                // we have entered into a new subtable. So,
                indentation = spaces;                        // we must increase the number of column titles.
                if (subtable)
                {
                    temp_headings.back() = temp_columns[0];
                }
                else
                {
                    temp_headings.push_back(temp_columns[0]);
                }
            }
            else if (spaces == 0)                            // This bracket is only accessed when we
            {                                                // transition out of a subtable, into the
                indentation = spaces;                        // main table.
                temp_headings.clear();
                temp_headings.push_back(temp_columns[0]);
            }
            else
            {
                indentation = spaces;                        // The rarest of conditions, this bracket is
                temp_headings.pop_back();                    // only accessed when we transition out of a
                temp_headings.back() = temp_columns[0];      // subsubtable and into a subtable.
            }
            temp_columns.clear();

            for (size_t ii = 0; ii < temp_headings.size(); ii++)
            {
                if (ii == 0) { temp1 = temp_headings[ii]; }                     // A unique column proto-title
                else                                                            // string is made for this line,
                {                                                               // regardless of the line's
                    temp1 += L" + ";                                            // status within a table,
                    temp1 += temp_headings[ii];                                 // subtable, or subsubtable.
                }
            }

            if (subtable)                                                       // If we are within a subtable,
            {                                                                   // the column proto-title is
                for (size_t ii = 1; ii < sub_columns.size(); ii++)              // combined with each CSV row
                {                                                               // header to make unique
                    temp2 = L"\"" + temp1 + L" + " + sub_columns[ii] + L"\"";   // column titles for our 1D
                    columns.push_back(temp2);                                   // SQL row.
                }

                pos1 = 0;
                for (size_t ii = 0; ii < sub_columns.size() - 1; ii++)
                {
                    pos1 = line.find(L',', pos1 + 1); pos1++;
                    pos2 = line.find(L',', pos1 + 1);
                    temp1 = line.substr(pos1, pos2 - pos1);

                                                                     // Some Stats Canada spreadsheets have these
                    if (temp1 == L"..") { return 1; }                // "unknown" placeholders for values. If one
                    else                                             // is found while building the CSV model
                    {                                                // (here!), we must abort and try another
                        pos1 = temp1.find(L'.', 0);                  // CSV file from the catalogue.
                        if (pos1 < temp1.size())
                        {
                            try
                            {
                                dnum = stod(temp1);
                            }
                            catch (invalid_argument& ia)
                            {
                                exception.append(ia.what());
                                err8("stod exception in cata.read_CSV: " + exception);
                            }
                            is_int.push_back(0);
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
                                err8("stoi exception in cata.read_CSV: " + exception);
                            }
                            is_int.push_back(1);
                        }
                    }
                }
            }
            else
            {
                temp2 = L"\"" + temp1 + L"\"";
                columns.push_back(temp2);

                pos1 = line.rfind(L',', line.size() - 2); pos1++;
                pos2 = line.find(L'\r', pos1);
                temp1 = line.substr(pos1, pos2 - pos1);
                                                                 // Some Stats Canada spreadsheets have these
                if (temp1 == L"..") { return 1; }                // "unknown" placeholders for values. If one
                else                                             // is found while building the CSV model
                {                                                // (here!), we must abort and try another
                    pos1 = temp1.find(L'.', 0);                  // CSV file from the catalogue.
                    if (pos1 < temp1.size())
                    {
                        try
                        {
                            dnum = stod(temp1);
                        }
                        catch (invalid_argument& ia)
                        {
                            exception.append(ia.what());
                            err8("stod exception in cata.read_CSV: " + exception);
                        }
                        is_int.push_back(0);
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
                            err8("stoi exception in cata.read_CSV: " + exception);
                        }
                        is_int.push_back(1);
                    }
                }
            }
        }
    }
    return 0;
}

QString CATALOGUE::get_create_table_statement()
{
    wstring sql = L"CREATE TABLE IF NOT EXISTS \"" + wname + L"\" ( GID INTEGER PRIMARY KEY, ";
    for (size_t ii = 0; ii < variables.size(); ii++)
    {
        sql += variables[ii][0];
        sql += L" TEXT, ";
    }
    for (size_t ii = 0; ii < columns.size(); ii++)
    {
        sql += columns[ii];
        if (is_int[ii]) { sql += L" INTEGER, "; }
        else { sql += L" REAL, "; }
    }
    sql.pop_back(); sql.pop_back();
    sql.append(L" );");

    QString sql2 = QString::fromStdWString(sql.c_str());
    return sql2;
}

void CATALOGUE::add_branch(QVector<QObject*>& qlist, QObject *parent, QString qname)
{
    QObject *branch = new QObject(parent);
    branch->setObjectName(qname);
    qlist.append(branch);
}

void CATALOGUE::linearize_tree(wstring& wfile, wstring root)
{
    size_t posnl1, posnl2, pos1, pos2;
    wstring line, temp1, temp2;
    QString qname;
    wchar_t math;
    int spaces = 0;
    int indentation = 0;

    // Locate the end of the "text variables" section.
    pos1 = 0;
    while (1)
    {
        pos1 = wfile.find(L'=', pos1 + 1);
        if (pos1 > wfile.size())
        {
            break;
        }
        math = wfile[pos1 - 1];
        if (math == L'<' || math == L'>')
        {
            continue;
        }
        else
        {
            pos2 = pos1;
        }
    }

    // Extract the final row title segments (leaves).
    posnl1 = wfile.find(L'\n', pos2);
    posnl2 = wfile.find(L'\n', posnl1 + 1);
    vector<wstring> leaves;
    pos2 = posnl1;
    while (1)
    {
        pos1 = wfile.find(L'"', pos2 + 1);
        if (pos1 > posnl2) { break; }
        pos2 = wfile.find(L'"', pos1 + 1);
        temp1 = wfile.substr(pos1 + 1, pos2 - pos1 - 1);
        spaces = clean(temp1, 0);
        leaves.push_back(temp1);
    }

    // Extract the in-between row segments (branches).
    int branch_index = 0;
    QObject trunk(nullptr);
    trunk.setObjectName(QString::fromStdWString(root));
    QVector<QObject*> branches;
    QVector<QObject*> hall_of_fame;
    hall_of_fame.append(new QObject(&trunk));
    QObject *active = new QObject(&trunk);
    while (1)
    {
        posnl1 = posnl2;
        posnl2 = wfile.find(L'\n', posnl1 + 1);
        if (posnl2 > wfile.size()) { break; }  // Secondary exit from the loop.
        pos1 = wfile.find(L'"', posnl1);
        pos2 = wfile.find(L'"', pos1 + 1);
        temp1 = wfile.substr(pos1 + 1, pos2 - pos1 - 1);
        spaces = clean(temp1, 0);
        if (temp1 == L"Note") { break; }  // Primary exit from the loop.
        qname = QString::fromStdWString(temp1);

        if (spaces == indentation)
        {
            add_branch(branches, active->parent(), qname);
        }
        else if (spaces > indentation)
        {
            if (branches.size() < 1)
            {
                add_branch(branches, &trunk, qname);
            }
            else
            {
                active->setParent(branches[branches.size() - 1]);
                branch_index++;
                if (branch_index >= hall_of_fame.size())
                {
                    hall_of_fame.append(new QObject(branches[branches.size() - 1]));
                }
                else
                {
                    hall_of_fame.replace(branch_index, new QObject(branches[branches.size() - 1]));
                }
                add_branch(branches, active->parent(), qname);
            }
            indentation = spaces;
        }
        else if (spaces < indentation)
        {
            branch_index--;
            active->setParent(hall_of_fame[branch_index]);
            add_branch(branches, active->parent(), qname);
            indentation = spaces;
        }
    }

    // Crush the 2D tree structure into a 1D vector.
    vector<wstring> rows;
    QVector<QObject*> family_line;
    QObject* dad = new QObject(nullptr);
    for (qsizetype ii = 0; ii < branches.size(); ii++)
    {
        family_line.append(branches[ii]);
        do
        {
            dad = family_line.last()->parent();
            if (dad != nullptr)
            {
                family_line.append(dad);
            }
        } while (dad != nullptr);
    }


}

void CATALOGUE::make_name_tree()
{
    size_t pos0 = csv_trunk.size();
    csv_branches = get_file_path_endings(wpath, pos0);
}

wstring CATALOGUE::get_csv_path(int index)
{
    wstring csv_path = csv_trunk + csv_branches[index];
    return csv_path;
}

// Conversion of 1 file's data from CSV to SQL.
QString CATALOGUE::get_insert_csv_statement(wstring& wfile)
{
    size_t posnl1, posnl2, pos1, pos2;
    wstring line, temp1, temp2;
    vector<wstring> text_vars;
    vector<wstring> sub_columns;
    vector<wstring> temp_columns;
    vector<wstring> temp_headings = { L"\0" };
    wchar_t math;
    int spaces = 0;
    int indentation = 0;

    // Extract the text variables.
    pos1 = 0;
    pos2 = 0;
    while (1)
    {
        pos1 = wfile.find(L'=', pos1 + 1);
        if (pos1 > wfile.size()) { break; }
        math = wfile[pos1 - 1];
        if (math == L'<' || math == L'>')
        {
            continue;
        }
        pos2 = wfile.find(L'"', pos1);
        temp1 = wfile.substr(pos1 + 1, pos2 - pos1 - 1);
        if (temp1.size() == 0) { err8(sname + " -cata.gics(vars)"); }
        clean(temp1, 1);
        temp2 = L"\'" + temp1 + L"\'";
        text_vars.push_back(temp2);
    }

    // Extract the integer/decimal row values.
    bool subtable = 0;
    int column_count = 0;
    posnl2 = wfile.find(L'\n', pos2);  // Start from the closing quotation mark of the last variable.
    while (1) // For every line in the csv file...
    {
        posnl1 = posnl2;
        posnl2 = wfile.find(L'\n', posnl1 + 1);
        if (posnl2 < wfile.size() - 1) { line = wfile.substr(posnl1, posnl2 - posnl1); }
        else { break; }  // Either make the wstring "line" for this line, or break if EoF.

        pos1 = 0;
        while (1) // For every set of quotation marks on this line...
        {
            pos1 = line.find(L'"', pos1 + 1);
            if (pos1 < line.size())
            {
                pos2 = line.find(L'"', pos1 + 1);
                temp1 = line.substr(pos1 + 1, pos2 - pos1 - 1);
                spaces = clean(temp1, 0);
                temp_columns.push_back(temp1);
                pos1 = pos2;
            }
            else { break; }
        }

        if (temp_columns.size() > 1)                 // Multiple column headers on a single line indicates
        {                                            // that the CSV table has subheaders. In order to
            subtable = 1;                            // compress such a 2D table into a 1D row, new columns
            sub_columns = temp_columns;              // are made by iterating through the subheader list
            temp_columns.clear();                    // so that each column has a unique title.
            continue;
        }
        else if (temp_columns[0] == L"Note")         // This is effectively EoF. The data underneath the
        {                                            // "Note" line is not of interest.
            break;
        }
        else                                         // If a line has only one pair of quotation
        {                                            // marks, then it is a data row.
            if (spaces == indentation)                       // If these match, then this line is a
            {                                                // "middle link" in the chain, and we can
                temp_headings.back() = temp_columns[0];      // extract its data straightforwardly.
            }
            else if (spaces > indentation)                   // If the number of spaces increases, then
            {                                                // we have entered into a new subtable. So,
                indentation = spaces;                        // we must increase the number of column titles.
                temp_headings.push_back(temp_columns[0]);
            }
            else if (spaces == 0)                            // This bracket is only accessed when we
            {                                                // transition out of a subtable, into the
                indentation = spaces;                        // main table.
                temp_headings.clear();
                temp_headings.push_back(temp_columns[0]);
            }
            else
            {
                indentation = spaces;                        // The rarest of conditions, this bracket is
                temp_headings.pop_back();                    // only accessed when we transition out of a
                temp_headings.back() = temp_columns[0];      // subsubtable and into a subtable.
            }
            temp_columns.clear();                            // This string container is now discarded.

            for (size_t ii = 0; ii < temp_headings.size(); ii++)
            {
                if (ii == 0) { temp1 = temp_headings[ii]; }                     // A unique column proto-title
                else                                                            // string is made for this line,
                {                                                               // regardless of the line's
                    temp1 += L" + ";                                            // status within a table,
                    temp1 += temp_headings[ii];                                 // subtable, or subsubtable.
                }
            }

            if (subtable)                                                       // If we are within a subtable,
            {                                                                   // the column proto-title is
                for (size_t ii = 1; ii < sub_columns.size(); ii++)              // combined with each CSV row
                {                                                               // header to make unique
                    temp2 = L"\"" + temp1 + L" + " + sub_columns[ii] + L"\"";   // column titles for our 1D
                    columns.push_back(temp2);                                   // SQL row.
                }

                pos1 = 0;
                for (size_t ii = 0; ii < sub_columns.size() - 1; ii++)
                {
                    pos1 = line.find(L',', pos1 + 1); pos1++;
                    pos2 = line.find(L',', pos1 + 1);
                    temp1 = line.substr(pos1, pos2 - pos1);

                    // Some Stats Canada spreadsheets have these "unknown" placeholders for values...
                    if (temp1 == L"..")
                    {
                        if (is_int[column_count])
                        {
                            temp1 = L"0";
                        }
                        else
                        {
                            temp1 = L"0.0";
                        }
                        column_count++;
                    }
                    else
                    {
                        if (is_int[column_count])
                        {
                            try { int_val.push_back(stoi(temp1)); }
                            catch (invalid_argument& ia) { err8(ia.what()); }
                        }
                        else
                        {
                            try { double_val.push_back(stod(temp1)); }
                            catch (invalid_argument& ia) { err8(ia.what()); }
                        }
                        column_count++;
                    }
                }
            }
            else
            {
                temp2 = L"\"" + temp1 + L"\"";
                columns.push_back(temp2);

                pos1 = line.rfind(L',', line.size() - 2); pos1++;
                pos2 = line.find(L'\r', pos1);
                temp1 = line.substr(pos1, pos2 - pos1);

                // Some Stats Canada spreadsheets have these ".." placeholders for unknown values...
                if (temp1 == L"..")
                {
                    if (is_int[column_count])
                    {
                        temp1 = L"0";
                        try { int_val.push_back(stoi(temp1)); }
                        catch (invalid_argument& ia) { err8(ia.what()); }
                    }
                    else
                    {
                        temp1 = L"0.0";
                        try { double_val.push_back(stod(temp1)); }
                        catch (invalid_argument& ia) { err8(ia.what()); }
                    }
                    column_count++;
                }
                else
                {
                    if (is_int[column_count])
                    {
                        try { int_val.push_back(stoi(temp1)); }
                        catch (invalid_argument& ia) { err8(ia.what()); }
                    }
                    else
                    {
                        try { double_val.push_back(stod(temp1)); }
                        catch (invalid_argument& ia) { err8(ia.what()); }
                    }
                    column_count++;
                }
            }
        }
    }

    QString bbbb = "bbbbbbbbb";
    return bbbb;

}
