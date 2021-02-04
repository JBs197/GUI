#include "workthreader.h"

WORKTHREADER::WORKTHREADER (QSqlDatabase& datab, QObject *parent) :
    db(datab), QObject(parent) {}

void WORKTHREADER::qpreload(QStringList& ql)
{
    qlist = ql;
}

void WORKTHREADER::wpreload(vector<wstring>& wl)
{
    wlist = wl;
}

void WORKTHREADER::begin(int type, int& work_per_cata)
{
    switch (type)
    {
    case 1:  // Insert a list of catalogues into the database.
        work_per_cata = init_cata();
        prepare_table();
        get_CSV_paths(qlist);

        break;
    }

}

int WORKTHREADER::init_cata()
{
    folder = wlist[0];
    size_t pos1, pos2;
    wstring temp1, temp2;

    wstring folder_search = folder + L"\\*.csv";
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr(L"FindFirstFile-WORKTHREAD.init"); }
    wstring sample_name = folder + L"\\" + info.cFileName;
    HANDLE hfile2 = CreateFileW(sample_name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile2 == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-WORKTHREAD.init"); }

    wstring wfile = bin_memory(hfile2);
    pos1 = wfile.rfind(L"Catalogue Number ", wfile.size() - 4);
    pos1 += 17;
    pos2 = wfile.find(L'.', pos1);
    temp1 = wfile.substr(pos1, pos2 - pos1);
    cata_number = utf16to8(temp1);

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
    string cata_name = utf16to8(temp1);

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
                if (hfile2 == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-WORKTHREAD.init"); }

                wfile = bin_memory(hfile2);
                pos1 = wfile.rfind(L"Catalogue Number ", wfile.size() - 4);
                pos1 += 17;
                pos2 = wfile.find(L'.', pos1);
                temp1 = wfile.substr(pos1, pos2 - pos1);
                cata_number = utf16to8(temp1);

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
    if (!CloseHandle(hfile2)) { warn(L"CloseHandle-WORKTHREAD.init"); }

    int count = 0;
    do
    {
        count++;
    } while (FindNextFileW(hfile1, &info));

    if (!FindClose(hfile1)) { warn(L"FindClose-WORKTHREAD.init"); }
    log8(cata_number + " initialized.\r\n");
    return count;
}

void WORKTHREADER::prepare_table()
{
    string sql = "CREATE TABLE IF NOT EXISTS \"T" + cata_number + "\" ( GID INTEGER PRIMARY KEY, ";
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
        sqlerr(L"stmt.exec-peon::prepare_table", stmt.lastError());
    }
    log8(cata_number + " table prepared.\r\n");
}

void WORKTHREADER::get_CSV_paths(QStringList& csv_list)
{
    csv_list.clear();
    wstring folder_search = folder + L"\\*.csv";
    WIN32_FIND_DATAW info;
    wstring wcsv_path;
    QString qcsv_path;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr(L"FindFirstFile-get_CSV_paths"); }
    do
    {
        wcsv_path = folder + L"\\" + info.cFileName;
        qcsv_path = QString::fromStdWString(wcsv_path);
        csv_list.append(qcsv_path);
    } while (FindNextFileW(hfile1, &info));
}

void WORKTHREADER::add_CSV(QString qcsv_path)
{
    wstring wcsv_path = qcsv_path.toStdWString();
    HANDLE hfile1 = CreateFileW(wcsv_path.c_str(),(GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, 0, NULL);
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-add_CSV"); }
    wstring wfile = bin_memory(hfile1);
    HANDLE hfile2;

    size_t pos1, pos2;
    int GID;
    pos1 = wcsv_path.rfind(L"\\");
    wstring short_name = wcsv_path.substr(pos1 + 1);
    pos1 = short_name.find(L"(", 0);
    pos1++;
    pos2 = short_name.find(L")", pos1);
    wstring temp1 = short_name.substr(pos1, pos2 - pos1);
    try { GID = stoi(temp1); }
    catch (invalid_argument& ia) { err8(ia.what()); }

    pos1 = short_name.find(L')', 0);
    pos1 += 2;
    pos2 = short_name.rfind(L'.', short_name.size() - 1);
    temp1 = short_name.substr(pos1, pos2 - pos1);

    CSV_DB cdb(wfile, GID, cata_number, is_int);
    int error = cdb.read();
    if (error > 0) { err(L"CSV_DB read error #" + to_wstring(error)); }
    string error_msg;
    string sql = cdb.get_insert();
    QSqlQuery stmt(db);
    QString sql2 = QString::fromUtf8(sql.c_str());
    stmt.prepare(sql2);
    if (!stmt.exec())
    {
        sqlwarn(L"populate_table", stmt.lastError());
        if (cdb.bad_apple(wcsv_path))
        {
            err(L"really_bad_apple");
        }
        else
        {
            hfile2 = CreateFileW(wcsv_path.c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hfile2 == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-bad_apple"); }
            wfile = bin_memory(hfile2);
            if (!CloseHandle(hfile2)) { winwarn(L"CloseHandle-bad_apple"); }
            CSV_DB cdb2(wfile, GID, cata_number);
            error = cdb2.read();
            if (error > 0) { err(L"CSV_DB (bad_apple) read error #" + to_wstring(error)); }
            sql = cdb2.get_insert();
            QSqlQuery stmt2(db);
            QString sql3 = QString::fromUtf8(sql.c_str());
            stmt2.prepare(sql3);
            if (!stmt2.exec()) { sqlerr(L"add_CSV-bad_apple", stmt2.lastError()); }
        }
    }
    log8(cata_number + " table populated.\r\n");
    jobsdone();
}

void WORKTHREADER::add_cata()
{
    QFutureWatcher<void> watch;
    QFuture<void> future;
    for (qsizetype ii = 0; ii < qlist.size(); ii++)
    {
        connect(&watch, &QFutureWatcher<void>::finished, this, &WORKTHREADER::jobsdone);
        future = QtConcurrent::run(QThreadPool::globalInstance(), &WORKTHREADER::add_CSV, qlist[ii]);
    }
}
