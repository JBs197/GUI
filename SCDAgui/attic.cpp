// This is where unwanted functions are laid to rest...

// Generate the next unique name extension for a subtable.
QString CSV::subqname_gen()
{
    subqname++;
    QString name = QString::number(subqname);
    int size = name.size();
    for (int ii = 0; ii < (6 - size); ii++)
    {
        name.push_front('0');
    }
    return name;
}

void CSV::create_table_cata_old(QVector<QString>& work)
{
    int row;
    QString sql = "CREATE TABLE IF NOT EXISTS \"T" + qname + "\" ( GID INTEGER PRIMARY KEY, ";
    for (int ii = 0; ii < text_variables.size(); ii++)
    {
        sql += "\"";
        sql += text_variables[ii][0];
        sql += "\" TEXT, ";
    }
    for (int ii = 0; ii < is_int.size(); ii++)
    {
        row = is_int[ii][0];
        sql += "\"";
        sql += unique_row_title(row);
        if (is_int[ii][1] == 1) { sql += "\" INTEGER, "; }
        else if (is_int[ii][1] == 0) { sql += "\" REAL, "; }
        else { err8("Missing is_int values-csv.create_table_cata"); }
    }
    sql.remove(sql.size() - 2, 2);
    sql.append(" );");
    work.append(sql);
}

void CSV::create_table_csvs_old(QVector<QString>& work, QVector<QString>& gid_list)
{
    QString sql0 = "CREATE TABLE IF NOT EXISTS \"T" + qname;
    QString sql2 = "\" ( ";
    for (int ii = 0; ii < text_variables.size(); ii++)
    {
        sql2 += "\"";
        sql2 += text_variables[ii][0];
        sql2 += "\" TEXT, ";
    }
    if (multi_column)
    {
        sql2 += "\"";
        sql2 += column_titles[0];
        sql2 += "\" TEXT, ";
        for (int ii = 1; ii < column_titles.size(); ii++)
        {
            sql2 += "\"";
            sql2 += column_titles[ii];
            sql2 += "\" NUMERIC, ";
        }
    }
    else
    {
        sql2 += "\"Description\" TEXT, ";
        sql2 += "\"Value\" NUMERIC, ";
    }
    sql2.remove(sql2.size() - 2, 2);
    sql2.append(" );");

    QString sql1;
    for (int ii = 0; ii < gid_list.size(); ii++)
    {
        sql1 = "$";
        sql1.append(gid_list[ii]);
        work.append(sql0 + sql1 + sql2);
    }
}

// Build a list of SQL statements to create all the subtables for this CSV/catalogue.
void CSV::create_table_subs(QVector<QString>& work, QVector<QString>& gid_list, QVector<QVector<QVector<int>>>& model_tree)
{
    QString sql0 = "CREATE TABLE IF NOT EXISTS \"";
    QString sql2 = "\" ( ";
    for (int ii = 0; ii < text_variables.size(); ii++)
    {
        sql2 += "\"";
        sql2 += text_variables[ii][0];
        sql2 += "\" TEXT, ";
    }
    if (multi_column)
    {
        sql2 += "\"";
        sql2 += column_titles[0];
        sql2 += "\" TEXT, ";
        for (int ii = 1; ii < column_titles.size(); ii++)
        {
            sql2 += "\"";
            sql2 += column_titles[ii];
            sql2 += "\" NUMERIC, ";
        }
    }
    else
    {
        sql2 += "\"Description\" TEXT, ";
        sql2 += "\"Value\" NUMERIC, ";
    }
    sql2.remove(sql2.size() - 2, 2);
    sql2.append(" );");

    QString sql1;
    for (int ii = 0; ii < gid_list.size(); ii++)
    {
        for (int jj = 0; jj < model_tree.size(); jj++)
        {
            sql1 = sublabelmaker(gid_list[ii], model_tree[jj]);
            work.append(sql0 + sql1 + sql2);
        }
    }
}

// Build a SQL statement to create a subtable for this CSV. Append it to the list.
void CSV::create_subtable_statement(QVector<QString>& work, QVector<QVector<int>>& subtree)
{
    QString sub_tname = "T" + qname + "$" + subqname_gen();
    sub_tname_list.append(sub_tname);
    int new_subtable_vars = subtree[0].size();
    QString new_var;
    QString new_val;
    QVector<QVector<QString>> subtable_text_variables;
    for (int ii = 0; ii < new_subtable_vars; ii++)
    {
        new_var = "Subtable-" + QString::number(ii);
        new_val = row_titles[subtree[0][ii]];
        subtable_text_variables.append({ new_var, new_val });
    }

    int is_int_index;
    QVector<int> is_int_vals;
    int new_subtable_vals = subtree[1].size();
    for (int ii = 0; ii < new_subtable_vals; ii++)
    {
        is_int_index = map_isint.value(subtree[1][ii]);
        is_int_vals.append(is_int[is_int_index][1]);
    }

    QString sql = "CREATE TABLE IF NOT EXISTS \"" + sub_tname + "\" ( ";
    for (int ii = 0; ii < text_variables.size(); ii++)
    {
        sql += "\"";
        sql += text_variables[ii][0];
        sql += "\" TEXT, ";
    }
    for (int ii = 0; ii < subtable_text_variables.size(); ii++)
    {
        sql += "\"";
        sql += subtable_text_variables[ii][0];
        sql += "\" TEXT, ";
    }

    for (int ii = 0; ii < new_subtable_vals; ii++)
    {
        sql += "\"";
        sql += row_titles[subtree[1][ii]];
        if (is_int_vals[ii] == 1) { sql += "\" INTEGER, "; }
        else if (is_int_vals[ii] == 0) { sql += "\" REAL, "; }
        else { err8("Missing is_int values-csv.create_subtable_statement"); }
    }
    sql.remove(sql.size() - 2, 2);
    sql.append(" );");
    work.append(sql);
}

// Return a list of SQL statements to create the main table and all nested subtables for this catalogue.
QVector<QString> CATALOGUE::get_create_table_statements(int cores)
{
    QElapsedTimer timer;
    timer.start();
    QVector<QString> work;
    model.create_table_cata(work);
    model.create_table_csvs(work, gid_list);
    qDebug() << "Initialize tables: " << timer.restart();
    model.create_table_subs(work, gid_list, model_tree);
    qDebug() << "Create Table subs, ST: " << timer.restart();
    create_table_taskmaster(work, cores);
    qDebug() << "Create Table subs, MT: " << timer.restart();
    create_table_mapped(work);
    qDebug() << "Create Table subs, mapped: " << timer.restart();
    QString tc;
    qlog("Created a SQL statement list to insert " + tc.setNum(work.size()) + " tables from catalogue " + qname + ".");
    return work;
}

