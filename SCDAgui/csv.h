#ifndef CSV_H
#define CSV_H

#include <QObject>
#include <QString>
#include "basictools.h"

class CSV
{
public:
    explicit CSV() {}
    ~CSV() {}
    void scan(QString&, QString&);
    void quick_scan(QString&, wstring&, wstring&);
    QVector<int> insert_value_all_statements(QVector<QString>&, QVector<QVector<QVector<int>>>&, QVector<QString>&, QVector<QVector<QString>>&);
    void tree_walker();
    void tree_printer();
    void set_gid(QString&);
    bool get_multi_column();
    QVector<QString> get_column_titles();
    QVector<QVector<QString>> get_text_variables();
    QVector<QVector<QVector<int>>> get_model_tree();
    QVector<QString> create_table_cata(QVector<QVector<QString>>&);
    void create_table_csvs(QVector<QVector<QString>>&, QVector<QString>&);
    void create_table_subs(QVector<QString>&, QVector<QString>&, QVector<QVector<QVector<int>>>&);
    void create_sub_template(QString&);
    void insert_row_template(QString&);
    QVector<QString> get_row_titles();

private:
    QString qfile;
    QString qname;
    QString gid;
    int subqname = 0;
    bool multi_column;
    QVector<int> indent;
    QVector<QVector<QString>> text_variables;  // Form [row][text var type, text var value]
    QVector<QString> column_titles;
    QVector<QVector<QVector<int>>> tree;  // Form [path possibility][genealogy][leaves]
    QVector<QString> unique_row_buffer;  // Form [value's indentation]. It is initialized with an empty string in 'scan'.
    QVector<QVector<QString>> model_rows;  // Form [row_index][row title, row values...]
    QVector<int> model_is_int;  // Form [row_index], values 0 = error, 1 = int, 2 = double.
    int extract_variables();
    int extract_column_titles(int);
    void extract_rows(int);
    void extract_classic_rows(int);
    void extract_model_rows(int);
    QString unique_row_title(int);
    QString unique_row_title_multicol(int, int&);
    QString sublabelmaker(QString&, QVector<QVector<int>>&);
    void extract_row_values(int&, QVector<QString>&, QVector<int>&);
    int is_parent(QVector<QVector<QVector<int>>>&, QVector<int>, int, int);
    QVector<int> insert_subtable_statement(QVector<QString>&, QVector<QVector<int>>&, QVector<QString>&, QVector<QVector<QString>>&);

};

#endif // CSV_H

