#ifndef DBCLASSES_H
#define DBCLASSES_H

#endif // DBCLASSES_H

#include <string>
#include <windows.h>
#include <QtSql>
#include "basictools.h"

using namespace std;

// This object represents one CSV spreadsheet file.
class CSV_DB {
    wstring wfile; // Full CSV file.
    int GID; // Numeric code for the geographical region.
    string cata_number; // Stats Can's alphanumeric designation for the catalogue.
    bool making_the_model;  // If true, will determine each column value's integer/decimal status for the CATA_DB.
    vector<bool> is_int; // Its length is equal to the number of columns.
    vector<int> int_val; // All the integer values of the column, in order.
    vector<double> double_val; // All the decimal values of the column, in order.
    vector<vector<string>> variables;  // Form [variable][variable type, variable shown in CSV]
    vector<string> columns; // Descriptor title for each value in the 1D CSV row.
public:
    CSV_DB(wstring& wf, int gid, string cn, vector<bool> is_integer) { wfile = wf; GID = gid; cata_number = cn; is_int = is_integer; making_the_model = 0; }
    CSV_DB(wstring& wf, int gid, string cn) { wfile = wf; GID = gid; cata_number = cn; making_the_model = 1; }
    ~CSV_DB() {}
    int read(); // Populates the internal variables. Returns greater than zero for errors.
    vector<string> get_varicols(); // Returns a list of the column titles.
    string get_insert(); // Returns a SQL INSERT statement string for the complete CSV.
    int bad_apple(wstring); // Given the full path of a corrupt CSV file, re-download it using the archive bin.
    vector<vector<string>> get_variables() { return variables; }
    vector<string> get_columns() { return columns; }
    vector<bool> get_value_types() { return is_int; }
};
int CSV_DB::read()
{
    size_t posnl1, posnl2, pos1, pos2;
    wstring line, temp1, temp2;
    vector<wstring> sub_columns;
    vector<wstring> temp_columns;
    vector<wstring> temp_headings = { L"\0" };
    wchar_t math;
    int spaces = 0;
    int indentation = 0;

    // Populate the variables.
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
        variables.push_back(vector<string>());

        pos2 = wfile.rfind(L'"', pos1); pos2++;
        temp1 = wfile.substr(pos2, pos1 - pos2);
        clean(temp1, 0);
        temp2 = L"\"" + temp1 + L"\"";
        variables[variables.size() - 1].push_back(utf16to8(temp2));

        pos2 = wfile.find(L'"', pos1);
        temp1 = wfile.substr(pos1 + 1, pos2 - pos1 - 1);
        if (temp1.size() == 0) { err(L"csvread - GID " + to_wstring(GID)); return 1; }
        clean(temp1, 1);
        temp2 = L"\'" + temp1 + L"\'";
        variables[variables.size() - 1].push_back(utf16to8(temp2));
    }

    // Populate the columns and values.
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

        if (temp_columns.size() > 1)
        {
            subtable = 1;
            sub_columns = temp_columns;
            temp_columns.clear();
            continue;
        }
        else if (temp_columns[0] == L"Note")
        {
            break;
        }
        else
        {
            if (spaces == indentation)
            {
                temp_headings.back() = temp_columns[0];
            }
            else if (spaces > indentation)
            {
                indentation = spaces;
                temp_headings.push_back(temp_columns[0]);
            }
            else if (spaces == 0)
            {
                indentation = spaces;
                temp_headings.clear();
                temp_headings.push_back(temp_columns[0]);
            }
            else
            {
                indentation = spaces;
                temp_headings.pop_back();
                temp_headings.back() = temp_columns[0];
            }
            temp_columns.clear();

            for (size_t ii = 0; ii < temp_headings.size(); ii++)
            {
                if (ii == 0) { temp1 = temp_headings[ii]; }
                else
                {
                    temp1 += L" + ";
                    temp1 += temp_headings[ii];
                }
            }

            if (subtable)
            {
                for (size_t ii = 1; ii < sub_columns.size(); ii++)
                {
                    temp2 = L"\"" + temp1 + L" + " + sub_columns[ii] + L"\"";
                    columns.push_back(utf16to8(temp2));
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
                        if (making_the_model)
                        {
                            return 2;
                        }
                        else
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
                columns.push_back(utf16to8(temp2));

                pos1 = line.rfind(L',', line.size() - 2); pos1++;
                pos2 = line.find(L'\r', pos1);
                temp1 = line.substr(pos1, pos2 - pos1);

                // Some Stats Canada spreadsheets have these ".." placeholders for unknown values...
                if (temp1 == L"..")
                {
                    if (making_the_model)
                    {
                        return 2;
                    }
                    else
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
                }
                else if (making_the_model)
                {
                    pos1 = temp1.find(L'.', 0);
                    if (pos1 < temp1.size())
                    {
                        is_int.push_back(0);
                        try { double_val.push_back(stod(temp1)); }
                        catch (invalid_argument& ia) { err8(ia.what()); }
                    }
                    else
                    {
                        is_int.push_back(1);
                        try { int_val.push_back(stoi(temp1)); }
                        catch (invalid_argument& ia) { err8(ia.what()); }
                    }
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
    return 0;
}
vector<string> CSV_DB::get_varicols()
{
    vector<string> varicols = columns;
    vector<string>::iterator it;
    for (int ii = variables.size() - 1; ii >= 0; ii--)
    {
        it = varicols.begin();
        varicols.insert(it, variables[ii][0]);
    }
    return varicols;
}
string CSV_DB::get_insert()
{
    string statement = "INSERT INTO \"T" + cata_number + "\" ( GID, ";
    for (size_t ii = 0; ii < variables.size(); ii++)
    {
        statement += variables[ii][0];
        statement += ", ";
    }
    for (size_t ii = 0; ii < columns.size(); ii++)
    {
        statement += columns[ii];
        if (ii < columns.size() - 1) { statement += ", "; }
    }
    statement += ") VALUES (" + to_string(GID) + ", ";

    int int_counter = 0;
    int double_counter = 0;
    for (size_t ii = 0; ii < variables.size(); ii++)
    {
        statement += variables[ii][1] + ", ";
    }
    for (size_t ii = 0; ii < columns.size(); ii++)
    {
        if (is_int[ii])
        {
            statement += to_string(int_val[int_counter]);
            int_counter++;
        }
        else
        {
            statement += to_string(double_val[double_counter]);
            double_counter++;
        }

        if (ii < columns.size() - 1)
        {
            statement += ", ";
        }
    }
    statement += " ) ";

    statement += "ON CONFLICT ( GID ) WHERE true DO NOTHING ";

    statement += ";";

    return statement;
}
int CSV_DB::bad_apple(wstring apple_full_name)
{
    delete_file(apple_full_name);

    size_t pos1 = apple_full_name.rfind(L'\\', apple_full_name.size() - 1);
    wstring folder = apple_full_name.substr(0, pos1);
    wstring short_name = apple_full_name.substr(pos1 + 1);
    size_t pos2 = apple_full_name.rfind(L'\\', pos1 - 1);
    wstring archive = apple_full_name.substr(0, pos2);
    wstring temp1 = archive.substr(archive.size() - 4, 4);
    archive += L"\\" + temp1 + L" archive.bin";
    HANDLE hfile = CreateFileW(archive.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { warn(L"CreateFile-bad_apple"); return 1; }
    wstring wfile = bin_memory(hfile);
    if (!CloseHandle(hfile)) { warn(L"CloseHandle-bad_apple"); return 2; }

    wstring cata16 = utf8to16(cata_number);
    pos1 = wfile.find(cata16, 0);
    pos1 = wfile.find(L' ', pos1);
    pos1++;
    pos2 = wfile.find(L"||", pos1);
    pos2--;
    wstring url = wfile.substr(pos1, pos2 - pos1);
    pos1 = url.find(L"GID=", 0);
    pos1 += 4;
    pos2 = url.find(L'&', pos1);
    url.replace(pos1, pos2 - pos1, to_wstring(GID));

    int error = download(url, folder, short_name);
    if (error) { warn(L"download-bad_apple"); return 3; }

    return 0;
}

// This object represents one catalogue folder, as geographically categorized by Statistics Canada.
class CATA_DB {
    wstring folder; // Full path of the catalogue folder containing the CSVs.
    int col_width;
    string cata_number; // Stats Can's alphanumeric designation for the catalogue.
    string table_name; // The catalogue number, immediately proceeded by a 'T'.
    string cata_name; // The phrase describing the catalogue's topic.
    vector<bool> is_int; // Its length is equal to the number of columns.
    vector<string> column_header; // Descriptor title for each value in the 1D CSV row.
    vector<vector<string>> variables; // Form [variable][variable type, variable shown in CSV]
public:
    CATA_DB(wstring folder_path) { init(folder_path); }
    ~CATA_DB() {}
    void init(wstring); // Given a catalogue folder, populate the new CATA_DB object's internal variables.
    void prepare_table(QSqlDatabase&); // Each table in the database corresponds to one catalogue.
    void populate_table(QSqlDatabase&); // Read every CSV's values into the table.
};
void CATA_DB::init(wstring folder_path)
{
    folder = folder_path;
    size_t pos1, pos2;
    wstring temp1, temp2;
    col_width = 0;

    wstring folder_search = folder + L"\\*.csv";
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { err(L"FindFirstFile-init"); }
    wstring sample_name = folder + L"\\" + info.cFileName;
    HANDLE hfile2 = CreateFileW(sample_name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile2 == INVALID_HANDLE_VALUE) { err(L"CreateFile-init"); }

    wstring wfile = bin_memory(hfile2);
    pos1 = wfile.rfind(L"Catalogue Number ", wfile.size() - 4);
    pos1 += 17;
    pos2 = wfile.find(L'.', pos1);
    temp1 = wfile.substr(pos1, pos2 - pos1);
    cata_number = utf16to8(temp1);
    table_name = "T" + cata_number;

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
    cata_name = utf16to8(temp1);

    CSV_DB model(wfile, -1, cata_name);
    int error = model.read();
    if (error > 0)
    {
        if (error == 2)
        {
            while (FindNextFileW(hfile1, &info))
            {
                sample_name = folder + L"\\" + info.cFileName;
                hfile2 = CreateFileW(sample_name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hfile2 == INVALID_HANDLE_VALUE) { err(L"CreateFile-init(bogus model-making)"); }

                wfile = bin_memory(hfile2);
                pos1 = wfile.rfind(L"Catalogue Number ", wfile.size() - 4);
                pos1 += 17;
                pos2 = wfile.find(L'.', pos1);
                temp1 = wfile.substr(pos1, pos2 - pos1);
                cata_number = utf16to8(temp1);
                table_name = "T" + cata_number;

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
                cata_name = utf16to8(temp1);

                CSV_DB model(wfile, -1, cata_name);
                error = model.read();
                if (error == 0) { break; }
            }
        }
    }
    variables = model.get_variables();
    column_header = model.get_columns();
    is_int = model.get_value_types();
    if (!FindClose(hfile1)) { warn(L"FindClose-init"); }
    if (!CloseHandle(hfile2)) { warn(L"CloseHandle-init"); }
    log8(cata_number + " initialized.\r\n");
}
void CATA_DB::prepare_table(QSqlDatabase& db)
{
    string sql = "CREATE TABLE IF NOT EXISTS \"" + table_name + "\" ( GID INTEGER PRIMARY KEY, ";
    for (size_t ii = 0; ii < variables.size(); ii++)
    {
        sql += variables[ii][0];
        sql += " TEXT, ";
    }
    for (size_t ii = 0; ii < column_header.size(); ii++)
    {
        sql += column_header[ii];
        if (is_int[ii]) { sql += " INTEGER, "; }
        else { sql += " REAL, "; }
    }
    sql.pop_back(); sql.pop_back();
    sql.append(" );");
    QSqlQuery stmt(db);
    QString sql2 = QString::fromUtf8(sql.c_str());
    stmt.prepare(sql2);
    if (!stmt.exec())
    {
        sqlerr(L"prepare_table", stmt);
    }
    log8(cata_number + " table prepared.\r\n");
}
void CATA_DB::populate_table(QSqlDatabase& db)
{
    wstring folder_search = folder + L"\\*.csv";
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr(L"FindFirstFile-populate_table"); }
    HANDLE hfile2, hfile3;
    wstring short_name, full_name, wfile, temp1, sql;
    size_t pos1, pos2;
    int GID, error;

    do
    {
        short_name = info.cFileName;
        pos1 = short_name.find(L"(", 0); pos1++;
        pos2 = short_name.find(L")", pos1);
        temp1 = short_name.substr(pos1, pos2 - pos1);
        try { GID = stoi(temp1); }
        catch (invalid_argument& ia) { err8(ia.what()); }

        pos1 = short_name.find(L')', 0);
        pos1 += 2;
        pos2 = short_name.rfind(L'.', short_name.size() - 1);
        temp1 = short_name.substr(pos1, pos2 - pos1);

        full_name = folder + L"\\" + short_name;
        hfile2 = CreateFileW(full_name.c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hfile2 == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-populate_table"); }
        wfile = bin_memory(hfile2);
        if (!CloseHandle(hfile2)) { winwarn(L"CloseHandle-populate_table"); }

        CSV_DB cdb(wfile, GID, cata_number, is_int);
        error = cdb.read();
        if (error > 0) { err(L"CSV_DB read error #" + to_wstring(error)); }
        string error_msg;
        string sql;

        sql = cdb.get_insert();
        QSqlQuery stmt(db);
        QString sql2 = QString::fromUtf8(sql.c_str());
        stmt.prepare(sql2);
        if (!stmt.exec())
        {
            sqlwarn(L"populate_table", stmt);
            if (cdb.bad_apple(full_name))
            {
                err(L"really_bad_apple");
            }
            else
            {
                hfile3 = CreateFileW(full_name.c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hfile3 == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-bad_apple"); }
                wfile = bin_memory(hfile3);
                if (!CloseHandle(hfile3)) { winwarn(L"CloseHandle-populate"); }
                CSV_DB cdb2(wfile, GID, cata_number);
                error = cdb2.read();
                if (error > 0) { err(L"CSV_DB (bad_apple) read error #" + to_wstring(error)); }
                sql = cdb2.get_insert();
                QSqlQuery stmt2(db);
                QString sql3 = QString::fromUtf8(sql.c_str());
                stmt2.prepare(sql3);
                if (!stmt2.exec()) { sqlerr(L"populate_table-bad_apple", stmt2); }
            }
        }
    } while (FindNextFileW(hfile1, &info));
    log8(cata_number + " table populated.\r\n");
}

// This object represents the organizational union of all catalogues in the database.
class TREE_DB {
    QSqlDatabase& db;
    wstring root_folder;
    vector<wstring> years; // Indices match the first dimension of the 'catalogue_paths' matrix.
    vector<vector<wstring>> catalogue_paths;
    vector<vector<CATA_DB>> trunk; // Form [Year][Catalogue]
public:
    TREE_DB(QSqlDatabase& db) : db(db) {}
    ~TREE_DB() {}
    void tree_initialize(wstring); // Given a root folder, build the database using all CSVs in all subfolders within.
    CATA_DB tablemaker(QSqlDatabase&, wstring); // Given a database and a folder containing CSV files, make a table for that catalogue.
};
CATA_DB TREE_DB::tablemaker(QSqlDatabase& db, wstring cata_folder)
{
    CATA_DB cata(cata_folder);
    cata.prepare_table(db);
    cata.populate_table(db);
    return cata;
}
void TREE_DB::tree_initialize(wstring rf)
{
    root_folder = rf;
    catalogue_paths = get_subfolders2(root_folder);
    for (size_t ii = 0; ii < catalogue_paths.size(); ii++)
    {
        trunk.push_back(vector<CATA_DB>());
        for (size_t jj = 0; jj < catalogue_paths[ii].size(); jj++)
        {
            trunk[ii].push_back(tablemaker(db, catalogue_paths[ii][jj]));
        }
    }
}

