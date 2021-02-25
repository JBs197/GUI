#include "mainwindow.h"
#include "ui_mainwindow.h"

using std::vector;

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
    db = QSqlDatabase::addDatabase("QSQLITE", name_gen());
    db.setDatabaseName(db_qpath);
    if (!db.open()) { sqlerr("db.open-initialize", db.lastError()); }
    QSqlQuery q(db);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));

    create_cata_index_table();  // Even if starting a new database, this meta-table will always exist.
    update_cata_tree();
    build_ui_tree(cata_tree, 2);

    clear_log();
    logger("MainWindow initialized.");
}

// Return a unique name for a new database connection.
QString MainWindow::name_gen()
{
    m_namegen.lock();
    QString num;
    QString name = "Connection" + num.number(connection_count);
    connection_count++;
    m_namegen.unlock();
    return name;
}

// Populate a 2D tree containing the names of all catalogues in the database. Form [year][year, catalogues...].
// Also, populate a map connecting qyear->year_index  and  qname->cata_index.
void MainWindow::all_cata_db(QVector<QVector<QString>>& ntree, QMap<QString, int>& map_tree)
{
    QString qyear, qname, temp;
    int year_index, cata_index;
    QString stmt = "SELECT Year, Name FROM TCatalogueIndex";
    QSqlQuery q(db);
    bool fine = q.prepare(stmt);
    if (!fine) { sqlerr("prepare-all_cata_db", q.lastError()); }
    executor(q);
    //if (!fine) { sqlerr("exec-all_cata_db", q.lastError()); }
    while (q.next())
    {
        qyear = q.value(0).toString();
        qname = q.value(1).toString();
        qname.chop(1);
        qname.remove(0, 1);
        year_index = map_tree.value(qyear, -1);
        if (year_index < 0)
        {
            year_index = ntree.size();
            map_tree.insert(qyear, year_index);
            ntree.append(QVector<QString>());
            ntree[year_index].append(qyear);
        }
        cata_index = ntree[year_index].size();
        map_tree.insert(qname, cata_index);
        ntree[year_index].append(qname);
    }
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
    QString code = qerror.nativeErrorCode();  // RESUME HERE. Database is locking, even with WAL enabled.
    qDebug() << "Native error code: " << code;
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
    QString stmt = "CREATE TABLE IF NOT EXISTS TCatalogueIndex (Year TEXT, ";
    stmt += "Name TEXT, Description TEXT)";
    QSqlQuery q(db);
    bool fine = q.prepare(stmt);
    if (!fine) { sqlerr("prepare-create_cata_index_table", q.lastError()); }
    executor(q);
    //if (!fine) { sqlerr("exec-create_cata_index_table", q.lastError()); }
}
void MainWindow::insert_cata_index(CATALOGUE& cata)
{
    QString cata_tname = "[" + cata.get_qname() + "]";
    QString cata_year = cata.get_year();
    QString cata_desc = "Incomplete";  // All catalogues being inserted begin with this.

    bool fine;
    QSqlQuery q(db);
    fine = q.prepare("INSERT INTO TCatalogueIndex ( Year, Name, Description ) VALUES (?, ?, ?)");
    if (!fine) { sqlerr("prepare-insert_cata_index", q.lastError()); }
    q.addBindValue(cata_year);
    q.addBindValue(cata_tname);
    q.addBindValue(cata_desc);
    executor(q);
    //if (!fine) { sqlerr("exec-insert_cata_index", q.lastError()); }

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
    QString cata_tname = "[" + cata.get_qname() + "]";
    //QString cata_year = cata.get_year();
    QString cata_desc = cata.get_description();
    QString stmt = "UPDATE TCatalogueIndex SET Description = ? WHERE Name = ?";
    QSqlQuery q(db);
    bool fine = q.prepare(stmt);
    if (!fine) { sqlerr("prepare-finish_cata_index", q.lastError()); }
    q.addBindValue(cata_desc);
    q.addBindValue(cata_tname);
    executor(q);
    //if (!fine) { sqlerr("exec-finish_cata_index", q.lastError()); }
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

// For a prepared matrix of QString data, display it on the GUI as a 2D tree widget (year -> cata entry).
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

void MainWindow::executor(QSqlQuery& q)
{
    int failcount = 0;
    bool fine = q.exec();
    if (!fine)
    {
        QSqlError qerror = q.lastError();
        QString qcode = qerror.nativeErrorCode();
        int icode = qcode.toInt();
        while (icode == 5 && failcount < 1000)  // Database is busy. Try again.
        {
            QThread::msleep(10);
            fine = q.exec();
            if (fine) { break; }
            else
            {
                failcount++;
                qerror = q.lastError();
                qcode = qerror.nativeErrorCode();
                icode = qcode.toInt();
            }
        }
        if (icode != 5 && !fine)  // If the error isn't a busy database, report it.
        {
            sqlerr("exec-executor", qerror);
        }
    }
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
    map_tree_year.clear();
    map_tree_cata.clear();
    QString stmt = "SELECT DISTINCT Year FROM TCatalogueIndex";
    QSqlQuery q(db);
    bool fine = q.prepare(stmt);
    if (!fine) { sqlerr("prepare-update_cata_tree", q.lastError()); }
    executor(q);
    //if (!fine) { sqlerr("exec-update_cata_tree", q.lastError()); }

    int tree_index, name_index, year_index, desc_index;
    QVector<QString> temp;
    QString qname, qyear, qdesc;

    // Create the map connecting qyear to tree's first index.
    year_index = 0;
    while (q.next())
    {
        qyear = q.value(0).toString();
        map_tree_year.insert(qyear, year_index);
        cata_tree.append(QVector<QVector<QString>>());
        year_index++;
    }

    // Populate the tree, mapping catalogue names as we go.
    stmt = "SELECT * FROM TCatalogueIndex";
    q.clear();
    fine = q.prepare(stmt);
    if (!fine) { sqlerr("prepare2-update_cata_tree", q.lastError()); }
    executor(q);
    //if (!fine) { sqlerr("exec2-update_cata_tree", q.lastError()); }
    year_index = q.record().indexOf("Year");
    name_index = q.record().indexOf("Name");
    desc_index = q.record().indexOf("Description");
    while (q.next())                                            // For each catalogue in the database...
    {
        qname = q.value(name_index).toString();
        qyear = q.value(year_index).toString();
        qdesc = q.value(desc_index).toString();
        temp = { qyear, qname, qdesc };
        tree_index = map_tree_year.value(qyear);
        map_tree_cata.insert(qname, cata_tree[tree_index].size());
        cata_tree[tree_index].append(temp);
    }
    logger("Updated the cata_tree");
}


// DEBUG FUNCTIONS:
void MainWindow::insert_catalogue_st(CATALOGUE& cata, int& report)
{
    QVector<QString> gid_list = cata.get_gid_list();
    int num_gid = gid_list.size();
    int workload = num_gid / cores;
    int bot = 0;
    int top = workload - 1;
    bool cancelled = 0;
    reset_bar(num_gid, "Adding spreadsheets to catalogue   " + cata.get_qname());

    m_db.lock();
    db = QSqlDatabase::addDatabase("QSQLITE", "judi");
    db.setDatabaseName(db_qpath);
    if (!db.open()) { sqlerr("db.open-insert_catalogue_st", db.lastError()); }
    m_db.unlock();

    vector<std::thread> peons;
    cata.initialize_threading(cores);
    for (int ii = 0; ii < cores; ii++)
    {
        cata.jobs[ii] = -1 * (ii + 1);  // Naming threads. Should be immediately resolved by the new thread.
        cata.bot_top[ii][0] = bot;
        if (ii < cores - 1)
        {
            cata.bot_top[ii][1] = top;
        }
        else
        {
            cata.bot_top[ii][1] = num_gid - 1;
        }
        std::thread thr(&MainWindow::insert_csvs, std::ref(cata));
        peons.push_back(std::move(thr));
        bot += workload;
        top += workload;
    }
    judicator(cata, report);  // Processing of worker output.
    for (auto& th : peons)
    {
        if (th.joinable())
        {
            th.join();
        }
    }
    /*
    if (!cancelled)
    {
        finish_cata_index(cata);  // Mark this entry as complete in the meta-table, if it was finished.
        logger("Inserted all data for catalogue " + cata.get_qname());
    }
    else
    {
        logger("Insertion of catalogue " + cata.get_qname() + " was cancelled before it could finish.");
    }
    */
    int bbq = 1;
}
void MainWindow::insert_csvs(CATALOGUE& cata)
{
    int my_id, bot, top;
    int my_status = 0;
    QVector<QVector<QString>> text_vars, data_rows;
    QString gid, qfile, stmt;
    wstring csv_path;
    m_id.lock();
    for (int ii = 0; ii < (int)cata.jobs.size(); ii++)
    {
        if (cata.jobs[ii] < 0)
        {
            my_id = -1 * cata.jobs[ii];
            my_id--;
            cata.jobs[ii] = 0;
        }
    }
    m_id.unlock();
    bot = cata.bot_top[my_id][0];
    top = cata.bot_top[my_id][1];



    for (int ii = bot; ii <= top; ii++)  // Iterating by CSV...
    {
        my_status = cata.get_status();
        if (!my_status)  // Under normal circumstances...
        {
            gid = cata.get_gid(ii);
            csv_path = cata.get_csv_path(ii);
            qfile = q_memory(csv_path);
            text_vars = cata.extract_text_vars(qfile);
            data_rows = cata.extract_data_rows(qfile);

            // RESUME HERE. Have the workers send their statements to judicator.

            insert_primary_row(q, cata, gid, text_vars, data_rows);
            create_insert_csv_table(q, cata, gid, data_rows);
            create_insert_csv_subtables(q, cata, gid, data_rows);

            m_bar.lock();
            report++;
            m_bar.unlock();
            my_report++;
        }
        else  // Message from the controller thread...
        {
            switch (my_status)
            {
            case 1:
                m_bar.lock();
                report += top - bot + 1 - my_report;
                m_bar.unlock();
                my_status = -1;
                qDebug() << "my_status is " << my_status;

            case -1:
                qDebug() << "Aborting threaded work...";
                return;
            }
        }
    }
}
void MainWindow::judicator(CATALOGUE& cata, int& report)
{
    int num1 = cata.jobs.size();
    vector<int> peons_work(num1, 0);
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

    bool fine;
    QSqlQuery q(db);
    fine = q.prepare(primary_stmt);
    if (!fine) { sqlerr("prepare-initialize_catalogue for " + qn, q.lastError()); }
    executor(q);
    if (!fine)
    {
        //sqlerr("create primary table-mainwindow.initialize_catalogue for " + qn, q.lastError());
    }
}

// For a given incomplete catalogue, determine its existing table entries and return which tables are missing.
void MainWindow::scan_incomplete_cata(CATALOGUE& cata)
{
    // Determine which CSVs are missing from the database.
    QString stmt = "SELECT GID FROM [" + cata.get_qname() + "]";
    QVector<QString> gid_want_list = cata.get_gid_list();
    QVector<QString> gid_have_list;
    QString gid;
    QSqlQuery q(db);
    bool fine = q.prepare(stmt);
    if (!fine) { sqlerr("prepare-resume_incomplete_cata", q.lastError()); }
    executor(q);
    //if (!fine) { sqlerr("exec-resume_incomplete_cata", q.lastError()); }
    while (q.next())
    {
        gid = q.value(0).toString();
        gid_have_list.append(gid);
    }
    for (int ii = 0; ii < gid_have_list.size(); ii++)
    {
        gid = gid_have_list[ii];
        for (int jj = 0; jj < gid_want_list.size(); jj++)
        {
            if (gid_want_list[jj] == gid)
            {
                gid_want_list.remove(jj, 1);
                break;
            }
        }
    }
    cata.set_gid_want_list(gid_want_list);
    logger("Catalogue " + cata.get_qname() + " was scanned for missing CSVs.");
}

// For a given catalogue, insert all of its CSVs into the database, iterating by GID.
void MainWindow::insert_cata_csvs_mt(CATALOGUE& cata)
{
    QVector<QString> gid_list, gid_want_list;
    gid_want_list = cata.get_gid_want_list();
    if (gid_want_list.size() > 0)
    {
        gid_list = gid_want_list;
    }
    else
    {
        gid_list = cata.get_gid_list();
    }
    int num_gid = gid_list.size();
    qDebug() << "Catalogue " << cata.get_qname() << " needs " << num_gid << " CSVs.";
    int workload = num_gid / cores;
    int bot = 0;
    int top = workload - 1;
    int report = 0;
    bool cancelled = 0;
    reset_bar(num_gid, "Adding spreadsheets to catalogue   " + cata.get_qname());

    auto insert_csvs = [=] (CATALOGUE& cata, int bot, int top, int& report)
    {
        QString gid, qfile, connection_name;
        wstring csv_path;
        QVector<QVector<QString>> text_vars, data_rows;
        int my_report = 0;
        int my_status = 0;
        m_db.lock();
        connection_name = name_gen();
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connection_name);  // Every thread gets one.
        db.setDatabaseName(db_qpath);
        if (!db.open()) { sqlerr("db.open-insert_csvs", db.lastError()); }
        m_db.unlock();
        QSqlQuery q(db);

        for (int ii = bot; ii <= top; ii++)  // Iterating by CSV...
        {
            my_status = cata.get_status();
            if (!my_status)  // Under normal circumstances...
            {
                gid = cata.get_gid(ii);
                csv_path = cata.get_csv_path(ii);
                qfile = q_memory(csv_path);
                text_vars = cata.extract_text_vars(qfile);
                data_rows = cata.extract_data_rows(qfile);

                insert_primary_row(q, cata, gid, text_vars, data_rows);
                create_insert_csv_table(q, cata, gid, data_rows);
                create_insert_csv_subtables(q, cata, gid, data_rows);

                m_bar.lock();
                report++;
                m_bar.unlock();
                my_report++;
            }
            else  // Message from the controller thread...
            {
                switch (my_status)
                {
                case 1:
                    m_bar.lock();
                    report += top - bot + 1 - my_report;
                    m_bar.unlock();
                    my_status = -1;
                    qDebug() << "my_status is " << my_status;

                case -1:
                    qDebug() << "Aborting threaded work...";
                    return;
                }
            }
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
        if (remote_controller > 0 && !cancelled)
        {
            switch (remote_controller)
            {
            case 1:
                cata.cancel_insertion();
                cancelled = 1;
                break;
            }
        }
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

    if (!cancelled)
    {
        finish_cata_index(cata);  // Mark this entry as complete in the meta-table, if it was finished.
        logger("Inserted all data for catalogue " + cata.get_qname());
    }
    else
    {
        logger("Insertion of catalogue " + cata.get_qname() + " was cancelled before it could finish.");
    }
}
void MainWindow::insert_primary_row(QSqlQuery& q, CATALOGUE& cata, QString& gid, QVector<QVector<QString>>& text_vars, QVector<QVector<QString>>& data_rows)
{
    QString stmt = cata.get_primary_template();
    QString temp;
    q.clear();
    bool fine = q.prepare(stmt);
    if (!fine)
    {
        sqlerr("prepare-insert_primary_row " + cata.get_qname() + ", GID " + gid, q.lastError());
    }
    q.addBindValue(gid);
    for (int ii = 0; ii < text_vars.size(); ii++)
    {
        temp = "[" + text_vars[ii][1] + "]";
        q.addBindValue(temp);
    }
    for (int ii = 0; ii < data_rows.size(); ii++)
    {
        for (int jj = 1; jj < data_rows[ii].size(); jj++)
        {
            q.addBindValue(data_rows[ii][jj]);
        }
    }

    executor(q);
    if (!fine)
    {
        //sqlerr("mainwindow.insert_primary_row for T" + cata.get_qname() + ", GID " + gid, q.lastError());
    }

}
void MainWindow::create_insert_csv_table(QSqlQuery& q, CATALOGUE& cata, QString& gid, QVector<QVector<QString>>& data_rows)
{
    // Create this CSV's full table, starting from the template.
    QString stmt = cata.get_csv_template();
    QString qname = cata.get_qname();
    int pos1 = stmt.indexOf("!!!");
    QString tname = "[" + qname + "$" + gid + "]";
    stmt.replace(pos1, 3, tname);
    q.clear();
    bool fine = q.prepare(stmt);
    if (!fine) { sqlerr("prepare-create_insert_csv_table for " + tname, q.lastError()); }
    executor(q);
    //if (!fine) { sqlerr("exec-create_insert_csv_table for " + tname, q.lastError()); }

    // Insert the full table's data rows, one by one, starting from the template each time.
    stmt = cata.get_insert_csv_row_template();
    pos1 = stmt.indexOf("!!!");
    stmt.replace(pos1, 3, tname);
    for (int ii = 0; ii < data_rows.size(); ii++)
    {
        q.clear();
        fine = q.prepare(stmt);
        if (!fine) { sqlerr("prepare2-create_insert_csv_table for " + tname, q.lastError()); }
        for (int jj = 0; jj < data_rows[ii].size(); jj++)
        {
            q.addBindValue(data_rows[ii][jj]);
        }
        executor(q);
        //if (!fine) { sqlerr("exec2-create_insert_csv_table for " + tname, q.lastError()); }
    }
}
void MainWindow::create_insert_csv_subtables(QSqlQuery& q, CATALOGUE& cata, QString& gid, QVector<QVector<QString>>& data_rows)
{
    QString qname = cata.get_qname();
    QVector<QVector<QVector<int>>> tree = cata.get_tree();
    QString create_subtable_template = cata.get_create_sub_template();
    QString ins_csv_row_template = cata.get_insert_csv_row_template();
    QString stmt, stmt0, tname;
    int pos1, num_rows, row_index;
    bool fine;

    // Iterate by subtable.
    for (int ii = 0; ii < tree.size(); ii++)
    {
        tname = cata.sublabelmaker(gid, tree[ii]);
        num_rows = tree[ii][1].size() + 1;  // Top row is the parent, subsequent rows are direct children.

        // Create the subtable.
        stmt = create_subtable_template;
        pos1 = stmt.indexOf("!!!");
        stmt.replace(pos1, 3, tname);
        q.clear();
        fine = q.prepare(stmt);
        if (!fine)
        {
            sqlerr("prepare-create_insert_csv_subtables for " + tname, q.lastError());
        }
        executor(q);
        //if (!fine) { sqlerr("exec-create_insert_csv_subtables for " + tname, q.lastError()); }

        // Insert the subtable's rows.
        stmt = ins_csv_row_template;
        pos1 = stmt.indexOf("!!!");
        stmt.replace(pos1, 3, tname);
        for (int jj = 0; jj < num_rows; jj++)
        {
            q.clear();
            fine = q.prepare(stmt);
            if (!fine) { sqlerr("prepare2-create_insert_csv_subtables for " + tname, q.lastError()); }
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
                q.addBindValue(data_rows[row_index][kk]);
            }
            executor(q);
            if (!fine)
            {
                //sqlerr("exec2-create_insert_csv_subtables for " + tname, q.lastError());
            }
        }
    }
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
    QVector<QVector<QString>> catas_in_db;  // Form [year][year, catalogues...]
    QMap<QString, int> map_cata;
    QString qyear, qname, qdesc, stmt;
    int year_index, cata_index;
    QSqlQuery q(db);
    bool fine;

    ui->pB_cancel->setEnabled(1);
    all_cata_db(catas_in_db, map_cata);  // Populate the tree and map.
    for (int ii = 0; ii < catas_to_do.size(); ii++)  // For each catalogue selected...
    {
        CATALOGUE cata;
        qyear = catas_to_do[ii]->text(0);
        qname = catas_to_do[ii]->text(1);

        year_index = map_cata.value(qyear, -1);
        cata_index = map_cata.value(qname, -1);
        if (year_index < 0 || cata_index < 0)  // Catalogue is not present. Insert it completely.
        {
            initialize_catalogue(cata, qyear, qname);
            insert_cata_csvs_mt(cata);
        }
        else
        {
            stmt = "SELECT Description FROM TCatalogueIndex WHERE Name = [" + qname + "]";
            q.clear();
            fine = q.prepare(stmt);
            if (!fine) { sqlerr("prepare-on_pB_insert", q.lastError()); }
            executor(q);
            //if (!fine) { sqlerr("exec-on_pB_insert", q.lastError()); }
            q.next();
            qdesc = q.value(0).toString();
            if (qdesc == "Incomplete")    // Catalogue is partially present. Insert only the missing files.
            {
                initialize_catalogue(cata, qyear, qname);
                scan_incomplete_cata(cata);  // Inform the catalogue object which CSVs are missing.
                insert_cata_csvs_mt(cata);
            }
            else                          // Catalogue is fully present. Nothing to do.
            {
                qDebug() << "Catalogue " << qname << " is already included in the database.";
            }
        }
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
    int report = 0;
    initialize_catalogue(cata, qyear, qname);
    judicator_working = 1;
    std::thread judi(&MainWindow::insert_catalogue_st, std::ref(cata), std::ref(report));
    while (judicator_working)
    {
        QCoreApplication::processEvents();
        jobs_done = report;
        update_bar();
        if (remote_controller)
        {
            cata.cancel_insertion();
        }
        std::this_thread::sleep_for (std::chrono::milliseconds(50));
    }
    judi.join();

    /*
    insert_cata_csvs_mt(cata);
    insert_cata_index(cata);
    update_cata_tree();
    build_ui_tree(cata_tree, 2);
    */
    int bbq = 1;
}

// Display the 'tabbed data' for the selected catalogue.
void MainWindow::on_pB_viewdata_clicked()
{
    QSqlQuery q(db);
    QStringList geo_list, row_list;
    QString temp;
    QList<QTreeWidgetItem *> cata_to_do = ui->TW_cataindb->selectedItems();  // Only 1 catalogue can be selected.
    QString qyear = cata_to_do[0]->text(0);
    QString tname = cata_to_do[0]->text(1);

    // Populate the 'Geographic Region' tab.
    QString stmt = "SELECT Geography FROM " + tname;
    bool fine = q.prepare(stmt);
    if (!fine) { sqlerr("prepare-on_pB_viewdata", q.lastError()); }
    executor(q);
    //if (!fine) { sqlerr("exec-on_pB_viewdata", q.lastError()); }
    while (q.next())
    {
        temp = q.value(0).toString();
        temp.chop(1);
        temp.remove(0, 1);
        geo_list.append(temp);
    }

    // Populate the 'Row Data' tab.
    q.clear();
    stmt = "SELECT * FROM " + tname;
    fine = q.prepare(stmt);
    if (!fine) { sqlerr("prepare2-on_pB_viewdata", q.lastError()); }
    executor(q);
    //if (!fine) { sqlerr("exec2-on_pB_viewdata", q.lastError()); }
    QSqlRecord rec = q.record();
    int columns = rec.count();
    for (int ii = 0; ii < columns; ii++)
    {
        temp = rec.fieldName(ii);
        row_list.append(temp);
    }

    ui->GID_list->addItems(geo_list);
    ui->Rows_list->addItems(row_list);
}

// All threads inserting catalogues are told to stop after finishing their current CSV.
void MainWindow::on_pB_cancel_clicked()
{
    remote_controller = 1;
}
