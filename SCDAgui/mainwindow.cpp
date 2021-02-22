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
    ui->progressBar->setValue(0);
    ui->QL_bar->setText("");

    initialize();
}

MainWindow::~MainWindow()
{
    delete ui;
}


// MULTI-PURPOSE FUNCTIONS:

// Perform a variety of tasks from the MainWindow constructor:
// 1. Connect the database memory object with a pre-existing local database file, if it exists.
// 2. Display (in tree form) the database's current repository of catalogues on the GUI.
// 3. Begin a new process log for this runtime session.
void MainWindow::initialize()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(db_qpath);
    if (!db.open()) { sqlerr("db.open-initialize", db.lastError()); }
    create_cata_index_table();  // Even if starting a new database, this meta-table will always exist.

    update_cata_tree();
    build_ui_tree(cata_tree, 2);

    clear_log();
    logger("MainWindow initialized.");
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

// Progress bar related functions.
void MainWindow::update_bar()
{
    if (jobs_max >= 0 && jobs_done >= 0)
    {
        jobs_percent = 100 * jobs_done / jobs_max;
        ui->progressBar->setValue(jobs_percent);
    }
    else
    {
        warn(L"Tried to update the progress bar before resetting it.");
    }
}
void MainWindow::reset_bar(int max, QString status)
{
    QMutexLocker lock(&m_bar);
    jobs_done = 0;
    jobs_max = max;
    ui->progressBar->setValue(0);
    ui->QL_bar->setText(status);
}

// Functions for the meta-table 'TCatalogueIndex'.
void MainWindow::create_cata_index_table()
{
    QString stmt = "CREATE TABLE IF NOT EXISTS TCatalogueIndex ( Year TEXT, ";
    stmt += "Name TEXT, Description TEXT );";
    m_executor.lockForWrite();
    QSqlError qerror = executor(stmt);
    m_executor.unlock();
    if (qerror.isValid())
    {
        sqlerr("executor-mainwindow.create_cata_index_table", qerror);
    }
}
void MainWindow::insert_cata_index(CATALOGUE& cata)
{
    QString cata_tname = "[" + cata.get_qname() + "]";
    QString cata_year = cata.get_year();
    QString cata_desc = "Incomplete";  // All catalogues being inserted begin with this.

    bool fine;
    QSqlQuery q(db);
    q.prepare("INSERT INTO TCatalogueIndex ( Year, Name, Description ) VALUES (?, ?, ?)");
    q.addBindValue(cata_year);
    q.addBindValue(cata_tname);
    q.addBindValue(cata_desc);
    fine = q.exec();
    if (!fine) { sqlerr("insert into-insert_cata_index", q.lastError()); }


    /*
    QString stmt = "INSERT INTO TCatalogueIndex ( Year, Name, Description ) VALUES ( ";
    stmt += cata_year + ", [" + cata_tname + "], " + cata_desc + " );";
    m_executor.lockForWrite();
    QSqlError qerror = executor(stmt);
    m_executor.unlock();
    if (qerror.isValid())
    {
        sqlerr("executor-mainwindow.insert_cata_index", qerror);
    }
    */
}
void MainWindow::finish_cata_index(CATALOGUE& cata)
{
    QString cata_tname = "T" + cata.get_qname();
    //QString cata_year = cata.get_year();
    QString cata_desc = cata.get_description();
    qclean(cata_desc, 1);
    QString stmt = "UPDATE TCatalogueIndex SET Description = '";
    stmt += cata_desc + "' WHERE Name = '" + cata_tname + "';";
    m_executor.lockForWrite();
    QSqlError qerror = executor(stmt);
    m_executor.unlock();
    if (qerror.isValid())
    {
        sqlerr("executor-mainwindow.insert_cata_index", qerror);
    }
}

// Close the db object, delete the db file, and remake the db file with a new CatalogueIndex table.
void MainWindow::reset_db(QString& db_path)
{
    db.close();
    db.removeDatabase("qt_sql_default_connection");
    delete_file(db_path.toStdWString());
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(db_path);
    if (!db.open()) { sqlerr("db.open-mainwindow.reset_db", db.lastError()); }    
    create_cata_index_table();
    logger("Database was reset.");
}

