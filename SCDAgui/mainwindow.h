#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QListWidgetItem>
#include <QTreeWidgetItem>
#include <QThreadPool>
#include <QtConcurrent>
#include <QFutureWatcher>
#include "workthreader.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void set_wdrive(wstring wd) { wdrive = wd; }
    std::wstring get_wdrive() { return wdrive; }
    void set_model_string_list(QStringList&);
    void get_columns_from_table(QStringList&, QString&);
    void build_tree(QVector<QVector<QString>>&);
    void add_children(QTreeWidgetItem*, QVector<QString>&);
    PARTS_DB parcel;
public slots:
    void progress();
    void workload(int);

private slots:
    void on_pushButton_Scan_clicked();

    void on_comboBox_drives_currentTextChanged(const QString &arg1);

    void on_pushButton_SelectAll_clicked();

    void on_pushButton_Commit_clicked();

    void on_pushButton_ShowTables_clicked();

    void on_pushButton_ShowColumns_clicked();

    void on_treeWidget_Scan_itemClicked(QTreeWidgetItem *item, int column);

    void on_pushButton_ThrCommit_clicked();

private:
    Ui::MainWindow *ui;
    QSqlDatabase db;
    TREE_DB treebeard;
    std::wstring wdrive;
    QStringListModel mdl;
    int percent = 0;
    int work_done = 0;
    double percent_per_workpiece;
    int total_work = 0;
    //RELAY hub(int percent, int work_done, int total_work);
    QMutex mpb;
};

#endif
