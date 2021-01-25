#include <QtSql>
#include "DBclasses.h"

using namespace std;

QSqlDatabase db_initialize(QString db_name)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(db_name);
    db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=1000");
    return db;
}