void CATALOGUE::create_table_taskmaster(QVector<QString>& work, int cores)
{
    QElapsedTimer timer;
    timer.start();
    QVector<QVector<QVector<int>>> local_tree = model_tree;

    // Split 'gid_list' into 'cores' number of 2D vectors.
    int workload = gid_list.size() / cores;
    QVector<QVector<QString>> gid_pie(cores, QVector<QString>());
    int bot = 0;
    int top = workload - 1;
    for (int ii = 0; ii < cores; ii++)
    {
        if (ii < cores - 1)
        {
            gid_pie[ii] = string_vector_slicer(gid_list, bot, top);
            bot += workload;
            top += workload;
        }
        else
        {
            gid_pie[ii] = string_vector_slicer(gid_list, bot, gid_list.size() - 1);
        }
    }

    // Create the portion of the final SQL statement which is common to all GIDs and subtables.
    QVector<QVector<QString>> csv_text_variables = model.get_text_variables();
    QVector<QString> col_titles;
    QString sql0 = "CREATE TABLE IF NOT EXISTS \"";
    QString sql2 = "\" ( ";
    for (int ii = 0; ii < csv_text_variables.size(); ii++)
    {
        sql2 += "\"";
        sql2 += csv_text_variables[ii][0];
        sql2 += "\" TEXT, ";
    }
    if (model.get_multi_column())
    {
        col_titles = model.get_column_titles();
        sql2 += "\"";
        sql2 += col_titles[0];
        sql2 += "\" TEXT, ";
        for (int ii = 1; ii < col_titles.size(); ii++)
        {
            sql2 += "\"";
            sql2 += col_titles[ii];
            sql2 += "\" NUMERIC, ";
        }
    }
    else
    {
        sql2 += "\"Description\" TEXT, ";
        sql2 += "\"Value\" NUMERIC, ";
    }
    sql2.remove(sql2.size() - 2, 2);
    sql2.append(" );");

    auto create_table_peon = [] (QVector<QString> gid_list, int num, QVector<QVector<QVector<int>>> my_tree, QString qnam)
    {
        QElapsedTimer timer;
        timer.start();
        QString sql1;
        QVector<QString> output;
        QString stname, temp;
        int ancestry;
        int cheddar;
        for (int ii = 0; ii < gid_list.size(); ii++)
        {
            for (int jj = 0; jj < my_tree.size(); jj++)
            {
                ancestry = my_tree[jj][0].size();
                cheddar = 2;
                sql1 = "T" + qnam + "$" + gid_list[ii];
                for (int kk = 0; kk < ancestry; kk++)
                {
                    for (int ll = 0; ll < cheddar; ll++)
                    {
                        sql1 += "$";
                    }
                    cheddar++;
                    temp = QString::number(my_tree[jj][0][kk]);
                    sql1 += temp;
                }
                output.append(sql1);
            }
        }
        qDebug() << " peon " << num << " finished in " << timer.restart();
        return output;
    };

    QList<QFuture<QList<QString>>> work_order;
    qDebug() << "Time to setup the threaded section: " << timer.restart();
    for (int ii = 0; ii < cores; ii++)
    {
        work_order.append(QtConcurrent::task(std::move(create_table_peon))
                .withArguments(gid_pie[ii], ii, local_tree, qname)
                .spawn()
                );
    }
    QThread::msleep(5);
    bool jobsdone = 0;
    int tally;
    while (!jobsdone)
    {
        QThread::msleep(5);
        tally = 0;
        for (int ii = 0; ii < work_order.size(); ii++)
        {
            tally += work_order[ii].isFinished();
        }
        if (tally >= work_order.size())
        {
            jobsdone = 1;
        }
    }
    qDebug() << "Time to finish all worker threads: " << timer.restart();

    QString create_table_sub;
    for (int ii = 0; ii < work_order.size(); ii++)
    {
        auto result = work_order[ii].result();
        for (int jj = 0; jj < result.size(); jj++)
        {
            create_table_sub = sql0 + result[jj] + sql2;
            work.append(create_table_sub);
        }
    }
    qDebug() << "Time to append results to the statement list: " << timer.restart();
}

void CATALOGUE::create_table_mapped(QVector<QString>& work)
{
    QElapsedTimer timer1;
    timer1.start();
    // Create the portion of the final SQL statement which is common to all GIDs and subtables.
    QVector<QVector<QString>> csv_text_variables = model.get_text_variables();
    QVector<QVector<QVector<int>>> local_tree = model_tree;
    QVector<QString> local_gid_list = gid_list;
    QVector<QString> col_titles;
    QString local_qname = qname;
    QString sql0 = "CREATE TABLE IF NOT EXISTS \"";
    QString sql2 = "\" ( ";
    for (int ii = 0; ii < csv_text_variables.size(); ii++)
    {
        sql2 += "\"";
        sql2 += csv_text_variables[ii][0];
        sql2 += "\" TEXT, ";
    }
    if (model.get_multi_column())
    {
        col_titles = model.get_column_titles();
        sql2 += "\"";
        sql2 += col_titles[0];
        sql2 += "\" TEXT, ";
        for (int ii = 1; ii < col_titles.size(); ii++)
        {
            sql2 += "\"";
            sql2 += col_titles[ii];
            sql2 += "\" NUMERIC, ";
        }
    }
    else
    {
        sql2 += "\"Description\" TEXT, ";
        sql2 += "\"Value\" NUMERIC, ";
    }
    sql2.remove(sql2.size() - 2, 2);
    sql2.append(" );");

    auto create_table_mapeon = [local_tree, local_qname] (QString gid)
    {
        int num_subs = local_tree.size();
        QVector<QString> output(num_subs);
        QString sql1;
        QString temp;
        int ancestry;
        int cheddar;

        for (int jj = 0; jj < num_subs; jj++)
        {
            ancestry = local_tree[jj][0].size();
            cheddar = 2;
            sql1 = "T" + local_qname + "$" + gid;
            for (int kk = 0; kk < ancestry; kk++)
            {
                for (int ll = 0; ll < cheddar; ll++)
                {
                    sql1 += "$";
                }
                cheddar++;
                temp = QString::number(local_tree[jj][0][kk]);
                sql1 += temp;
            }
            output[jj] = sql1;
        }
        return output;
    };

    qDebug() << "Time to setup the threaded section: " << timer1.restart();

    QFuture<QVector<QString>> name_list = QtConcurrent::mapped(local_gid_list, create_table_mapeon);
    QThread::msleep(5);
    name_list.waitForFinished();

    qDebug() << "Time to finish all mapping: " << timer1.restart();

    QString create_table_sub;
    auto result = name_list.results();
    for (int ii = 0; ii < result.size(); ii++)  // GID index...?
    {
        for (int jj = 0; jj < result[ii].size(); jj++)  // tree index...?
        {
            create_table_sub = sql0 + result[ii][jj] + sql2;
            work.append(create_table_sub);
        }
    }
    qDebug() << "Time to append results to the statement list: " << timer1.restart();
}

