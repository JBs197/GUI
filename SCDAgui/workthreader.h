#ifndef WORKTHREADER_H
#define WORKTHREADER_H

#include <QThread>
#include <QObject>
#include <QtConcurrent>
#include "DBclasses.h"
#include "basictools.h"

class WORKTHREADER : public QObject
{
    Q_OBJECT

private:
    QSqlDatabase& db;
    int type;  // type 1 = add selected catalogue(s) to the database.
    QStringList qlist;
    vector<wstring> wlist;  // List of catalogue folder paths to be worked on.
    wstring folder;  // Currently active catalogue's full path.
    string cata_number;  // Stats Can's alphanumeric designation for the currently active catalogue.
    vector<bool> is_int; // Its length is equal to the number of columns in the active catalogue.
    vector<string> column_header; // Descriptor title for each value in the 1D CSV row.
    vector<vector<string>> variables; // Form [variable][variable type, variable shown in CSV]
    int init_cata();
    void prepare_table();
public:
    explicit WORKTHREADER(QSqlDatabase& datab, QObject *parent = nullptr);
    ~WORKTHREADER() {}
    void begin(int, int&);
    void qpreload(QStringList&);
    void wpreload(vector<wstring>&);
    void get_CSV_paths(QStringList&);
    void populate_table(int);
    void add_CSV(QString);
    void add_cata();

public slots:


signals:
    void jobsdone();
    void setgoal(int);
};

#endif