// For a perpared matrix of QString data, display it on the GUI as a 2D tree widget (year -> cata entry).
void MainWindow::build_ui_tree(QVector<QVector<QVector<QString>>>& qtree, int window)
{
    QTreeWidgetItem *item;
    QVector<QTreeWidgetItem*> qroots;
    switch (window)  // Window:  1 = Catalogues on Drive, 2 = Catalogues in Database
    {
    case 1:
        ui->TW_cataondrive->clear();
        for (int ii = 0; ii < qtree.size(); ii++)
        {
            item = new QTreeWidgetItem(ui->TW_cataondrive);  // For every year, make a root pointer.
            item->setText(0, qtree[ii][0][0]);
            item->setText(1, " ");
            auto item_flags = item->flags();
            item_flags.setFlag(Qt::ItemIsSelectable, false);
            item->setFlags(item_flags);
            qroots.append(item);
            add_children(item, qtree[ii]);
        }
        ui->TW_cataondrive->addTopLevelItems(qroots);
        break;
    case 2:
        ui->TW_cataindb->clear();
        for (int ii = 0; ii < qtree.size(); ii++)
        {
            item = new QTreeWidgetItem(ui->TW_cataindb);  // For every year, make a root pointer.
            item->setText(0, qtree[ii][0][0]);
            item->setText(1, " ");
            item->setText(2, " ");
            auto item_flags = item->flags();
            item_flags.setFlag(Qt::ItemIsSelectable, false);
            item->setFlags(item_flags);
            qroots.append(item);
            add_children(item, qtree[ii]);
        }
        ui->TW_cataindb->addTopLevelItems(qroots);
        break;
    }
}
void MainWindow::add_children(QTreeWidgetItem* parent, QVector<QVector<QString>>& cata_list)
{
    QList<QTreeWidgetItem*> branch;
    QTreeWidgetItem *item;
    for (int ii = 0; ii < cata_list.size(); ii++)  // For every catalogue in the list...
    {
        item = new QTreeWidgetItem();
        for (int jj = 0; jj < cata_list[ii].size(); jj++)  // For every column we want to display...
        {
            item->setText(jj, cata_list[ii][jj]);
        }
        branch.append(item);
    }
    parent->addChildren(branch);
}

// For a given CSV qfile, extract the Stats Canada 'text variables'.
void MainWindow::update_text_vars(QVector<QVector<QString>>& text_vars, QString& qf)
{
    int pos1, pos2;
    int var_index = -1;
    QChar math;
    QString temp1, temp2;
    pos1 = 0;
    pos2 = 0;
    while (1)
    {
        pos1 = qf.indexOf('=', pos1 + 1);
        if (pos1 < 0) { break; }  // Primary loop exit.
        math = qf[pos1 - 1];
        if (math == '<' || math == '>') { continue; }

        var_index++;
        pos2 = qf.lastIndexOf('"', pos1); pos2++;
        temp1 = qf.mid(pos2, pos1 - pos2);
        qclean(temp1, 0);
        if (temp1 != text_vars[var_index][0])
        {
            err8("text variable type check-update_text_vars");
        }

        pos2 = qf.indexOf('"', pos1);
        temp1 = qf.mid(pos1 + 1, pos2 - pos1 - 1);
        qclean(temp1, 1);
        text_vars[var_index][1] = temp1;
    }
}

// Takes a prepared SQL statement and executes it. Returns the SQL error generated (can be null).
QSqlError MainWindow::executor(QString& stmt)
{
    QSqlQuery query(db);
    query.prepare(stmt);
    QSqlError qerror = query.lastError();
    if (qerror.isValid())
    {
        return qerror;
    }
    query.exec();
    qerror = query.lastError();
    return qerror;
}
QSqlError MainWindow::executor_select(QString& stmt, QSqlQuery& mailbox)
{
    mailbox.clear();
    mailbox.prepare(stmt);
    //qDebug() << "Statement: " << stmt;
    QSqlError qerror = mailbox.lastError();
    if (qerror.isValid())
    {
        return qerror;
    }
    mailbox.exec();
    qerror = mailbox.lastError();
    return qerror;
}