// Return a list of SQL statements to insert the row values for each table/subtable in one given CSV.
QVector<QString> CATALOGUE::get_CSV_insert_value_statements(int csv_index)
{
    QVector<QString> work;
    make_name_tree();
    wstring csv_path = get_csv_path(csv_index);
    size_t wpos1 = csv_path.find(L'(');
    size_t wpos2 = csv_path.find(L')', wpos1);
    wstring wtemp = csv_path.substr(wpos1 + 1, wpos2 - wpos1 - 1);
    QString gid = QString::fromStdWString(wtemp);
    bool yesno;
    gid.toUInt(&yesno);
    if (!yesno) { err8("gid.toUInt-cata.get_CSV_insert_value_statements"); }

    HANDLE hfile = CreateFileW(csv_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-cata.get_CSV_insert_value_statements"); }
    QString qfile = q_memory(hfile);
    int pos1 = qfile.lastIndexOf("Catalogue Number ", qfile.size() - 4);
    pos1 += 17;
    int pos2 = qfile.indexOf('.', pos1);
    QString cata_name = qfile.mid(pos1, pos2 - pos1);

    CSV page;
    page.scan(qfile, cata_name);
    page.insert_value_all_statements(work, model_tree, model_subtable_names, model_subtable_text_variables);
    return work;
}

// Creates a catalogue object with all the column headers, then inserts
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

void MainWindow::subtables_mt(CATALOGUE& cat)
{
    QVector<QVector<QVector<int>>> tree = cat.get_tree();
    QVector<QString> gid_list = cat.get_gid_list();
    QString sub_template = cat.get_create_sub_template();
    QString qname = cat.get_qname();
    DWORD compass = GetCurrentThreadId();
    LPMSG mailbox = new MSG;
    int num_gid = gid_list.size();
    reset_bar(num_gid);

    auto insert_subtable_gid = [=] (int bot, int top)  // Interval is inclusive.
    {
        QString name, stmt, temp;
        int pos1, ancestry, cheddar;
        UINT message;
        QElapsedTimer timer;
        timer.start();
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
            message = WM_APP + ii;
            if (!PostThreadMessageW(compass, message, 0, 0))
            {
                winwarn(L"subtables_mt");
            }
        }
    };

    //QList<QFutureWatcher<void>> giles(cores, QFutureWatcher<void>());
    QList<QFuture<void>> bag_of_nothing;
    max_progress = gid_list.size();
    int workload = gid_list.size() / cores;
    int bot = 0;
    int top = workload - 1;
    for (int ii = 0; ii < cores; ii++)
    {
        if (ii < cores - 1)
        {
            bag_of_nothing.append(QtConcurrent::task(std::move(insert_subtable_gid))
                    .withArguments(bot, top)
                    .spawn() );
            bot += workload;
            top += workload;
        }
        else
        {
            bag_of_nothing.append(QtConcurrent::task(std::move(insert_subtable_gid))
                    .withArguments(bot, gid_list.size() - 1)
                    .spawn() );
        }
    }
    bool jobsdone = 0;
    int tally, enigma;
    bool aol = 0;
    while (!jobsdone)
    {
        QThread::msleep(100);
        tally = 0;
        aol = PeekMessageW(mailbox, NULL, WM_APP, WM_APP + 15000, PM_REMOVE);
        if (aol)
        {
            enigma = mailbox->message;
            enigma -= WM_APP;
            qDebug() << "Received message: " << enigma;
            update_bar();
        }
        for (int ii = 0; ii < cores; ii++)
        {
            tally += bag_of_nothing[ii].isFinished();
        }
        if (tally >= cores)
        {
            jobsdone = 1;
        }
    }
}

void CSV::extract_rows(int pos0)
{
    vector<int> space_index = { 0 };
    int indentation = 0;
    vector<int> family_line = { 0 };
    size_t pos1, pos2, nl1, nl2;
    int spaces;
    QString temp1, temp2, val;
    string exception;

    nl1 = pos0;
    nl2 = qfile.indexOf('\n', nl1 + 1);
    do
    {
        pos1 = qfile.indexOf('"', nl1);
        pos2 = qfile.indexOf('"', pos1 + 1);
        temp1 = qfile.mid(pos1 + 1, pos2 - pos1 - 1);
        spaces = qclean(temp1, 0);
        if (temp1 == "Note") { break; }  // Primary exit from the loop.
        indentation = index_card(space_index, spaces);
        if (indentation < 0) { err(L"index_card-csv.extract_rows"); }

        temp2.clear();
        for (int ii = 0; ii < indentation; ii++)
        {
            temp2.push_back('+');
        }
        row_titles.append(temp2 + temp1);

        nl1 = nl2;
        nl2 = qfile.indexOf('\n', nl1 + 1);
    } while (nl2 > 0);  // Secondary exit from the loop.
}

void CSV::extract_classic_rows(int pos0)
{
    vector<int> space_index = { 0 };
    int indentation = 0;
    vector<int> family_line = { 0 };
    size_t pos1, pos2, pos3, nl1, nl2;
    int spaces, row_index;
    QString temp1, temp2, val;
    string exception;

    nl1 = pos0;
    nl2 = qfile.indexOf('\n', nl1 + 1);
    do
    {
        classic_rows.append(QVector<QString>());
        row_index = classic_rows.size() - 1;
        pos1 = qfile.indexOf('"', nl1);
        pos2 = qfile.indexOf('"', pos1 + 1);
        temp1 = qfile.mid(pos1 + 1, pos2 - pos1 - 1);
        spaces = qclean(temp1, 0);
        if (temp1 == "Note") { break; }  // Primary exit from the loop.
        indentation = index_card(space_index, spaces);
        if (indentation < 0) { err(L"index_card-csv.extract_classic_rows"); }

        temp2.clear();
        for (int ii = 0; ii < indentation; ii++)
        {
            temp2.push_back('+');
        }
        temp2 += temp1;
        classic_rows[row_index].append(temp2);  // First string is the row title.

        pos2 = qfile.indexOf(',', pos2);
        do
        {
            pos1 = pos2;
            pos2 = qfile.indexOf(',', pos1 + 1);
            if (pos2 > nl2)
            {
                pos3 = qfile.indexOf(' ', pos1 + 1);
                if (pos3 > nl2)
                {
                    pos3 = qfile.indexOf('\r', pos1 + 1);
                    if (pos3 > nl2)
                    {
                        err8("pos error in extract_classic_rows");
                    }
                }
                temp1 = qfile.mid(pos1 + 1, pos3 - pos1 - 1);
                classic_rows[row_index].append(temp1);  // All strings after the first are values, in string form.
            }
            else
            {
                temp1 = qfile.mid(pos1 + 1, pos2 - pos1 - 1);
                classic_rows[row_index].append(temp1);  // All strings after the first are values, in string form.
            }
        } while (pos2 < nl2);

        classic_is_int.append(qnum_test(temp1));  // Index is identical to 'classic_rows'.

        nl1 = nl2;
        nl2 = qfile.indexOf('\n', nl1 + 1);
    } while (nl2 > 0);  // Secondary exit from the loop.
}

