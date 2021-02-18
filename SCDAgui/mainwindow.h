#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QtSql>
#include "catalogue.h"

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

signals:
    //void report_progress(int);

public slots:
    //void receive_progress(int);

private slots:

    void on_cB_drives_currentTextChanged(const QString &arg1);

    void on_pB_scan_clicked();

    void on_pB_insert_clicked();

    void on_pB_test_clicked();

    void on_pB_benchmark_clicked();

private:
    Ui::MainWindow *ui;
    int cores = 3;
    int bar_threads;
    int jobs_max;
    int jobs_done;
    int jobs_percent;
    std::wstring wdrive;
    QString qdrive;
    QReadWriteLock m_executor;
    QMutex m_err, m_log, m_bar;
    int max_progress;
    wstring root_directory = L"F:";  // NOTE: REMOVE HARDCODING LATER
    QString db_qpath = "F:\\SCDA.db";
    void build_ui_tree(QVector<QVector<QString>>&);
    void add_children(QTreeWidgetItem*, QVector<QString>&);
    QSqlError executor(QString&);
    int insert_cata_tables(QString&);
    void insert_csv_values(CATALOGUE&);
    void clear_log();
    void reset_db(QString&);
    void update_bar();
    void reset_bar(int);
    std::vector<CATALOGUE> binder;
    QString sqlerr_enum(QSqlError::ErrorType);
    void nobles_st(QVector<QVector<QString>>&);
    void subtables_st(CATALOGUE&);
    void subtables_mt(CATALOGUE&);
    void subtables_mapped(CATALOGUE&);
    void populate_cata(int);
    void benchmark1(QString&);
    void update_text_vars(QVector<QVector<QString>>&, QString&);
};

#endif // MAINWINDOW_H
