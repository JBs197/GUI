#ifndef CATALOGUE_H
#define CATALOGUE_H

#include <windows.h>
#include <QtSql>
#include "basictools.h"

class CATALOGUE
{
    QString qpath;
    std::wstring wpath;
    QString qname;
    QString tname;
    string sname;
    wstring wname;
    QString qdescription;
    wstring wfile;
    int GID = -1;
    wstring csv_trunk;
    vector<wstring> csv_branches;
    vector<wstring> csv_rows;
    vector<bool> is_int; // Its length is equal to the number of columns.
    vector<int> int_val; // All the integer values of the column, in order.
    vector<double> double_val; // All the decimal values of the column, in order.
    vector<vector<wstring>> variables;  // Form [variable][variable type, variable shown in CSV]
    vector<wstring> columns; // Descriptor title for each value in the 1D CSV row.
    int model_CSV(wstring&);
    void linearize_tree(wstring&, wstring);
    void add_branch(QVector<QObject*>&, QObject*, QString);

public:
    explicit CATALOGUE() {}
    ~CATALOGUE() {}
    void set_path(QString&);
    void initialize_table();
    QString get_create_table_statement();
    void make_name_tree();
    wstring get_csv_path(int);
    QString get_insert_csv_statement(wstring&);
};

#endif // CATALOGUE_H