// Use the CSV object's stored data to build a tree structure of row indices. Also, populate values.
void CSV::organize_subtables(int pos0)
{
    QString row;
    int pos, indentation;
    QVector<QVector<int>> pidgeonhole;  // Form [indentation][row_titles index]
    if (multi_column)
    {
        indent.resize(row_titles.size());
        for (int ii = 0; ii < row_titles.size(); ii++)
        {
            row = row_titles[ii];
            pos = 0;
            indentation = 0;
            while (row[pos] == '+')                                   // The number of '+' signs for a given
            {                                                         // row title tells us that row's rank
                pos++;                                                // within the catalogue hierarchy.
                indentation++;
            }
            indent[ii] = indentation;
            while (indentation >= pidgeonhole.size())                 // We create a 2D vector which categorizes
            {                                                         // each table row by its rank (steps
                pidgeonhole.append(QVector<int>());                   // removed from the tree's root).
            }

            pidgeonhole[indentation].append(ii);
        }
    }
    else
    {
        for (int ii = 0; ii < row_titles.size(); ii++)
        {
            row = row_titles[ii];
            pos = 0;
            indentation = 0;
            while (row[pos] == '+')                                   // The number of '+' signs for a given
            {                                                         // row title tells us that row's rank
                pos++;                                                // within the catalogue hierarchy.
                indentation++;
            }

            while (indentation >= pidgeonhole.size())                 // We create a 2D vector which categorizes
            {                                                         // each table row by its rank (steps
                pidgeonhole.append(QVector<int>());                   // removed from the tree's root).
            }

            pidgeonhole[indentation].append(ii);
        }
    }


    int top, bot, mid;
    QVector<int> temp;
    subtables.resize(pidgeonhole.size() - 1);                     // Subtables is always one row smaller than
    for (int ii = 0; ii < subtables.size(); ii++)                 // pidgeonhole, because the group of rows
    {                                                             // which is furthest removed from the tree's
        for (int jj = 0; jj < pidgeonhole[ii].size(); jj++)       // root cannot possibly have their own
        {                                                         // subordinate rows.
            if (jj == pidgeonhole[ii].size() - 1)
            {
                top = row_titles.size() - 1;
                bot = pidgeonhole[ii][jj];
            }
            else
            {
                top = pidgeonhole[ii][jj + 1];
                bot = pidgeonhole[ii][jj];
            }
            temp.clear();
            temp.append(bot);                                        // NOTE: The first integer in every
            for (int kk = 0; kk < pidgeonhole[ii + 1].size(); kk++)  // "subtables" 1D vector is the immediate
            {                                                        // parent row index.
                mid = pidgeonhole[ii + 1][kk];
                if (mid > bot && mid <= top)
                {
                    temp.append(mid);
                }
                else if (mid > top) { break; }
            }
            if (temp.size() > 1)
            {
                subtables[ii].append(temp);
            }
        }
    }

    QString colrow, colrowbase;
    int orig_size = row_titles.size();
    int baserow;
    QVector<QString> lvalues(column_titles.size());
    QVector<int> lis_int(column_titles.size());
    QVector<int> ltemp(3);
    pos = pos0;
    if (multi_column)
    {
        subtables.append(QVector<QVector<int>>());  // The columns will add one hierarchy layer everywhere.
        for (int ii = 0; ii < orig_size; ii++)  // For every data row with values...
        {
            colrowbase.clear();
            for (int jj = 0; jj <= indent[ii]; jj++)
            {
                colrowbase.append('+');
            }

            extract_row_values(pos, lvalues, lis_int);  // ... read qfile to pull this row's data.

            temp.clear();
            temp.append(ii);  // Parent row index.
            baserow = row_titles.size();
            for (int jj = 0; jj < column_titles.size(); jj++)  // For every column in the 2D CSV...
            {
                colrow = colrowbase + column_titles[jj];  // ... make a new row title with indentation.
                row_titles.append(colrow);  // ... insert the new row title beneath the existing titles.
                row_values.append(lvalues[jj]);  // ... load that column's value into the object.
                map_values.insert(baserow + jj, row_values[row_values.size() - 1]);
                ltemp[0] = baserow + jj;  // New row's index inside row_titles.
                ltemp[1] = lis_int[jj];  // New row's "is int" status (1=int, 0=double, -1=missing).
                ltemp[2] = row_values.size() - 1;  // New row's index inside row_values.
                is_int.append(ltemp);
                map_isint.insert(baserow + jj, is_int[is_int.size() - 1][1]);
                temp.append(ltemp[0]);  // Child row index.
            }

            subtables[indent[ii]].append(temp);
        }
    }
    else
    {
        column_titles.resize(2);
        column_titles[0] = "Description";  // These are inserted to avoid SQL problems.
        column_titles[1] = "Values";

        for (int ii = 0; ii < orig_size; ii++)          // For every data row with values...
        {
            extract_row_values(pos, lvalues, lis_int);  // ... read qfile to pull this row's data.

            row_values.append(lvalues[0]);
            map_values.insert(ii, row_values[row_values.size() - 1]);
            ltemp[0] = ii;
            ltemp[1] = lis_int[0];
            ltemp[2] = row_values.size() - 1;
            is_int.append(ltemp);
            map_isint.insert(ii, is_int[is_int.size() - 1][1]);
        }
    }
}

QVector<int> CSV::insert_value_all_statements(QVector<QString>& work, QVector<QVector<QVector<int>>>& model_tree, QVector<QString>& subtable_names, QVector<QVector<QString>>& subtable_text_variables)
{
    QVector<QVector<int>> int_double_results;
    QVector<int> int_double_subtable_results;
    // Do the main table first...
    int row, sindex;
    int icount = 0;
    int dcount = 0;
    QString sql = "INSERT INTO \"T" + qname + "\" ( GID, ";
    for (int ii = 0; ii < text_variables.size(); ii++)
    {
        sql += "\"";
        sql += text_variables[ii][0];
        sql += "\", ";
    }
    for (int ii = 0; ii < is_int.size(); ii++)
    {
        row = is_int[ii][0];
        sql += "\"";
        sql += row_titles[row];
        sql += "\", ";
    }
    sql.remove(sql.size() - 2, 2);
    sql += " ) VALUES ( ";
    for (int ii = 0; ii < text_variables.size(); ii++)
    {
        sql += "\"";
        sql += text_variables[ii][1];
        sql += "\", ";
    }
    for (int ii = 0; ii < is_int.size(); ii++)
    {
        sindex = is_int[ii][2];
        sql += "\"";
        sql += row_values[sindex];
        sql += "\", ";

        if (is_int[ii][1] == 1) { icount++; }
        else if (is_int[ii][1] == 0) { dcount++; }
    }
    sql.remove(sql.size() - 2, 2);
    sql.append(" );");
    work.append(sql);
    int_double_results.append({ icount, dcount });

    // Add INSERT statements for all subtables in this CSV.
    int subtrees = model_tree.size();
    for (int ii = 0; ii < subtrees; ii++)
    {
        int_double_subtable_results = insert_subtable_statement(work, model_tree[ii], subtable_names, subtable_text_variables);
        int_double_results.append(int_double_subtable_results);
    }

    // Consistency check.
    icount = int_double_results[0][0];
    dcount = int_double_results[0][1];
    for (int ii = 1; ii < int_double_results.size(); ii++)
    {
        if (int_double_results[ii][0] != icount)
        {
            err8("Inconsistent integer value counts for cata " + qname.toStdString() + ", GID " + gid.toStdString());
        }
        else if (int_double_results[ii][1] != dcount)
        {
            err8("Inconsistent double value counts for cata " + qname.toStdString() + ", GID " + gid.toStdString());
        }
    }

    return int_double_results[0];
}

QVector<int> CSV::insert_subtable_statement(QVector<QString>& work, QVector<QVector<int>>& model_subtree, QVector<QString>& subtable_names, QVector<QVector<QString>>& subtable_text_variables)
{
    int tree_index = work.size() - 1;
    int isint_index;
    int subtable_size = subtable_text_variables.size();
    int icount = 0;
    int dcount = 0;

    QString sql = "INSERT INTO \"" + subtable_names[tree_index] + "\" ( ";
    for (int ii = 0; ii < text_variables.size(); ii++)
    {
        sql += "\"";
        sql += text_variables[ii][0];
        sql += "\", ";
    }
    for (int ii = 0; ii < subtable_size; ii++)
    {
        sql += "\"";
        sql += subtable_text_variables[ii][0];
        sql += "\", ";
    }
    int num_val_rows = model_subtree[1].size();
    for (int ii = 0; ii < num_val_rows; ii++)
    {
        sql += "\"";
        sql += row_titles[model_subtree[1][ii]];
        sql += "\", ";
    }
    sql.remove(sql.size() - 2, 2);
    sql.append(" ) VALUES ( ");
    for (int ii = 0; ii < text_variables.size(); ii++)
    {
        sql += "\"";
        sql += text_variables[ii][1];
        sql += "\", ";
    }
    for (int ii = 0; ii < subtable_size; ii++)
    {
        sql += "\"";
        sql += subtable_text_variables[ii][1];
        sql += "\", ";
    }
    for (int ii = 0; ii < num_val_rows; ii++)
    {
        sql += "\"";
        sql += map_values.value(model_subtree[1][ii]);
        sql += "\", ";
        isint_index = map_isint.value(model_subtree[1][ii]);
        if (is_int[isint_index][1] == 1)
        {
            icount++;
        }
        else if (is_int[isint_index][1] == 0)
        {
            dcount++;
        }
    }
    sql.remove(sql.size() - 2, 2);
    sql.append(" );");
    work.append(sql);
    QVector<int> int_double_result = { icount, dcount };
    return int_double_result;
}

