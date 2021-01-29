#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "DBfunctions.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->comboBox->addItem("D:");
    ui->comboBox->addItem("E:");
    ui->comboBox->addItem("F:");
    ui->comboBox->addItem("G:");

    db = db_initialize("data1");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    /*
    QStringListModel *m_years = new QStringListModel();
    m_years->setStringList(year_list);
    ui->listView->setModel(m_years);
    */
    QStringList year_list = scan_years(wdrive);
    ui->listWidget->addItems(year_list);
}

void MainWindow::on_comboBox_currentTextChanged(const QString &arg1)
{
    wdrive = arg1.toStdWString();
}

void MainWindow::on_pushButton_2_clicked()
{
    int year_count = ui->listWidget->count();
    for (int ii = 0; ii < year_count; ii++)
    {
        ui->listWidget->item(ii)->setSelected(true);
    }
}

void MainWindow::on_pushButton_3_clicked()
{
    QList<QListWidgetItem *> years_to_do = ui->listWidget->selectedItems();
    int year_count = years_to_do.count();
    std::vector<std::wstring> year_folders(year_count);
    QString q_year;
    wstring w_year;
    for (int ii = 0; ii < year_count; ii++)
    {
        q_year = years_to_do[ii]->text();
        w_year = q_year.toStdWString();
        year_folders[ii] = wdrive + L"\\" + w_year;
    }

    treebeard = tree_initialize(db, year_folders);
}
