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

    void on_pB_viewdata_clicked();

private:
    Ui::MainWindow *ui;
    int cores = 3;
    int bar_threads;
    int jobs_max;
    int jobs_done;
    int jobs_percent;
    int threads_working = 0;
    int connection_count = 0;
    std::wstring wdrive;
    QString qdrive;
    QMutex m_db, m_err, m_log, m_bar;
    int max_progress;
    wstring root_directory = L"F:";  // NOTE: REMOVE HARDCODING LATER
    QString db_qpath = "F:\\SCDA.db";
    QVector<QVector<QVector<QString>>> cata_tree;  // Form [year][catalogue][qyear, qname, qdescription].
    QMap<QString, int> map_tree_year;  // For a given qyear, return that cata_tree index.
    QMap<QString, int> map_tree_cata;  // For a given qname, return that cata_tree index.
    void build_ui_tree(QVector<QVector<QVector<QString>>>&, int);
    void add_children(QTreeWidgetItem*, QVector<QVector<QString>>&);
    void executor(QSqlQuery&);
    QSqlError executor_select(QString&, QSqlQuery&);
    void initialize_catalogue(CATALOGUE&, QString&, QString&);
    void bbq();
    QString name_gen();
    void clear_log();
    void reset_db(QString&);
    void update_bar();
    void reset_bar(int, QString);
    void initialize();
    QString sqlerr_enum(QSqlError::ErrorType);
    void nobles_st(CATALOGUE&);
    void subtables_mt(CATALOGUE&);
    void subtables_mapped(CATALOGUE&);
    void populate_cata_mt(int);
    void update_text_vars(QVector<QVector<QString>>&, QString&);
    void update_cata_tree();
    void create_cata_index_table();
    void insert_cata_index(CATALOGUE&);
    void finish_cata_index(CATALOGUE&);
    void insert_cata_csvs_mt(CATALOGUE&);
    void create_csv_tables(CATALOGUE&, QString);
    void populate_csv_tables(CATALOGUE&, int);
    void insert_primary_row(QSqlQuery&, CATALOGUE&, QString&, QVector<QVector<QString>>&, QVector<QVector<QString>>&);
    void create_insert_csv_table(QSqlQuery&, CATALOGUE&, QString&, QVector<QVector<QString>>&);
    void create_insert_csv_subtables(QSqlQuery&, CATALOGUE&, QString&, QVector<QVector<QString>>&);
    void all_cata_db(QVector<QVector<QString>>&, QMap<QString, int>&);
};

#endif // MAINWINDOW_H