// Update the catalogue tree and the tree's maps by reading from the database.
void MainWindow::update_cata_tree()
{
    cata_tree.clear();
    QString stmt = "SELECT DISTINCT Year FROM TCatalogueIndex;";
    QSqlQuery mailbox(db);
    QSqlError qerror = executor_select(stmt, mailbox);
    if (qerror.isValid())
    {
        sqlerr("select distinct year-MainWindow.update_cata_tree", qerror);
    }
    QSqlRecord rec = mailbox.record();
    int tree_index, name_index, year_index, desc_index;
    QVector<QString> temp;
    QString qname, qyear, qdesc;

    // Create the map connecting qyear to tree's first index.
    cata_tree.resize(rec.count(), QVector<QVector<QString>>());
    year_index = 0;
    while (mailbox.next())
    {
        qyear = mailbox.value(0).toString();
        map_tree_year.insert(qyear, year_index);
        year_index++;
    }

    // Populate the tree, mapping catalogue names as we go.
    stmt = "SELECT * FROM TCatalogueIndex";
    qerror = executor_select(stmt, mailbox);
    if (qerror.isValid())
    {
        sqlerr("select wildcard-MainWindow.update_cata_tree", qerror);
    }
    rec = mailbox.record();
    year_index = rec.indexOf("Year");
    name_index = rec.indexOf("Name");
    desc_index = rec.indexOf("Description");
    do               // For each catalogue in the database...
    {
        qname = mailbox.value(name_index).toString();
        qyear = mailbox.value(year_index).toString();
        qdesc = mailbox.value(desc_index).toString();
        temp = { qyear, qname, qdesc };
        tree_index = map_tree_year.value(qyear);
        map_tree_cata.insert(qname, cata_tree[tree_index].size());
        cata_tree[tree_index].append(temp);
    } while (mailbox.next());

}


// DEBUG FUNCTIONS:
void MainWindow::bbq()
{
    bool fine;
    QString qyear, qname, qdesc;
    QSqlQuery q(db);
    q.prepare("SELECT * FROM TCatalogueIndex");
    fine = q.exec();
    if (!fine) { sqlerr("select * -bbq", q.lastError()); }
    while (q.next())
    {
        qyear = q.value(0).toString();
        qname = q.value(1).toString();
        qdesc = q.value(2).toString();
        qDebug() << "Year, name, desc: " << qyear << ", " << qname << ", " << qdesc;
    }
    int barbeque = 1;
}


// TASK FUNCTIONS, USED BY ONE OR MORE GUI BUTTONS:

// For a given catalogue, populate its memory object with all values shared amongst GIDs.
// Then, create its primary table in the database.
void MainWindow::initialize_catalogue(CATALOGUE& cata, QString& qy, QString& qn)
{
    // Load the catalogue object with values to be shared between different CSVs.
    QString cata_path = QString::fromStdWString(root_directory);
    cata_path += "\\" + qy + "\\" + qn;
    cata.set_path(cata_path);
    cata.initialize_table();
    QString primary_stmt = cata.create_primary_table();
    cata.insert_primary_columns_template();
    cata.create_csv_tables_template();
    cata.insert_csv_row_template();

    m_executor.lockForWrite();
    QSqlError qerror = executor(primary_stmt);
    m_executor.unlock();
    if (qerror.isValid())
    {
        sqlerr("create primary table-mainwindow.initialize_catalogue for " + qn, qerror);
    }
    cata.print_stuff();
}