// Convenience function to obtain the value data from a specific line in the CSV file.
void CSV::extract_row_values(int& pos0, QVector<QString>& values, QVector<int>& isint)
{
    QString val;
    bool success;
    int pos1, pos2, nl;
    pos2 = pos0;
    nl = qfile.indexOf('\n', pos0 + 1);
    pos1 = qfile.indexOf('"', pos2);
    pos2 = qfile.indexOf('"', pos1 + 1);
    for (int ii = 0; ii < values.size(); ii++)
    {
        pos1 = qfile.indexOf(',', pos2 + 1);
        pos2 = qfile.indexOf(',', pos1 + 1);
        if (pos2 > nl)
        {
            pos2 = qfile.indexOf(' ', pos1 + 1);
            if (pos2 > nl)
            {
                pos2 = qfile.indexOf('\r', pos1 + 1);
            }
        }
        if (pos1 > nl || pos2 > nl) { err8("find commas-csv.extract_row_values"); }
        val = qfile.mid(pos1 + 1, pos2 - pos1 - 1);
        qclean(val, 0);
        values[ii] = val;
        if (val == "..")  // Indicates "no data" from Stats Canada.
        {
            isint[ii] = -1;
            continue;
        }

        pos1 = val.indexOf('.');
        if (pos1 < 0)
        {
            val.toInt(&success);
            if (!success)
            {
                err8("stoi error in csv.extract_row_values");
            }
            isint[ii] = 1;
        }
        else
        {
            val.toDouble(&success);
            if (!success)
            {
                err8("stod error in csv.extract_row_values");
            }
            isint[ii] = 0;
        }
    }
    pos0 = nl;
}

void MainWindow::populate_cata_piece(CATALOGUE worker, int bot, int top)
{
    QVector<QVector<QString>> rows;
    QString qname = worker.get_qname();
    QString temp;
    bool fine;
    int inum;
    double dnum;
    QString primary_template = worker.get_primary_columns_template();
    for (int ii = bot; ii <= top; ii++)  // For every assigned CSV...
    {
        worker.set_qfile(ii);
        rows = worker.extract_data_rows();

        // Insert this CSV's values into the catalogue's primary table.
        QSqlQuery primary_row(db);
        fine = primary_row.prepare(primary_template);
        if (!fine)
        {
            sqlerr("primary_row.prepare-populate_cata_piece", primary_row.lastError());
        }
        for (int jj = 0; jj < rows.size(); jj++)  // For every data row...
        {
            for (int kk = 1; kk < rows[jj].size(); kk++)  // For every column with a numeric value...
            {
                inum = rows[jj][kk].toInt(&fine);
                if (fine)
                {
                    primary_row.addBindValue(inum);  // If the value is an integer, insert it as such.
                }
                else
                {
                    dnum = rows[jj][kk].toDouble(&fine);
                    if (fine)
                    {
                        primary_row.addBindValue(dnum);  // If the value is a double, insert it as such.
                    }
                    else
                    {
                        primary_row.addBindValue(-999);  // If the value is nonconformant, insert it as -999
                        temp.setNum(jj);                 // and make a note of the failure in the error log.
                        qwarn("Erroneous value in cata " + qname + " | GID " + worker.get_gid(ii) + " | row " + temp);
                    }
                }
            }
        }
        m_executor.lockForWrite();
        fine = primary_row.exec();
        if (!fine)
        {
            sqlerr("primary_row.exec-populate_cata_piece", primary_row.lastError());
        }
        m_executor.unlock();
    }
}

while (jobs_done < jobs_max)
{
	aol = PeekMessageW(&mailbox, NULL, WM_APP, WM_APP + 15000, PM_REMOVE);
	if (aol)
	{
		enigma = mailbox.message;
		enigma -= WM_APP;
		qDebug() << "Received message: " << enigma;
		update_bar();
	}
	QCoreApplication::processEvents();
}
	
void MainWindow::subtables_st(CATALOGUE& cat)
{
    QVector<QVector<QVector<int>>> tree = cat.get_tree();
    QVector<QString> gid_list = cat.get_gid_list();
    QString sub_template = cat.get_create_sub_template();
    QString stmt, name;
    int pos1;
    int num_gid = gid_list.size();
    reset_bar(num_gid);
    for (int ii = 0; ii < num_gid; ii++)
    {
        for (int jj = 0; jj < tree.size(); jj++)
        {
            name = cat.sublabelmaker(gid_list[ii], tree[jj]);
            stmt = sub_template;
            pos1 = stmt.indexOf("!!!");
            stmt.replace(pos1, 3, name);

            m_executor.lockForWrite();
            QSqlError qerror = executor(stmt);
            m_executor.unlock();
            if (qerror.isValid())
            {
                sqlerr("mainwindow.subtables_st for " + name, qerror);
            }
        }
        update_bar();
    }
}

void MainWindow::subtables_mapped(CATALOGUE& cat)
{
    QVector<QVector<QVector<int>>> tree = cat.get_tree();
    QVector<QString> gid_list = cat.get_gid_list();
    QString sub_template = cat.get_create_sub_template();
    QString qname = cat.get_qname();
    DWORD compass = GetCurrentThreadId();
    LPMSG mailbox = new MSG;
    int num_gid = gid_list.size();
    reset_bar(num_gid);

    auto insert_subtable = [=] (QString gid)
    {
        QString name, stmt, temp;
        int pos1, ancestry, cheddar;
        UINT message;
        for (int jj = 0; jj < tree.size(); jj++)
        {
            name = "T" + qname + "$" + gid;
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
                sqlerr("mainwindow.subtables_mapped for " + name, qerror);
            }
        }
        pos1 = gid.toInt();
        pos1 -= 1375000;
        message = WM_APP + pos1;
        if (!PostThreadMessageW(compass, message, 0, 0))
        {
            winwarn(L"subtables_mt");
        }
    };

    QFuture<void> bit_of_nothing = QtConcurrent::map(gid_list, insert_subtable);
    int enigma;
    bool aol = 0;
    while (!bit_of_nothing.isFinished())
    {
        QThread::msleep(100);
        aol = PeekMessageW(mailbox, NULL, WM_APP, WM_APP + 15000, PM_REMOVE);
        if (aol)
        {
            enigma = mailbox->message;
            enigma -= WM_APP;
            qDebug() << "Received message: " << enigma;
            update_bar();
        }
    }
}

