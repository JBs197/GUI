#include <QtSql>
#include <QApplication>
#include <windows.h>
#include "DBfunctions.h"
#include "mainwindow.cpp"

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, ".UTF8");
    QApplication a(argc, argv);
    MainWindow mw;
    mw.show();

    QSqlDatabase db = db_initialize("SCDA");
    if (!db.open())
    {
        err(L"db_initialize");
    }
    TREE_DB treebeard(db);
    treebeard.tree_initialize(db_root);


    return a.exec();
}