// For a given catalogue, insert all of its CSVs into the database, iterating by GID.
void MainWindow::insert_cata_csvs_mt(CATALOGUE& cata)
{
    QVector<QString> gid_list = cata.get_gid_list();
    int num_gid = gid_list.size();
    int workload = num_gid / cores;
    int bot = 0;
    int top = workload - 1;
    int report = 0;
    //double gids_per_percent = (double)num_gid / 100.0;
    reset_bar(num_gid, "Adding spreadsheets to catalogue   " + cata.get_qname());

    auto insert_csvs = [=] (CATALOGUE& cata, int bot, int top, int& report)
    {
        double gids_done = 0.0;
        QString gid, qfile;
        wstring csv_path;
        QVector<QVector<QString>> text_vars, data_rows;
        for (int ii = bot; ii <= top; ii++)  // Iterating by CSV...
        {
            gid = cata.get_gid(ii);
            csv_path = cata.get_csv_path(ii);
            qfile = q_memory(csv_path);
            text_vars = cata.extract_text_vars(qfile);
            data_rows = cata.extract_data_rows(qfile);

            insert_primary_row(cata, gid, text_vars, data_rows);
            create_insert_csv_table(cata, gid, data_rows);
            create_insert_csv_subtables(cata, gid, data_rows);

            m_bar.lock();
            report++;
            m_bar.unlock();
            /*
            gids_done += 1.0;
            if (gids_done >= gids_per_percent)
            {
                m_bar.lock();
                report++;
                m_bar.unlock();
                gids_done -= gids_per_percent;
            }
            */
        }
    };

    insert_cata_index(cata);  // Add this catalogue to the meta-table, with an 'incomplete' description.
    vector<std::thread> tapestry;
    threads_working = 1;
    for (int ii = 0; ii < cores; ii++)
    {
        if (ii < cores - 1)
        {
            std::thread thr(insert_csvs, std::ref(cata), bot, top, std::ref(report));
            tapestry.push_back(std::move(thr));
            bot += workload;
            top += workload;
        }
        else
        {
            std::thread thr(insert_csvs, std::ref(cata), bot, num_gid - 1, std::ref(report));
            tapestry.push_back(std::move(thr));
        }
    }
    while (threads_working)
    {
        jobs_done = report;
        update_bar();
        QCoreApplication::processEvents();
        if (report >= num_gid)
        {
            jobs_done = report;
            update_bar();
            threads_working = 0;
        }
    }
    for (auto& th : tapestry)
    {
        if (th.joinable())
        {
            th.join();
        }
    }
    finish_cata_index(cata);  // Mark this entry as complete in the meta-table.
    logger("Inserted all data for catalogue " + cata.get_qname());
}
void MainWindow::create_csv_tables(CATALOGUE& cata, QString gid)
{
    QVector<QVector<QVector<int>>> csv_tree = cata.get_tree();
    QString csv_template = cata.get_csv_template();
    QString qname = cata.get_qname();
    QString stmt, tname;
    int pos1, pos2;

    // Create the full CSV table.
    stmt = csv_template;
    tname = "T" + qname + "$" + gid;
    pos1 = stmt.indexOf("!!!");
    stmt.replace(pos1, 3, tname);
    m_executor.lockForWrite();
    QSqlError qerror = executor(stmt);
    m_executor.unlock();
    if (qerror.isValid())
    {
        sqlerr("mainwindow.create_csv_tables for " + tname, qerror);
    }

    // Create all the CSV subtables.
    for (int ii = 0; ii < csv_tree.size(); ii++)
    {
        stmt = csv_template;
        tname = cata.sublabelmaker(gid, csv_tree[ii]);
        pos1 = stmt.indexOf("!!!");
        stmt.replace(pos1, 3, tname);
        m_executor.lockForWrite();
        QSqlError qerror = executor(stmt);
        m_executor.unlock();
        if (qerror.isValid())
        {
            sqlerr("mainwindow.create_csv_tables for " + tname, qerror);
        }
    }
}
void MainWindow::populate_csv_tables(CATALOGUE& cata, int csv_index)
{
    wstring csv_path = cata.get_csv_path(csv_index);
    QString qfile = q_memory(csv_path);
    QString gid = cata.get_gid(csv_index);
    QString qname = cata.get_qname();
    QString tname;
    QVector<QVector<QString>> text_vars = cata.extract_text_vars(qfile);
    QVector<QVector<QString>> rows = cata.extract_data_rows(qfile);
    int pos1;
    QSqlQuery query;

    // Insert this CSV's values into the catalogue's primary table.
    QString stmt = cata.get_primary_template();
    pos1 = stmt.lastIndexOf("VALUES");
    pos1 = stmt.indexOf('?', pos1);
    pos1 = insert_val(stmt, pos1, gid);  // GID is always first.
    for (int ii = 0; ii < text_vars.size(); ii++)
    {
        pos1 = insert_val(stmt, pos1, text_vars[ii][1]);
    }
    for (int ii = 0; ii < rows.size(); ii++)
    {
        for (int jj = 1; jj < rows[ii].size(); jj++)
        {
            pos1 = insert_val(stmt, pos1, rows[ii][jj]);
        }
    }
    m_executor.lockForWrite();
    QSqlError qerror = executor(stmt);
    m_executor.unlock();
    if (qerror.isValid())
    {
        sqlerr("mainwindow.populate_csv_tables for T" + qname, qerror);
    }

    // Insert this CSV's values into its full table.
    stmt = cata.get_csv_template();
    pos1 = stmt.indexOf("!!!");
    tname = "T" + qname + "$" + gid;
    stmt.replace(pos1, 3, tname);
    pos1 = stmt.lastIndexOf("VALUES");
    pos1 = stmt.indexOf('?', pos1);
    for (int ii = 0; ii < text_vars.size(); ii++)
    {
        pos1 = insert_val(stmt, pos1, text_vars[ii][1]);
    }
    for (int ii = 0; ii < rows.size(); ii++)
    {
        for (int jj = 1; jj < rows[ii].size(); jj++)
        {
            pos1 = insert_val(stmt, pos1, rows[ii][jj]);
        }
    }
    m_executor.lockForWrite();
    qerror = executor(stmt);
    m_executor.unlock();
    if (qerror.isValid())
    {
        sqlerr("mainwindow.populate_csv_tables for T" + qname, qerror);
    }

    // Insert this CSV's values into its subtables.


}
void MainWindow::insert_primary_row(CATALOGUE& cata, QString& gid, QVector<QVector<QString>>& text_vars, QVector<QVector<QString>>& data_rows)
{
    QString stmt = cata.get_primary_template();
    int pos1 = stmt.lastIndexOf("VALUES");
    pos1 = stmt.indexOf('?', pos1);
    pos1 = insert_val(stmt, pos1, gid);  // First value is always GID.
    for (int ii = 0; ii < text_vars.size(); ii++)
    {
        pos1 = insert_val(stmt, pos1, text_vars[ii][1]);  // Second category is the text variable values.
    }
    for (int ii = 0; ii < data_rows.size(); ii++)
    {
        for (int jj = 1; jj < data_rows[ii].size(); jj++)
        {
            pos1 = insert_val(stmt, pos1, data_rows[ii][jj]);  // Third category is the data row values,
        }                                                      // NOT including the row titles.
    }
    m_executor.lockForWrite();
    QSqlError qerror = executor(stmt);
    m_executor.unlock();
    if (qerror.isValid())
    {
        qprinter("F:\\stmt_debug.txt", stmt);
        sqlerr("mainwindow.insert_primary_row for T" + cata.get_qname() + ", GID " + gid, qerror);
    }
}
void MainWindow::create_insert_csv_table(CATALOGUE& cata, QString& gid, QVector<QVector<QString>>& data_rows)
{
    // Create this CSV's full table, starting from the template.
    QString stmt = cata.get_csv_template();
    QString qname = cata.get_qname();
    int pos1 = stmt.indexOf("!!!");
    QString tname = "T" + qname + "$" + gid;
    stmt.replace(pos1, 3, tname);
    m_executor.lockForWrite();
    QSqlError qerror = executor(stmt);
    m_executor.unlock();
    if (qerror.isValid())
    {
        sqlerr("create table-mainwindow.create_insert_csv_table for " + tname, qerror);
    }

    // Insert the full table's data rows, one by one, starting from the template each time.
    QString stmt0 = cata.get_insert_csv_row_template();
    pos1 = stmt0.indexOf("!!!");
    stmt0.replace(pos1, 3, tname);
    for (int ii = 0; ii < data_rows.size(); ii++)
    {
        stmt = stmt0;
        pos1 = stmt.lastIndexOf("VALUES");
        pos1 = stmt.indexOf('?', pos1 + 1);
        for (int jj = 0; jj < data_rows[ii].size(); jj++)
        {
            pos1 = insert_val(stmt, pos1, data_rows[ii][jj]);
        }
        m_executor.lockForWrite();
        qerror = executor(stmt);
        m_executor.unlock();
        if (qerror.isValid())
        {
            sqlerr("insert row-mainwindow.create_insert_csv_table for " + tname, qerror);
        }
    }
}
void MainWindow::create_insert_csv_subtables(CATALOGUE& cata, QString& gid, QVector<QVector<QString>>& data_rows)
{
    QString qname = cata.get_qname();
    QVector<QVector<QVector<int>>> tree = cata.get_tree();
    QString create_subtable_template = cata.get_create_sub_template();
    QString ins_csv_row_template = cata.get_insert_csv_row_template();
    QSqlError qerror;
    QString stmt, stmt0, tname;
    int pos1, num_rows, row_index;

    // Iterate by subtable.
    for (int ii = 0; ii < tree.size(); ii++)
    {
        tname = cata.sublabelmaker(gid, tree[ii]);
        num_rows = tree[ii][1].size() + 1;  // Top row is the parent, subsequent rows are direct children.

        // Create the subtable.
        stmt = create_subtable_template;
        pos1 = stmt.indexOf("!!!");
        stmt.replace(pos1, 3, tname);
        m_executor.lockForWrite();
        qerror = executor(stmt);
        m_executor.unlock();
        if (qerror.isValid())
        {
            sqlerr("create table-mainwindow.create_insert_csv_table for " + tname, qerror);
        }

        // Insert the subtable's rows.
        stmt0 = ins_csv_row_template;
        pos1 = stmt0.indexOf("!!!");
        stmt0.replace(pos1, 3, tname);
        for (int jj = 0; jj < num_rows; jj++)
        {
            stmt = stmt0;
            pos1 = stmt.indexOf('?');
            if (jj == 0)
            {
                row_index = tree[ii][0][tree[ii][0].size() - 1];
            }
            else
            {
                row_index = tree[ii][1][jj - 1];
            }
            for (int kk = 0; kk < data_rows[0].size(); kk++)
            {
                pos1 = insert_val(stmt, pos1, data_rows[row_index][kk]);
            }
            m_executor.lockForWrite();
            qerror = executor(stmt);
            m_executor.unlock();
            if (qerror.isValid())
            {
                sqlerr("insert row-mainwindow.create_insert_csv_subtables for " + tname, qerror);
            }
        }
    }
}