void MainWindow::nobles_st(QVector<QVector<QString>>& stmts)
{
    for (int ii = 0; ii < stmts.size(); ii++)
    {
        m_executor.lockForWrite();
        QSqlError qerror = executor(stmts[ii][0]);
        m_executor.unlock();
        if (qerror.isValid())
        {
            sqlerr("mainwindow.nobles_st for " + stmts[ii][1], qerror);
        }
    }
    logger(stmts[0][1] + " and CSV nobles were inserted into the database.");
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

void MainWindow::create_csv_tables(CATALOGUE& cata, QString gid)
{
    QVector<QVector<QVector<int>>> csv_tree = cata.get_tree();
    QString csv_template = cata.get_csv_template();
    QString qname = cata.get_qname();
    QString stmt, tname;
    int pos1;

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

// Given a full path name and a string, print that string to file (UTF-8 encoding).
void sprinter(string full_path, string& content)
{
    HANDLE hfile = CreateFileA(full_path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, CREATE_ALWAYS, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-sprinter"); }
    DWORD bytes_written;
    DWORD file_size = (DWORD)content.size();
    if (!WriteFile(hfile, content.c_str(), file_size, &bytes_written, NULL)) { winerr(L"WriteFile-sprinter"); }
    if (!CloseHandle(hfile)) { winerr(L"CloseHandle-sprinter"); }
}
void wprinter(wstring full_path, wstring& content)
{
    string path8 = utf16to8(full_path);
    string content8 = utf16to8(content);
    HANDLE hfile = CreateFileA(path8.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, CREATE_ALWAYS, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-wprinter"); }
    DWORD bytes_written;
    DWORD file_size = (DWORD)content8.size();
    if (!WriteFile(hfile, content8.c_str(), file_size, &bytes_written, NULL)) { winerr(L"WriteFile-wprinter"); }
    if (!CloseHandle(hfile)) { winerr(L"CloseHandle-wprinter"); }
}
void qprinter(QString full_path, QString& content)
{
    string path8 = full_path.toStdString();
    string content8 = content.toStdString();
    HANDLE hfile = CreateFileA(path8.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, CREATE_ALWAYS, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-qprinter"); }
    DWORD bytes_written;
    DWORD file_size = (DWORD)content8.size();
    if (!WriteFile(hfile, content8.c_str(), file_size, &bytes_written, NULL)) { winerr(L"WriteFile-qprinter"); }
    if (!CloseHandle(hfile)) { winerr(L"CloseHandle-qprinter"); }
}

// Make an entry into the error log. If severe, terminate the application.
void err8(string func8)
{
    string name = db_root8 + "\\SCDA Error Log.txt";
    string message = timestamperA() + " Generic error: " + func8 + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
    exit(EXIT_FAILURE);
}
void winerr(wstring func)
{
    DWORD num = GetLastError();
    LPSTR buffer = new CHAR[512];
    string mod = "wininet.dll";
    LPCSTR modul = mod.c_str();
    FormatMessageA((FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE), GetModuleHandleA(modul), num, 0, buffer, 512, NULL);
    string winmessage(buffer, 512);
    delete[] buffer;
    string func8 = utf16to8(func);
    string name = db_root8 + "\\SCDA Error Log.txt";
    string message = timestamperA() + " Windows error #" + to_string(num) + " inside " + func8 + "\r\n" + winmessage + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
    exit(EXIT_FAILURE);
}
void warn(wstring func)
{
    string func8 = utf16to8(func);
    string name = db_root8 + "\\SCDA Error Log.txt";
    string message = timestamperA() + " Generic warning: " + func8 + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}
void warn8(string func8)
{
    string name = db_root8 + "\\SCDA Error Log.txt";
    string message = timestamperA() + " Generic warning: " + func8 + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}
void qwarn(QString qfunc)
{
    string func8 = qfunc.toStdString();
    string name = db_root8 + "\\SCDA Error Log.txt";
    string message = timestamperA() + " Warning: " + func8 + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}
void sqlwarn(wstring func, QSqlError qerror)
{
    string func8 = utf16to8(func);
    string name = db_root8 + "\\SCDA Error Log.txt";
    QString qmessage = qerror.text();
    string message = timestamperA() + " SQL warning inside " + func8 + "\r\n" + qmessage.toStdString() + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}
void winwarn(wstring func)
{
    DWORD num = GetLastError();
    LPSTR buffer = new CHAR[512];
    string mod = "wininet.dll";
    LPCSTR modul = mod.c_str();
    FormatMessageA((FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE), GetModuleHandleA(modul), num, 0, buffer, 512, NULL);
    string winmessage(buffer, 512);
    delete[] buffer;
    string func8 = utf16to8(func);
    string name = db_root8 + "\\SCDA Error Log.txt";
    string message = timestamperA() + " Windows warning #" + to_string(num) + " inside " + func8 + "\r\n" + winmessage + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}

void log(wstring note)
{
    string note8 = utf16to8(note);
    string name = db_root8 + "\\SCDA Process Log.txt";
    string message = timestamperA() + "  " + note8 + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hprinter == INVALID_HANDLE_VALUE) { warn(L"CreateFile-log"); }
    if (!begun_logging)
    {
        if (!DeleteFileA(name.c_str())) { warn(L"DeleteFile-log"); }
        if (!CloseHandle(hprinter)) { warn(L"CloseHandle-log"); }
        hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hprinter == INVALID_HANDLE_VALUE) { warn(L"CreateFile-log(after deletion)"); }
        begun_logging = 1;
    }
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}
void log8(string note8)
{
    string name = db_root8 + "\\SCDA Process Log.txt";
    string message = timestamperA() + "  " + note8 + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hprinter == INVALID_HANDLE_VALUE) { warn(L"CreateFile-log"); }
    if (!begun_logging)
    {
        if (!DeleteFileA(name.c_str())) { warn(L"DeleteFile-log"); }
        if (!CloseHandle(hprinter)) { warn(L"CloseHandle-log"); }
        hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hprinter == INVALID_HANDLE_VALUE) { warn(L"CreateFile-log(after deletion)"); }
        begun_logging = 1;
    }
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}
void qlog(QString qnote)
{
    string note8 = qnote.toStdString();
    string name = db_root8 + "\\SCDA Process Log.txt";
    string message = timestamperA() + "  " + note8 + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hprinter == INVALID_HANDLE_VALUE) { warn(L"CreateFile-log"); }
    if (!begun_logging)
    {
        if (!DeleteFileA(name.c_str())) { warn(L"DeleteFile-log"); }
        if (!CloseHandle(hprinter)) { warn(L"CloseHandle-log"); }
        hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hprinter == INVALID_HANDLE_VALUE) { warn(L"CreateFile-log(after deletion)"); }
        begun_logging = 1;
    }
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}

int clean(wstring& out, int mode)
{
    int count = 0;
    size_t pos1, pos2;
    pos1 = out.find(L'[', 0);
    if (pos1 < out.size())
    {
        pos2 = out.find(L']', pos1);
        out.erase(pos1, pos2 - pos1 + 1);
    }
    /*
    pos1 = out.find(L'(', 0);
    if (pos1 < out.size())
    {
        pos2 = out.find(L')', pos1);
        out.erase(pos1, pos2 - pos1 + 1);
    }
    */
    if (mode == 1)
    {
        pos1 = out.find(L'\'', 0);
        while (pos1 < out.size())
        {
            out.replace(pos1, 1, L"''");
            pos1 = out.find(L'\'', pos1 + 2);
        }
    }

    /*
    pos1 = out.find(L'-', 0);
    while (pos1 < out.size())
    {
        out.replace(pos1, 1, L"\-");
        pos1 = out.find(L'-', pos1 + 1);
    }
    pos1 = out.find(L',', 0);
    while (pos1 < out.size())
    {
        out.replace(pos1, 1, L"_");
        pos1 = out.find(L',', pos1 + 1);
    }
    pos1 = out.find(L'<', 0);
    while (pos1 < out.size())
    {
        out.replace(pos1, 1, L"less_than_");
        pos1 = out.find(L'<', pos1 + 10);
    }
    pos1 = out.find(L'>', 0);
    while (pos1 < out.size())
    {
        out.replace(pos1, 1, L"greater_than_");
        pos1 = out.find(L'<', pos1 + 13);
    }
    pos1 = out.find(L'/', 0);
    while (pos1 < out.size())
    {
        out.replace(pos1, 1, L"_or_");
        pos1 = out.find(L'/', pos1 + 4);
    }
    pos1 = out.find(L'+', 0);
    while (pos1 < out.size())
    {
        out.replace(pos1, 1, L"_and_");
        pos1 = out.find(L'+', pos1 + 5);
    }
    */
    while (1)
    {
        if (out.front() == L' ') { out.erase(0, 1); count++; }
        else { break; }
    }
    while (1)
    {
        if (out.back() == L' ') { out.pop_back(); }
        else { break; }
    }
    /*
    pos1 = out.find(L' ', 0);
    while (pos1 < out.size())
    {
        out.replace(pos1, 1, L"_");
        pos1 = out.find(L' ', pos1 + 1);
    }
    */
    return count;
}
int qclean(QString& bbq, int mode)
{
    int count = 0;
    int pos1, pos2;
    pos1 = bbq.indexOf('[');
    if (pos1 > 0)
    {
        pos2 = bbq.indexOf(']', pos1);
        bbq.remove(pos1, pos2 - pos1 + 1);
    }
    if (mode == 1)
    {
        pos1 = bbq.indexOf('\'');
        while (pos1 > 0)
        {
            bbq.replace(pos1, 1, "''");
            pos1 = bbq.indexOf('\'', pos1 + 2);
        }
    }
    while (1)
    {
        if (bbq.front() == ' ') { bbq.remove(0, 1); count++; }
        else { break; }
    }
    while (1)
    {
        if (bbq.back() == ' ') { bbq.remove(bbq.size() - 1, 1); }
        else { break; }
    }
    return count;
}

