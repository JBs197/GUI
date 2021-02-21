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
        QMutexLocker lock(&m_bar);
        jobs_done++;
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
    QString stmt = "CREATE TABLE IF NOT EXISTS TCatalogueIndex ( \"Year\" TEXT, ";
    stmt += "\"Name\" TEXT, \"Description\" TEXT );";
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
    QString cata_tname = "T" + cata.get_qname();
    QString cata_year = cata.get_year();
    QString cata_desc = cata.get_description();
    QString stmt = "INSERT INTO TCatalogueIndex ( \"Year\", \"Name\", \"Description\" ) VALUES ( ";
    stmt += "'" + cata_year + "', '" + cata_tname + "', '" + cata_desc + "' );";
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
    QSqlError qerror1 = query.lastError();
    if (qerror1.isValid())
    {
        return qerror1;
    }
    query.exec();
    QSqlError qerror2 = query.lastError();
    return qerror2;
}

// Update the catalogue tree and the tree's maps by reading from the database.
void MainWindow::update_cata_tree()
{
    cata_tree.clear();
    QSqlQuery query("SELECT DISTINCT Year FROM TCatalogueIndex");
    QSqlError qerr = query.lastError();
    if (qerr.isValid())
    {
        sqlerr("query distinct year-MainWindow.update_cata_tree", qerr);
    }
    QSqlRecord rec = query.record();
    int tree_index, name_index, year_index, desc_index;
    QVector<QString> temp;
    QString qname, qyear, qdesc;
    if (!rec.isEmpty())
    {
        // Create the map connecting qyear to tree's first index.
        cata_tree.resize(rec.count(), QVector<QVector<QString>>());
        year_index = 0;
        while (query.next())
        {
            qyear = query.value(0).toString();
            map_tree_year.insert(qyear, year_index);
            year_index++;
        }

        // Populate the tree, mapping catalogue names as we go.
        query.clear();
        if (!query.prepare("SELECT * FROM TCatalogueIndex"))
        {
            sqlerr("query entire metatable-MainWindow.update_cata_tree", query.lastError());
        }
        if (!query.exec())
        {
            sqlerr("query entire metatable-MainWindow.update_cata_tree", query.lastError());
        }
        rec = query.record();
        year_index = rec.indexOf("Year");
        name_index = rec.indexOf("Name");
        desc_index = rec.indexOf("Description");
        while (query.next()) // For each catalogue in the database...
        {
            qname = query.value(name_index).toString();
            qyear = query.value(year_index).toString();
            qdesc = query.value(desc_index).toString();
            temp = { qyear, qname, qdesc };
            tree_index = map_tree_year.value(qyear);
            map_tree_cata.insert(qname, cata_tree[tree_index].size());
            cata_tree[tree_index].append(temp);
        }
    }
}


// DEBUG FUNCTIONS:
void MainWindow::insert_csv_values(CATALOGUE& cat)
{

}


// TASK FUNCTIONS, USED BY ONE OR MORE GUI BUTTONS:

// For a given catalogue, populate its memory object with all values shared amongst GIDs.
// Then, create its primary table in the database.
void MainWindow::initialize_catalogue(CATALOGUE& cata, QString& qy, QString& qn)
{
    QString cata_path = QString::fromStdWString(root_directory);
    cata_path += "\\" + qy + "\\" + qn;
    cata.set_path(cata_path);
    cata.initialize_table();

    QVector<QString> primary_stmt = cata.create_primary_table();  // Form [statement, tname]
    QString tname;
    m_executor.lockForWrite();
    QSqlError qerror = executor(primary_stmt[0]);
    m_executor.unlock();
    if (qerror.isValid())
    {
        tname = primary_stmt[1];
        sqlerr("mainwindow.initialize_catalogue for " + tname, qerror);
    }



    /*
    nobles_st(cata);
    subtables_mt(cata);
    insert_cata_index(cata);
    int index = (int)binder.size();
    binder.push_back(cata);
    return index;
    */
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
    reset_bar(100, "Adding geographic regions to catalogue " + cata.get_qname());
    double gids_per_percent = (double)num_gid / 100.0;

    auto insert_csvs = [=] (CATALOGUE& cata, int bot, int top, int& report)
    {
        double gids_done = 0.0;
        QString gid;
        for (int ii = bot; ii <= top; ii++)
        {
            gid = cata.get_gid(ii);

            // Create the CSV table and all its subtables.

        }
    };
}