// Creates all database sub-CSV subtables for a catalogue.
// st = single-threaded, mt = multithreaded, mapped = Qt concurrent
void MainWindow::subtables_mt(CATALOGUE& cat)
{
    QVector<QVector<QVector<int>>> tree = cat.get_tree();
    QVector<QString> gid_list = cat.get_gid_list();
    QString sub_template = cat.get_create_sub_template();
    QString qname = cat.get_qname();
    int num_gid = gid_list.size();
    int workload = gid_list.size() / cores;
    int bot = 0;
    int top = workload - 1;
    reset_bar(100, "Creating subtables for " + qname + "...");
    int report = 0;
    double gids_per_percent = (double)num_gid / 100.0;

    auto insert_subtable_gid = [=] (int bot, int top, int& report)  // Interval is inclusive.
    {
        QString name, stmt, temp;
        int pos1, ancestry, cheddar;
        double gids_done = 0.0;
        for (int ii = bot; ii <= top; ii++)
        {
            for (int jj = 0; jj < tree.size(); jj++)
            {
                name = "T" + qname + "$" + gid_list[ii];
                ancestry = tree[jj][0].size();
                cheddar = 2;
                for (int ii = 0; ii < ancestry; ii++)
                {
                    for (int jj = 0; jj < cheddar; jj++)
                    {
                        name += "$";
                    }
                    cheddar++;
                    temp = QString::number(tree[jj][0][ii]);
                    name += temp;
                }

                stmt = sub_template;
                pos1 = stmt.indexOf("!!!");
                stmt.replace(pos1, 3, name);

                m_executor.lockForWrite();
                QSqlError qerror = executor(stmt);
                m_executor.unlock();
                if (qerror.isValid())
                {
                    sqlerr("mainwindow.subtables_mt for " + name, qerror);
                }
            }
            gids_done += 1.0;
            if (gids_done >= gids_per_percent)
            {
                m_bar.lock();
                report++;
                m_bar.unlock();
                gids_done -= gids_per_percent;
            }
        }
    };

    vector<std::thread> tapestry;
    for (int ii = 0; ii < cores; ii++)
    {
        if (ii < cores - 1)
        {
            std::thread thr(insert_subtable_gid, bot, top, ref(report));
            tapestry.push_back(std::move(thr));
            bot += workload;
            top += workload;
        }
        else
        {
            std::thread thr(insert_subtable_gid, bot, gid_list.size() - 1, ref(report));
            tapestry.push_back(std::move(thr));
        }
    }
    while (report < 100)
    {
        jobs_done = report;
        ui->progressBar->setValue(jobs_done);
        QCoreApplication::processEvents();
    }
    for (auto& th : tapestry)
    {
        if (th.joinable())
        {
            th.join();
        }
    }

    logger("Inserted all tables for catalogue " + cat.get_qname());
}


