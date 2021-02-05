#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QtConcurrent>
#include <QtSql>
#include "threading.h"
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

signals:
    void begin_working();

public slots:
    void feedback();

private slots:

    void on_cB_drives_currentTextChanged(const QString &arg1);

    void on_pB_scan_clicked();

    void on_pB_insert_clicked();

private:
    Ui::MainWindow *ui;
    std::wstring wdrive;
    QString qdrive;
    int bbq;
    void build_tree(QVector<QVector<QString>>&);
    void add_children(QTreeWidgetItem*, QVector<QString>&);
    void prepare_table(QString&);
};
#endif // MAINWINDOW_H