// Given a full path name, delete the file/folder.
void delete_file(string filename)
{
    HANDLE hfile = CreateFileA(filename.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr_bt("CreateFile-delete_file"); }
    if (!DeleteFileA(filename.c_str())) { winerr_bt("DeleteFile-delete_file"); }
    if (!CloseHandle(hfile)) { winerr_bt("CloseHandle-delete_file"); }
}
void delete_folder(string folder_name)
{
    wstring folder_wname = utf8to16(folder_name);
    wstring folder_search = folder_wname + L"\\*";
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    wstring file_name;
    do
    {
        file_name = folder_wname + L"\\" + info.cFileName;
        if (file_name.back() != L'.') { delete_file(file_name); }
    } while (FindNextFileW(hfile1, &info));

    BOOL yesno = RemoveDirectoryW(folder_wname.c_str());
    if (!yesno) { winerr_bt("RemoveDirectory-delete_folder"); }
}


wstring bin_memory(HANDLE& hfile)
{
    DWORD size = GetFileSize(hfile, NULL);
    DWORD bytes_read;
    LPWSTR buffer = new WCHAR[size / 2];
    if (!ReadFile(hfile, buffer, size, &bytes_read, NULL)) { winerr_bt("ReadFile-bin_memory"); }
    wstring bin(buffer, size / 2);
    delete[] buffer;
    return bin;
}
QString q_memory(wstring& full_path)
{
    HANDLE hfile = CreateFileW(full_path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr_bt("CreateFile-q_memory"); }
    DWORD size = GetFileSize(hfile, NULL);
    DWORD bytes_read;
    LPWSTR buffer = new WCHAR[size / 2];
    if (!ReadFile(hfile, buffer, size, &bytes_read, NULL)) { winerr_bt("ReadFile-q_memory"); }
    QString qfile = QString::fromWCharArray(buffer, size / 2);
    delete[] buffer;
    return qfile;
}
wstring w_memory(wstring& full_path)
{
    HANDLE hfile = CreateFileW(full_path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr_bt("CreateFile-w_memory"); }
    DWORD size = GetFileSize(hfile, NULL);
    DWORD bytes_read;
    LPWSTR buffer = new WCHAR[size / 2];
    if (!ReadFile(hfile, buffer, size, &bytes_read, NULL)) { winerr_bt("ReadFile-w_memory"); }
    wstring wfile(buffer, size / 2);
    delete[] buffer;
    return wfile;
}

