#ifndef CSV_H
#define CSV_H

#include <QObject>
#include "row.h"
#include "basictools.h"

class CSV : public QObject
{
    Q_OBJECT
public:
    explicit CSV(QObject *parent = nullptr) {}
    ~CSV() {}
    void scan(std::wstring&);

private:
    QVector<QVector<QString>> text_variables;
    QVector<QString> column_titles;
    QVector<QVector<bool>> is_int;
    QVector<ROW> rows;
};

#endif // CSV_H
