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

}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::on_pushButton_clicked()
{
    std::vector<std::wstring> year_folders = get_subfolders(wdrive);
    QStringListModel *m_years = new QStringListModel();
    QStringList year_list;
    for (size_t ii = 0; ii < year_folders.size(); ii++)
    {
        year_list << QString::fromStdWString(year_folders[ii]);
    }
    m_years->setStringList(year_list);
    ui->listView->setModel(m_years);

}

void MainWindow::on_comboBox_currentTextChanged(const QString &arg1)
{
    wdrive = arg1.toStdWString();
}
