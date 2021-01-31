#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QListWidgetItem>
#include "DBclasses.h"

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
    void set_model_string_list() { mdl.setStringList(nex.get_table_list()); }
private slots:
    void on_pushButton_Scan_clicked();

    void on_comboBox_drives_currentTextChanged(const QString &arg1);

    void on_pushButton_SelectAll_clicked();

    void on_pushButton_Commit_clicked();

    void on_pushButton_ShowTables_clicked();

private:
    Ui::MainWindow *ui;
    std::wstring wdrive;
    NEXUS_DB nex;
    QStringListModel mdl;
};
#endif
