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


