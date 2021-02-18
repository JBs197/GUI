#ifndef CATALOGUE_H
#define CATALOGUE_H

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
    bool multi_column;
    QString qdescription;
    wstring wfile;
    QString qfile;
    wstring csv_trunk;
    vector<wstring> csv_branches;
    QVector<QString> gid_list;
    QVector<QString> column_titles;
    QVector<QString> row_titles;
    QVector<QString> primary_table_column_titles;
    QString primary_table_column_template;
    QVector<QVector<QVector<int>>> tree;
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
    int get_gid_size();
    QVector<QString> get_create_table_statements(int);
    QVector<QString> get_CSV_insert_value_statements(int);
    void create_table_taskmaster(QVector<QString>&, int);
    void create_table_mapped(QVector<QString>&);
    QVector<QVector<QString>> get_nobles();
    QString get_create_sub_template();
    QString get_insert_row_template();
    QVector<QVector<QVector<int>>> get_tree();
    QVector<QString> get_gid_list();
    QString get_primary_columns_template();
    void set_primary_columns_template(QString);
    void set_tree(QVector<QVector<QVector<int>>>);
    void set_gid_list(QVector<QString>);
    wstring get_csv_trunk();
    void set_csv_trunk(wstring);
    vector<wstring> get_csv_branches();
    void set_csv_branches(vector<wstring>);
    bool get_multicol();
    void set_multicol(bool);
    void set_qfile(int);
    QVector<QVector<QString>> extract_data_rows();
    QVector<QString> get_column_titles();
    void set_column_titles(QVector<QString>);
    QVector<QString> get_row_titles();
    void set_row_titles(QVector<QString>);
    QVector<QVector<QString>> get_model_text_variables();
};

#endif // CATALOGUE_H