// GUI-SPECIFIC FUNCTIONS, LINKED TO A SINGLE GUI ELEMENT:

// Choose a local drive to examine for spreadsheets.
void MainWindow::on_cB_drives_currentTextChanged(const QString &arg1)
{
    wdrive = arg1.toStdWString();
    qdrive = arg1;
}

// For the given local drive, display (as a tree widget) the available catalogues, organized by year.
void MainWindow::on_pB_scan_clicked()
{
    std::vector<std::vector<std::wstring>> wtree = get_subfolders2(wdrive);
    QVector<QVector<QVector<QString>>> qtree;  // Form [year][catalogue][year, name]
    wstring wyear, wcata;
    QString qyear, qcata;
    size_t pos1, pos2;
    for (size_t ii = 0; ii < wtree.size(); ii++)  // For every year...
    {
        qtree.append(QVector<QVector<QString>>());
        pos1 = wtree[ii][0].find(L"\\");
        pos1++;
        pos2 = wtree[ii][0].find(L"\\", pos1);
        wyear = wtree[ii][0].substr(pos1, pos2 - pos1);
        qyear = QString::fromStdWString(wyear);
        for (size_t jj = 0; jj < wtree[ii].size(); jj++)  // For every catalogue in that year...
        {
            qtree[ii].append(QVector<QString>());
            pos1 = wtree[ii][jj].rfind(L"\\");
            wcata = wtree[ii][jj].substr(pos1 + 1);
            qcata = QString::fromStdWString(wcata);
            qtree[ii][jj].append(qyear);
            qtree[ii][jj].append(qcata);
        }
    }
    build_ui_tree(qtree, 1);  // Window code 1 will populate the 'Catalogues on Drive' section.
}

