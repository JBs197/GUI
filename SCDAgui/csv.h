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
    QVector<QVector<QVector<int>>> tree;  // Form [path possibility][genealogy][leaves]
    void scan(QString&, QString&);
    int create_table_all_statements(QVector<QString>&);
    QVector<int> insert_value_all_statements(QVector<QString>&, QVector<QVector<QVector<int>>>&, QVector<QString>&, QVector<QVector<QString>>&);
    void tree_walker();
    void set_gid(QString&);
    void import_model(CSV&);
    QVector<QString> get_subtable_names();
    QVector<QVector<QString>> get_subtable_text_variables();

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
    QVector<QVector<QString>> subtable_text_variables;
    QVector<QString> column_titles;
    QVector<QString> row_titles;  // Form [row]. Title includes '+' to show space index.
    QVector<QVector<int>> is_int;  // Form [new index][row, bool, sindex]. Negative bool (ha) indicates bad data on file.
    QVector<QString> row_values;  // Form [sindex]
    QVector<QVector<QVector<int>>> subtables;  // Form [space_index][subtable_index][subtable_rows]
    QVector<QString> sub_tname_list;  // Form [path possibility]
    int extract_variables();
    int extract_column_titles(int);
    void extract_rows(int);
    void extract_row_values(int&, QVector<QString>&, QVector<int>&);
    void organize_subtables(int);
    QString subqname_gen();
    int is_parent(QVector<QVector<QVector<int>>>&, QVector<int>, int, int);
    void create_subtable_statement(QVector<QString>&, QVector<QVector<int>>&);
    QVector<int> insert_subtable_statement(QVector<QString>&, QVector<QVector<int>>&, QVector<QString>&, QVector<QVector<QString>>&);

};

#endif // CSV_H
