#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QtSql>
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
    void logger(QString);
    void insert_csv(QString&, QString&);
    void insert_tables(QVector<QString>&, QString);

signals:

public slots:

private slots:

    void on_cB_drives_currentTextChanged(const QString &arg1);

    void on_pB_scan_clicked();

    void on_pB_insert_clicked();

    void on_pB_test_clicked();

    void on_progressBar_valueChanged(int value);

private:
    Ui::MainWindow *ui;
    int cores = 3;
    std::wstring wdrive;
    QString qdrive;
    QReadWriteLock m_executor;
    QMutex m_err, m_log;
    wstring root_directory = L"F:";  // NOTE: REMOVE HARDCODING LATER
    void build_ui_tree(QVector<QVector<QString>>&);
    void add_children(QTreeWidgetItem*, QVector<QString>&);
    QSqlError executor(QString&);
    int insert_cata_tables(QString&);
    void clear_log();
    std::vector<CATALOGUE> binder;
    QString sqlerr_enum(QSqlError::ErrorType);
    void nobles_st(QVector<QVector<QString>>&);
    void subtables_st(CATALOGUE&);
    void subtables_mt(CATALOGUE&);
    void subtables_mapped(CATALOGUE&);
    void populate_cata_tables(int);
};

#endif // MAINWINDOW_H
