#ifndef CATALOGUE_H
#define CATALOGUE_H

#include <windows.h>
#include <QtSql>
#include "csv.h"

class CATALOGUE
{
    CSV model;
    QString qpath;
    std::wstring wpath;
    QString qname;
    QString tname;
    string sname;
    wstring wname;
    QString qdescription;
    wstring wfile;
    QString qfile;
    wstring csv_trunk;
    vector<wstring> csv_branches;


public:
    explicit CATALOGUE() {}
    ~CATALOGUE() {}
    void set_path(QString&);
    void initialize_table();
    void make_name_tree();
    wstring get_csv_path(int);
    QVector<QString> get_create_table_statements();
};

#endif // CATALOGUE_H