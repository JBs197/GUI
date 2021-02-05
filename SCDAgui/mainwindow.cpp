#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->cB_drives->addItem("D:");
    ui->cB_drives->addItem("E:");
    ui->cB_drives->addItem("F:");
    ui->cB_drives->addItem("G:");

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("F:\\SCDA.db");  // NOTE: REMOVE HARDCODING LATER
    if (!db.open()) { sqlerr(L"db.open-MainWindow constructor", db.lastError()); }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::feedback()
{
    qDebug() << "Report returned from thread: ";
}

void MainWindow::build_tree(QVector<QVector<QString>>& qtree)
{
    QVector<QTreeWidgetItem*> qroots;
    QTreeWidgetItem *item;
    for (qsizetype ii = 0; ii < qtree.size(); ii++)
    {
        item = new QTreeWidgetItem(ui->tW_cata);
        item->setText(0, qtree[ii][0]);
        item->setText(1, " ");
        qroots.append(item);
        add_children(item, qtree[ii]);
    }
    ui->tW_cata->addTopLevelItems(qroots);
}

void MainWindow::add_children(QTreeWidgetItem* parent, QVector<QString>& qlist)
{
    QList<QTreeWidgetItem*> branch;
    QTreeWidgetItem *item;
    for (qsizetype ii = 1; ii < qlist.size(); ii++)
    {
        item = new QTreeWidgetItem();
        item->setText(0, qlist[0]);
        item->setText(1, qlist[ii]);
        branch.append(item);
    }
    parent->addChildren(branch);
}

void MainWindow::prepare_table(QString& qpath)
{
    CATALOGUE cata(db);
    cata.insert_table(qpath);
}

void MainWindow::on_cB_drives_currentTextChanged(const QString &arg1)
{
    wdrive = arg1.toStdWString();
    qdrive = arg1;
}

void MainWindow::on_pB_scan_clicked()
{

    std::vector<std::vector<std::wstring>> wtree = get_subfolders2(wdrive);
    QVector<QVector<QString>> qtree;
    wstring wyear, wcata;
    QString qyear, qcata;
    size_t pos1, pos2;
    for (size_t ii = 0; ii < wtree.size(); ii++)
    {
        qtree.append(QVector<QString>());
        pos1 = wtree[ii][0].find(L"\\");
        pos1++;
        pos2 = wtree[ii][0].find(L"\\", pos1);
        wyear = wtree[ii][0].substr(pos1, pos2 - pos1);
        qyear = QString::fromStdWString(wyear);
        qtree[ii].append(qyear);
        for (size_t jj = 0; jj < wtree[ii].size(); jj++)
        {
            pos1 = wtree[ii][jj].rfind(L"\\");
            wcata = wtree[ii][jj].substr(pos1 + 1);
            qcata = QString::fromStdWString(wcata);
            qtree[ii].append(qcata);
        }
    }

    build_tree(qtree);
}

void MainWindow::on_pB_insert_clicked()
{
    QList<QTreeWidgetItem *> catas_to_do = ui->tW_cata->selectedItems();
    QString qyear = catas_to_do[0]->text(0);
    QString qcata = catas_to_do[0]->text(1);
    QString cata_path = qdrive + "\\" + qyear + "\\" + qcata;
    prepare_table(cata_path);

    /*
    qDebug() << "Button was clicked in " << QThread::currentThread();
    THREADING rocinante;
    connect(this, &MainWindow::begin_working, &rocinante, &THREADING::work_order);
    connect(&rocinante, &THREADING::jobsdone, this, &MainWindow::feedback);
    QFuture<void> future = QtConcurrent::run(&THREADING::begin, &rocinante);
    */
}