// Insert the selected catalogues into the database.
void MainWindow::on_pB_insert_clicked()
{
    QList<QTreeWidgetItem *> catas_to_do = ui->TW_cataondrive->selectedItems();
    QString qyear, qname, qdesc;
    int year_index, cata_index;
    for (int ii = 0; ii < catas_to_do.size(); ii++)  // For each catalogue selected...
    {
        CATALOGUE cata;
        qyear = catas_to_do[ii]->text(0);
        qname = catas_to_do[ii]->text(1);
        cata_index = map_tree_cata.value(qname, -1);
        if (cata_index < 0)  // Insert the entire catalogue into the database.
        {
            initialize_catalogue(cata, qyear, qname);
            insert_cata_csvs_mt(cata);
        }
        /*
        else
        {
            year_index = map_tree_year.value(qyear);
            qdesc = cata_tree[year_index][cata_index][2];
            if (qdesc == "Incomplete")  // Determine which CSVs are missing, then insert them.
            {

            }
        }
        */

    }
    update_cata_tree();
    build_ui_tree(cata_tree, 2);
}

// (Debug function) Display some information.
void MainWindow::on_pB_test_clicked()
{
    QString stmt = "SELECT name FROM sqlite_master WHERE type='table' AND name='TCatalogueIndex';";
    QSqlQuery q(db);
    executor_select(stmt, q);
    while (q.next())
    {
        qDebug() << "List of tables: " << q.value(0).toString();
    }
    QString cata_year = "2005";
    QString cata_tname = "[T97-570-X1981005]";
    QString cata_desc = "Incomplete";
    q.clear();
    q.prepare("INSERT INTO TCatalogueIndex (Year, Name, Description) VALUES (?, ?, ?)");
    q.addBindValue(cata_year);
    q.addBindValue(cata_tname);
    q.addBindValue(cata_desc);
    q.exec();

    q.clear();
    q.prepare("SELECT * FROM TCatalogueIndex");
    q.exec();

    /*
    stmt = "INSERT INTO TCatalogueIndex ( Year, Name, Description ) VALUES ( ";
    stmt += cata_year + ", " + cata_tname + ", " + cata_desc + " );";
    QSqlError qerror = executor(stmt);
    if (qerror.isValid())
    {
        sqlerr("executor-mainwindow.insert_cata_index", qerror);
    }
    stmt = "SELECT * FROM TCatalogueIndex;";
    qerror = executor_select(stmt, q);
    if (qerror.isValid())
    {
        sqlerr("executor-mainwindow.insert_cata_index", qerror);
    }
    */
    while (q.next())
    {
        qDebug() << "Col 0: " << q.value(0).toString();
        qDebug() << "Col 1: " << q.value(1).toString();
        qDebug() << "Col 2: " << q.value(2).toString();
    }
    int bbq = 1;
}

