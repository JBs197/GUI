#ifndef DBFUNCTIONS_H
#define DBFUNCTIONS_H

#endif // DBFUNCTIONS_H

#include "DBclasses.h"

using namespace std;

QSqlDatabase db_initialize(QString db_name)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(db_name);
    db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=3000");
    return db;
}

TREE_DB tree_initialize(QSqlDatabase& db, vector<wstring>& year_folders)
{
    TREE_DB tb;
    tb.tree_assembly(db, year_folders);
}

QStringList scan_years(wstring wdrive)
{
    vector<wstring> year_folders = get_subfolders(wdrive);
    QStringList year_list;
    wstring year;
    size_t pos1;
    int YEAR;
    for (size_t ii = 0; ii < year_folders.size(); ii++)
    {
        pos1 = year_folders[ii].rfind(L'\\');
        year = year_folders[ii].substr(pos1 + 1);
        try
        {
            YEAR = stoi(year);
        }
        catch (invalid_argument& ia)
        {
            continue;
        }
        year_list << QString::number(YEAR);
    }
    return year_list;
}

