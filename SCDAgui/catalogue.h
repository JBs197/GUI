#ifndef CATALOGUE_H
#define CATALOGUE_H

#include <windows.h>
#include <QtSql>
#include <QtConcurrent>
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
    QVector<QString> gid_list;
    QVector<QVector<QVector<int>>> model_tree;
    QVector<QVector<QString>> model_subtable_text_variables;

public:
    explicit CATALOGUE() {}
    ~CATALOGUE() {}
    void set_path(QString&);
    void initialize_table();
    void make_name_tree();
    QString sublabelmaker(QString&, QVector<QVector<int>>&);
    wstring get_csv_path(int);
    QString get_csv_branch(int);
    QString get_qname();
    QString get_gid(int);
    QVector<QString> get_create_table_statements(int);
    QVector<QString> get_CSV_insert_value_statements(int);
    void create_table_taskmaster(QVector<QString>&, int);
    void create_table_mapped(QVector<QString>&);
    QVector<QVector<QString>> get_nobles();
    QString get_create_sub_template();
    QVector<QVector<QVector<int>>> get_tree();
    QVector<QString> get_gid_list();
};

#endif // CATALOGUE_H
