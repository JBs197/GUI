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
    void initialize(QString&, QString&);
    int create_all_table_statements(QVector<QString>&);

private:
    QString qfile;
    QString qname;
    int subqname = 0;
    bool multi_column;
    QVector<int> indent;
    QMap<int, QString> map_values;  // All maps use the row index from 'row_titles' as their key.
    QMap<int, int> map_isint;
    QVector<QVector<QString>> text_variables;  // Form [row][text var type, text var value]
    QVector<QString> column_titles;
    QVector<QString> row_titles;  // Form [row][title]  including '+' to show space index.
    QVector<QVector<int>> is_int;  // Form [new index][row, bool, sindex]. Negative bool (ha) indicates bad data on file.
    QVector<QString> row_values;  // Form [sindex]
    QVector<QVector<QVector<int>>> subtables;  // Form [space_index][subtable_index][subtable_rows]
    int extract_variables();
    int extract_column_titles(int);
    void extract_rows(int);
    void extract_row_values(int&, QVector<QString>&, QVector<int>&);
    void organize_subtables(int);
    QString subqname_gen();
    int tree_walker(QVector<QVector<QVector<int>>>&, int);

};

#endif // CSV_H