// Creates database tables for a catalogue's primary table and all full-CSV subtables.
void MainWindow::nobles_st(CATALOGUE& cat)
{
    QElapsedTimer timer;
    timer.start();
    QVector<QVector<QString>> noble_statements = cat.get_nobles();
    QString qname = cat.get_qname();
    int report = 0;
    double gids_per_percent = (double)noble_statements.size() / 100.0;
    reset_bar(100, "Creating CSV tables for " + qname + "...");

    auto create_nobles = [=] (int& report)
    {
        QString stmt, tname;
        double gids_done = 0.0;
        for (int ii = 0; ii < noble_statements.size(); ii++)
        {
            stmt = noble_statements[ii][0];
            m_executor.lockForWrite();
            QSqlError qerror = executor(stmt);
            m_executor.unlock();
            if (qerror.isValid())
            {
                tname = noble_statements[ii][1];
                sqlerr("mainwindow.nobles_mt for " + tname, qerror);
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
        m_bar.lock();
        report++;
        m_bar.unlock();
    };

    threads_working = 1;
    std::thread thr(create_nobles, ref(report));
    while (threads_working)
    {
        jobs_done = report;
        ui->progressBar->setValue(jobs_done);
        QCoreApplication::processEvents();
        if (report >= 100)
        {
            if (jobs_done < 100)
            {
                jobs_done = 100;
                ui->progressBar->setValue(jobs_done);
            }
            threads_working = 0;
        }
    }
    if (thr.joinable())
    {
        thr.join();
    }
    qDebug() << "nobles_mt " << timer.restart();
    logger(qname + " and CSV nobles were inserted into the database.");
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

// For a given catalogue, for all CSVs, for all tables/subtables,
// insert all row values into the database.
void MainWindow::populate_cata_mt(int cata_index)
{
    QVector<QString> gid_list = binder[cata_index].get_gid_list();
    wstring csv_trunk = binder[cata_index].get_csv_trunk();
    vector<wstring> csv_branches = binder[cata_index].get_csv_branches();
    QVector<QString> column_titles = binder[cata_index].get_column_titles();
    QVector<QString> row_titles = binder[cata_index].get_row_titles();
    QVector<QVector<QVector<int>>> tree = binder[cata_index].get_tree();
    QString primary_template = binder[cata_index].get_primary_columns_template();
    QString qname = binder[cata_index].get_qname();
    QVector<QVector<QString>> text_variables = binder[cata_index].get_model_text_variables();
    bool multi_column;
    if (column_titles.size() > 2)
    {
        multi_column = 1;
    }
    else
    {
        multi_column = 0;
    }

    int glist = binder[cata_index].get_gid_size();
    int workload = glist / cores;
    int bot = 0;
    int top = workload - 1;
    double gids_per_percent = (double)glist / 100.0;
    reset_bar(100, "Inserting row values for catalogue " + qname + "...");
    int report = 0;

    auto populate_cata_piece = [=] (int bot, int top, int& progress)  // Interval is inclusive.
    {
        QVector<QVector<QString>> rows;
        QVector<QVector<QString>> my_text_vars = text_variables;
        QString qfile, stmt, debug;
        wstring csv_path;
        QString temp, temp2, temp3;
        double gids_done = 0.0;
        int my_progress = 0;
        DWORD my_thread_id = GetCurrentThreadId();
        int pos1, pos2;

        for (int ii = bot; ii <= top; ii++)  // For every assigned CSV...
        {
            csv_path = csv_trunk + csv_branches[ii];
            qfile = q_memory(csv_path);
            rows = extract_csv_data_rows(qfile, row_titles, multi_column);
            stmt = primary_template;

            // Insert this CSV's values into the catalogue's primary table.
            pos1 = stmt.lastIndexOf("VALUES");
            pos2 = pos1;
            pos1 = stmt.indexOf('?', pos1);
            pos1 = insert_val(stmt, pos1, gid_list[ii]);  // GID is always first.

            update_text_vars(my_text_vars, qfile);
            for (int jj = 0; jj < my_text_vars.size(); jj++)
            {
                pos1 = insert_val(stmt, pos1, my_text_vars[jj][1]);
            }

            for (int jj = 0; jj < rows.size(); jj++)
            {
                for (int kk = 1; kk < rows[jj].size(); kk++)
                {
                    pos1 = insert_val(stmt, pos1, rows[jj][kk]);
                }
            }

            m_executor.lockForWrite();
            QSqlError qerror = executor(stmt);
            m_executor.unlock();
            if (qerror.isValid())
            {
                temp = QString::fromStdWString(csv_branches[ii]);
                sqlerr("mainwindow.populate_cata_piece for " + temp, qerror);
            }

            gids_done += 1.0;
            if (gids_done >= gids_per_percent)
            {
                m_bar.lock();
                progress++;
                m_bar.unlock();
                my_progress++;
                qDebug() << "Updated my progress to " << my_progress << " from " << my_thread_id;
                gids_done -= gids_per_percent;
            }
        }
    };

    vector<std::thread> tapestry;
    threads_working = 1;
    for (int ii = 0; ii < cores; ii++)
    {
        if (ii < cores - 1)
        {
            std::thread thr(populate_cata_piece, bot, top, ref(report));
            tapestry.push_back(std::move(thr));
            bot += workload;
            top += workload;
        }
        else
        {
            std::thread thr(populate_cata_piece, bot, glist - 1, ref(report));
            tapestry.push_back(std::move(thr));
        }
    }
    while (threads_working)
    {
        jobs_done = report;
        ui->progressBar->setValue(jobs_done);
        QCoreApplication::processEvents();
        if (report >= 100)
        {
            if (jobs_done < 100)
            {
                jobs_done = 100;
                ui->progressBar->setValue(jobs_done);
            }
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
    logger("Inserted all data rows for catalogue " + qname);
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
    for (int ii = 0; ii < catas_to_do.size(); ii++)
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
        else
        {
            year_index = map_tree_year.value(qyear);
            qdesc = cata_tree[year_index][cata_index][2];
            if (qdesc == "Incomplete")  // Determine which CSVs are missing, then insert them.
            {

            }
        }
        cata_index = insert_cata_tables(qyear, qname);
        populate_cata_mt(cata_index);

    }
    update_cata_tree();
    build_ui_tree(cata_tree, 2);
}

// (Debug function) Display some information.
void MainWindow::on_pB_test_clicked()
{
    QList<QTreeWidgetItem *> catas_to_do = ui->TW_cataondrive->selectedItems();
    auto bbq = catas_to_do[0]->flags();
    qDebug() << "Flags before: " << bbq;
    bbq.setFlag(Qt::ItemIsSelectable, false);
    catas_to_do[0]->setFlags(bbq);
    qDebug() << "Flags after: " << bbq;
}

// (Debug function) Perform a series of actions to test new functions.
void MainWindow::on_pB_benchmark_clicked()
{
    wdrive = L"F:";
    qdrive = "F:";
    QString cata_path = "F:\\3067\\97-570-X1981005";
    reset_db(db_qpath);
    int cata_index = insert_cata_tables(cata_path);
}

// Display the 'tabbed data' for the selected catalogue.
void MainWindow::on_pB_viewdata_clicked()
{
    QList<QTreeWidgetItem *> cata_to_do = ui->TW_cataindb->selectedItems();
}
