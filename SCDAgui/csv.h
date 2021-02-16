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
    QVector<int> insert_value_all_statements(QVector<QString>&, QVector<QVector<QVector<int>>>&, QVector<QString>&, QVector<QVector<QString>>&);
    void tree_walker();
    void set_gid(QString&);
    bool get_multi_column();
    QVector<QString> get_column_titles();
    QVector<QVector<QString>> get_text_variables();
    QVector<QVector<QVector<int>>> get_model_tree();
    void create_table_cata(QVector<QVector<QString>>&);
    void create_table_csvs(QVector<QVector<QString>>&, QVector<QString>&);
    void create_table_subs(QVector<QString>&, QVector<QString>&, QVector<QVector<QVector<int>>>&);
    void create_sub_template(QString&);
    void create_row_template(QString&);

private:
    QString qfile;
    QString qname;
    QString gid;
    int subqname = 0;
    bool multi_column;
    QVector<int> indent;
    QMap<int, QString> map_values;  // All maps use the row index from 'row_titles' as their key.
    QMap<int, int> map_isint;
    QVector<QVector<QString>> text_variables;  // Form [row][text var type, text var value]
    QVector<QString> column_titles;
    //QVector<QString> row_titles;  // Form [row]. Title includes '+' to show space index.
    //QVector<QVector<int>> is_int;  // Form [new index][row, bool, sindex]. Negative bool (ha) indicates bad data on file.
    //QVector<QString> row_values;  // Form [sindex]
    QVector<QVector<QVector<int>>> tree;  // Form [path possibility][genealogy][leaves]
    QVector<QString> unique_row_buffer;  // Form [value's indentation]. It is initialized with an empty string in 'scan'.
    QVector<QVector<QString>> model_rows;  // Form [row_index][title column, data columns][value]
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
