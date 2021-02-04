#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>
#include "threading.h"
#include "basictools.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_cB_drives_currentTextChanged(const QString &arg1);

    void on_pB_scan_clicked();

    void on_pB_insert_clicked();

private:
    Ui::MainWindow *ui;
    std::wstring wdrive;
    void build_tree(QVector<QVector<QString>>&);
    void add_children(QTreeWidgetItem*, QVector<QString>&);
};
#endif // MAINWINDOW_H
