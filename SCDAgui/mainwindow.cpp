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
    if (!db.open()) { sqlerr("db.open, in MainWindow constructor", db.lastError()); }

    clear_log();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Threadsafe error log function specific to SQL errors.
QString MainWindow::sqlerr_enum(QSqlError::ErrorType type)
{
    QString qtype;
    switch (type)
    {
    case 0:
        qtype = "NoError";
        break;

    case 1:
        qtype = "ConnectionError";
        break;

    case 2:
        qtype = "StatementError";
        break;

    case 3:
        qtype = "TransactionError";
        break;

    case 4:
        qtype = "UnknownError";
        break;
    }
    return qtype;
}

void MainWindow::sqlerr(QString qfunc, QSqlError qerror)
{
    string func8 = qfunc.toStdString();
    string name = utf16to8(root_directory) + "\\SCDA Error Log.txt";
    QString qmessage = qerror.text();
    QString qcode = qerror.nativeErrorCode();
    QSqlError::ErrorType et = qerror.type();
    QString qtype = sqlerr_enum(et);
    string message = timestamperA() + " SQL ERROR: type " + qtype.toStdString() + ", #" + qcode.toStdString() + ", inside " + func8 + "\r\n" + qmessage.toStdString() + "\r\n";
    m_err.lock();
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hprinter == INVALID_HANDLE_VALUE) { qDebug() << "CreateFileA error, in sqlerr, from " << qfunc << ". " << qerror.text(); }
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    if (WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL))
    {
        if (!CloseHandle(hprinter)) { qDebug() << "CloseHandle error, in sqlerr, from " << qfunc << ". " << qerror.text(); }
    }
    else
    {
        qDebug() << "WriteFile error, in sqlerr, from " << qfunc << ". " << qerror.text();
    }
    m_err.unlock();
    exit(EXIT_FAILURE);
}

// Threadsafe logger function for the most recent program runtime.
void MainWindow::logger(QString qnote)
{
    string note8 = qnote.toStdString();
    string name = utf16to8(root_directory) + "\\SCDA Process Log.txt";
    string message = timestamperA() + "  " + note8 + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hprinter == INVALID_HANDLE_VALUE) { warn(L"CreateFile-log"); }
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}

// Delete the previous runtime's log file.
void MainWindow::clear_log()
{
    string name = utf16to8(root_directory) + "\\SCDA Process Log.txt";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hprinter == INVALID_HANDLE_VALUE) { warn(L"CreateFile-refresh_log"); }
    if (!DeleteFileA(name.c_str())) { warn(L"DeleteFile-refresh_log"); }
    if (!CloseHandle(hprinter)) { warn(L"CloseHandle-refresh_log"); }
}

void MainWindow::feedback()
{
    qDebug() << "Report returned from thread: ";
}

void MainWindow::build_ui_tree(QVector<QVector<QString>>& qtree)
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

// (MULTITHREAD HANDLER) Creates a catalogue object with all the column headers, then inserts
// it into the database. Returns the index of the catalogue object in Mainwindow's binder.
int MainWindow::build_cata_tables(QString& qpath)
{
    CATALOGUE cata;
    cata.set_path(qpath);
    cata.initialize_table();
    QVector<QString> stmts = cata.get_create_table_statements(cores);
    qprinter("F:/stmt0.txt", stmts[0]);
    qprinter("F:/stmt1.txt", stmts[1]);
    insert_tables(stmts, cata.get_qname());
    cata.make_name_tree();
    int index = (int)binder.size();
    binder.push_back(cata);
    return index;
}

// (MULTITHREADED) Reads all CSV files for the given catalogue index, and inserts the values into the db as rows.
void MainWindow::populate_cata_tables(int cata_index)
{
    QVector<QString> stmts = binder[cata_index].get_CSV_insert_value_statements(0);  // HARDCODED for first CSV only.
    QString csv_branch;
    for (int ii = 0; ii < stmts.size(); ii++)
    {
        csv_branch = binder[cata_index].get_csv_branch(0);
        insert_csv(stmts[ii], csv_branch);
    }
}

// (SINGLETHREADED) Workhorse function to read a local CSV file and convert it into a SQL statement.
QString MainWindow::read_csv(CATALOGUE& cata, int csv_index)
{
    wstring csv_path = cata.get_csv_path(csv_index);
    wstring wfile = w_memory(csv_path);
    QString aaaa = "aaaaaaa";
    return aaaa;
}

// (MULTITHREADED) Insert a list of new tables/subtables into the database.
void MainWindow::insert_tables(QVector<QString>& stmts, QString qname)
{
    for (int ii = 0; ii < stmts.size(); ii++)
    {
        m_executor.lockForWrite();
        QSqlError qerror = executor(stmts[ii]);
        m_executor.unlock();
        if (qerror.isValid())
        {
            sqlerr("mainwindow.insert_tables for " + qname, qerror);
        }
    }
    logger(qname + " was inserted into the database.");
}

void MainWindow::insert_csv(QString& stmt, QString& csv_branch)
{
    m_executor.lockForWrite();
    QSqlError qerror = executor(stmt);
    m_executor.unlock();
    if (qerror.isValid())
    {
        sqlerr("mainwindow.insert_csv in " + csv_branch, qerror);
    }
    logger(csv_branch + " was inserted into the database.");
}

QSqlError MainWindow::executor(QString& stmt)
{
    QSqlQuery query(db);
    query.prepare(stmt);
    QSqlError qerror1 = query.lastError();
    if (qerror1.isValid())
    {
        return qerror1;
    }
    query.exec();
    QSqlError qerror2 = query.lastError();
    return qerror2;
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

    build_ui_tree(qtree);
}

void MainWindow::on_pB_insert_clicked()
{
    QList<QTreeWidgetItem *> catas_to_do = ui->tW_cata->selectedItems();
    QString qyear = catas_to_do[0]->text(0);
    QString qcata = catas_to_do[0]->text(1);
    QString cata_path = qdrive + "\\" + qyear + "\\" + qcata;
    //int cata_index = build_table(cata_path);
    //log8(qcata.toStdString() + " was assigned binder index " + to_string(cata_index));
    //populate_table(cata_index);

    /*
    qDebug() << "Button was clicked in " << QThread::currentThread();
    THREADING rocinante;
    connect(this, &MainWindow::begin_working, &rocinante, &THREADING::work_order);
    connect(&rocinante, &THREADING::jobsdone, this, &MainWindow::feedback);
    QFuture<void> future = QtConcurrent::run(&THREADING::begin, &rocinante);
    */
}

void MainWindow::on_pB_test_clicked()
{
    QList<QTreeWidgetItem *> catas_to_do = ui->tW_cata->selectedItems();
    QString qyear = catas_to_do[0]->text(0);
    QString qcata = catas_to_do[0]->text(1);
    QString cata_path = qdrive + "\\" + qyear + "\\" + qcata;
    int cata_index = build_cata_tables(cata_path);  // Returns the index in 'binder'.
    populate_cata_tables(cata_index);
}