// (Debug function) Perform a series of actions to test new functions.
void MainWindow::on_pB_benchmark_clicked()
{
    wdrive = L"F:";
    qdrive = "F:";
    QString cata_path = "F:\\3067\\97-570-X1981005";
    reset_db(db_qpath);
    CATALOGUE cata;
    QString qyear = "3067";
    QString qname = "97-570-X1981005";
    initialize_catalogue(cata, qyear, qname);
    insert_cata_csvs_mt(cata);
    insert_cata_index(cata);
    update_cata_tree();
    build_ui_tree(cata_tree, 2);
    int bbq = 1;
}

// Display the 'tabbed data' for the selected catalogue.
void MainWindow::on_pB_viewdata_clicked()
{
    QSqlQuery mailbox(db);
    QStringList geo_list;
    QList<QTreeWidgetItem *> cata_to_do = ui->TW_cataindb->selectedItems();  // Only 1 catalogue can be selected.
    QString qyear = cata_to_do[0]->text(0);
    QString qname = cata_to_do[0]->text(1);

    /*
    QSqlQuery q2("SELECT *");
    QSqlRecord rec = q2.record();
    int rc = rec.count();
    int num = rec.indexOf("Geography");
    while (q2.next())
    {
        qDebug() << q2.value(num).toString();
    }
    */

    QString stmt = "SELECT Geography FROM '" + qname + "';";
    // Trying to read without a mutex. That's smart...
    QSqlError qerror = executor_select(stmt, mailbox);
    if (qerror.isValid())
    {
        sqlerr("executor-mainwindow.on_pB_viewdata_clicked", qerror);
    }
    while (mailbox.next())
    {
        geo_list.append(mailbox.value("Geography").toString());
    }


    ui->GID_list->addItems(geo_list);
}
