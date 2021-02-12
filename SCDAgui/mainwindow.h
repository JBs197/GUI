#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QtConcurrent>
#include <QtSql>
//#include "threading.h"
#include "catalogue.h"
#include "basictools.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QSqlDatabase db;
    void sqlerr(QString, QSqlError);
    void logger(QString&);
    void insert_csv(QString&, QString&);
    void insert_tables(QVector<QString>&, QString&);

signals:
    void begin_working();

public slots:
    void feedback();

private slots:

    void on_cB_drives_currentTextChanged(const QString &arg1);

    void on_pB_scan_clicked();

    void on_pB_insert_clicked();

    void on_pB_test_clicked();

private:
    Ui::MainWindow *ui;
    std::wstring wdrive;
    QString qdrive;
    QReadWriteLock m_executor;
    QMutex m_err, m_log;
    wstring root_directory = L"F:";  // NOTE: REMOVE HARDCODING LATER
    void build_tree(QVector<QVector<QString>>&);
    void add_children(QTreeWidgetItem*, QVector<QString>&);
    void executor(QString&, QSqlError&);
    int build_cata_tables(QString&);
    void populate_cata_tables(int);
    void clear_log();
    QString read_csv(CATALOGUE&, int);
    std::vector<CATALOGUE> binder;
};

#endif // MAINWINDOW_H