// Insert the selected catalogues into the database.
void MainWindow::on_pB_insert_clicked()
{
    QList<QTreeWidgetItem *> catas_to_do = ui->TW_cataondrive->selectedItems();
    QVector<QVector<QString>> catas_in_db;  // Form [year][year, catalogues...]
    QMap<QString, int> map_cata;
    QString qyear, qname, qdesc, stmt;
    int year_index, cata_index;
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
void MainWindow::sqlerr(QString qmessage, QSqlError qerror)
{
    QString qcode = qerror.nativeErrorCode();
    string spath = sroots[location] + "\\SCDA Error Log.txt";
    QString qerrmess = qerror.text();
    QSqlError::ErrorType et = qerror.type();
    QString qtype = sqlerr_enum(et);
    string smessage = timestamperA() + " SQL ERROR: type " + qtype.toStdString() + ", #" + qcode.toStdString() + ", inside " + qmessage.toStdString() + "\r\n" + qerrmess.toStdString() + "\r\n\r\n";
    lock_guard lock(m_io);
    HANDLE hprinter = CreateFileA(spath.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)smessage.size();
    WriteFile(hprinter, smessage.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
    exit(EXIT_FAILURE);
}

// Threadsafe logger function for the most recent program runtime.
void MainWindow::logger(QString qnote)
{
    string note8 = qnote.toStdString();
    string name = utf16to8(wroots[location]) + "\\SCDA Process Log.txt";
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

vector<vector<string>> MainWindow::step2(sqlite3*& dbnoqt, sqlite3_stmt*& statement2, qint64& timer2, qint64& timer3)
{
    int type, col_count, size;  // Type: 1(int), 2(double), 3(string)
    QElapsedTimer t2, t3;
    t2.start();
    int error = sqlite3_step(statement2);
    timer2 = t2.restart();
    int ivalue;
    double dvalue;
    string svalue;
    vector<vector<string>> results;
    t3.start();
    while (error == 100)
    {
        col_count = sqlite3_column_count(statement2);
        results.push_back(vector<string>(col_count));
        for (int ii = 0; ii < col_count; ii++)
        {
            type = sqlite3_column_type(statement2, ii);
            switch (type)
            {
            case 1:
                ivalue = sqlite3_column_int(statement2, ii);
                results[results.size() - 1][ii] = to_string(ivalue);
                break;
            case 2:
                dvalue = sqlite3_column_double(statement2, ii);
                results[results.size() - 1][ii] = to_string(dvalue);
                break;
            case 3:
                size = sqlite3_column_bytes(statement2, ii);
                char* buffer = (char*)sqlite3_column_text(statement2, ii);
                svalue.assign(buffer, size);
                results[results.size() - 1][ii] = svalue;
                break;
            }
        }
        error = sqlite3_step(statement2);
    }
    if (error > 0 && error != 101)
    {
        sqlerror("step: " + to_string(error), dbnoqt);
    }
    timer3 = t3.restart();
    return results;
}

void MainWindow::judicator(int& control, int& report, QString qyear, QString qname)
{
    QString temp = qroots[location] + "\\" + qyear + "\\" + qname;
    wstring cata_wpath = temp.toStdWString();
    int num_gid = get_file_path_number(cata_wpath, L".csv");
    int workload = num_gid / cores;
    int bot = 0;
    int top = workload - 1;
    reset_bar(num_gid, "Adding spreadsheets to catalogue " + qname);
    QElapsedTimer timer, timer_prepare, timer_exec;

    // Establish the thread's connection to the database.
    m_db.lock();
    db = QSqlDatabase::addDatabase("QSQLITE", "judi");
    db.setDatabaseName(db_path);
    if (!db.open()) { sqlerr("db.open-judicator", db.lastError()); }
    QSqlQuery q(db);
    m_db.unlock();

    // Insert this catalogue into the catalogue index.
    bool fine = q.prepare("INSERT INTO TCatalogueIndex ( Year, Name, Description ) VALUES (?, ?, ?)");
    if (!fine) { sqlerr("prepare1-judicator", q.lastError()); }
    q.addBindValue(qyear);
    q.addBindValue("[" + qname + "]");
    q.addBindValue("Incomplete");
    fine = q.exec();
    if (!fine) { sqlerr("exec1-judicator", q.lastError()); }

    // Create a table for this catalogue's damaged CSVs.
    temp = "[" + qname + "$Damaged]";
    fine = q.prepare("CREATE TABLE IF NOT EXISTS " + temp + " (GID NUMERIC, [Number of Missing Data Entries] NUMERIC)");
    if (!fine) { sqlerr("prepare1.1-judicator", q.lastError()); }
    fine = q.exec();
    if (!fine) { sqlerr("exec1.1-judicator", q.lastError()); }

    // Launch the worker threads, which will iterate through the CSVs.
    vector<std::thread> peons;
    vector<int> controls;
    controls.assign(cores, 0);
    vector<int> reports;
    reports.assign(cores, 0);
    vector<int> prompt(3);  // Form [id, bot, top]
    QVector<QVector<QString>> all_thr_stmts(cores, QVector<QString>());  // Form [thread][statements]
    vector<vector<string>> std_stmts(cores, vector<string>());
    for (int ii = 0; ii < cores; ii++)
    {
        prompt[0] = ii;
        prompt[1] = bot;
        prompt[2] = top;
        std::thread thr(&MainWindow::insert_csvs, this, std::ref(all_thr_stmts[ii]), std::ref(controls[ii]), std::ref(reports[ii]), cata_wpath, prompt);
        //std::thread thr(&MainWindow::insert_csvs, this, std::ref(std_stmts[ii]), std::ref(controls[ii]), cata_wpath, prompt);
        peons.push_back(std::move(thr));
        bot += workload;
        if (ii < cores - 1) { top += workload; }
        else { top = num_gid - 1; }
    }

    // Create the catalogue's primary table.
    while (all_thr_stmts[0].size() < 2) { Sleep(50); }
    QString cata_desc = all_thr_stmts[0][1];  // Used later.
    m_jobs[0].lock();
    temp = all_thr_stmts[0][0];
    all_thr_stmts[0].remove(0, 2);
    m_jobs[0].unlock();
    fine = q.prepare(temp);
    if (!fine) { sqlerr("prepare2-judicator", q.lastError()); }
    fine = q.exec();
    if (!fine) { sqlerr("exec2-judicator", q.lastError()); }

    // Loop through the worker threads, inserting their statements into the database.
    int active_thread = 0;
    int inert_threads = 0;
    int pile, progress, num1, num2;
    QVector<QString> desk;
    //QVector<qint64> prep(100);
    //QVector<qint64> exec(100);
    qint64 time1;
    timer.start();
    timer_prepare.start();
    timer_exec.start();
    while (control == 0 && jobs_done < jobs_max)
    {
        progress = 0;
        for (int ii = 0; ii < cores; ii++)
        {
            progress += reports[ii];
        }
        report = progress;
        qDebug() << "Updated progress bar: " << timer.restart();
        pile = all_thr_stmts[active_thread].size();
        if (pile > 0)
        {
            m_jobs[active_thread].lock();
            desk = all_thr_stmts[active_thread];
            all_thr_stmts[active_thread].clear();
            m_jobs[active_thread].unlock();

            for (int ii = 0; ii < desk.size(); ii++)
            {
                fine = q.prepare(desk[ii]);
                if (!fine)
                {
                    sqlerr("prepare3-judicator", q.lastError());
                }
                timer_exec.restart();
                fine = q.exec();
                if (!fine) { sqlerr("exec3-judicator", q.lastError()); }
            }
        }
        active_thread++;
        if (active_thread >= cores) { active_thread = 0; }
    }
    if (control == 1)  // Before shutting down, finish inserting the CSVs that were in queue.
    {
        for (size_t ii = 0; ii < controls.size(); ii++)
        {
            controls[ii] = 1;
        }
        active_thread = 0;
        while (inert_threads < cores)  // Keep working while the threads finish their final tasks.
        {
            inert_threads = 0;
            for (int ii = 0; ii < cores; ii++)  // Count how many worker threads have stopped.
            {
                if (controls[ii] < 0)
                {
                    inert_threads++;
                }
            }
            pile = all_thr_stmts[active_thread].size();
            if (pile > 0)
            {
                m_jobs[active_thread].lock();
                desk = all_thr_stmts[active_thread];
                all_thr_stmts[active_thread].clear();
                m_jobs[active_thread].unlock();

                for (int ii = 0; ii < desk.size(); ii++)
                {
                    fine = q.prepare(desk[ii]);
                    if (!fine) { sqlerr("prepare3-judicator", q.lastError()); }
                    fine = q.exec();
                    if (!fine) { sqlerr("exec3-judicator", q.lastError()); }
                }
            }
            active_thread++;
            if (active_thread >= cores) { active_thread = 0; }
        }
        for (int ii = 0; ii < cores; ii++)  // When all worker threads are done, do one final sweep of the queue.
        {
            pile = all_thr_stmts[ii].size();
            if (pile > 0)
            {
                m_jobs[ii].lock();
                desk = all_thr_stmts[ii];
                all_thr_stmts[ii].clear();
                m_jobs[ii].unlock();

                for (int jj = 0; jj < desk.size(); jj++)
                {
                    fine = q.prepare(desk[jj]);
                    if (!fine) { sqlerr("prepare3-judicator", q.lastError()); }
                    fine = q.exec();
                    if (!fine) { sqlerr("exec3-judicator", q.lastError()); }
                }
            }
        }
        progress = 0;
        for (int ii = 0; ii < cores; ii++)  // Do one final update of the progress bar.
        {
            progress += reports[ii];
        }
        report = progress;
    }
    for (auto& th : peons)
    {
        if (th.joinable())
        {
            th.join();
        }
    }
    if (control == 0)  // If we are done without being cancelled, we can mark the catalogue as 'complete'.
    {
        temp = "[" + qname + "]";
        fine = q.prepare("UPDATE TCatalogueIndex SET Description = ? WHERE Name = ?");
        if (!fine) { sqlerr("prepare4-judicator", q.lastError()); }
        q.addBindValue(cata_desc);
        q.addBindValue(temp);
        fine = q.exec();
        if (!fine) { sqlerr("exec4-judicator", q.lastError()); }
        logger("Catalogue " + qname + " completed its CSV insertion.");
    }
    else
    {
        logger("Catalogue " + qname + " had its CSV insertion cancelled.");
    }
    threads_working = 0;
}

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
        int damaged_csv = 0;
        QVector<QVector<QString>> text_vars, data_rows;
        int my_report = 0;
        int my_status = 0;
        m_db.lock();
        connection_name = "TEMPORARY - REMOVE ASAP";
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connection_name);  // Every thread gets one.
        db.setDatabaseName(db_path);
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
                data_rows = cata.extract_data_rows(qfile, damaged_csv);

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

// For a given catalogue, insert all of its CSVs into the database, iterating by GID.
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

// Given an INSERT statement template QString and the position of a parameter char ('?') inside it,
// replace the char with the given QString (wrapped in ''). The return value is the next such parameter char.
int insert_val(QString& stmt, int pos, QString val)
{
    clean(val, 1);
    stmt.replace(pos, 1, val);
    int pos1 = stmt.indexOf('?', pos);
    return pos1;
}
int insert_text(QString& stmt, int pos, QString text)
{
    QString temp = "[" + text + "]";
    stmt.replace(pos, 1, temp);
    int pos1 = stmt.indexOf('?', pos);
    return pos1;
}

// Close the db object, delete the db file, and remake the db file with a new CatalogueIndex table.
void MainWindow::reset_db(string& db_path)
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


	