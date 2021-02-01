#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "basictools.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    percent = 0;
    work_done = 0;
    total_work = 0;

    ui->setupUi(this);
    ui->comboBox_drives->addItem("D:");
    ui->comboBox_drives->addItem("E:");
    ui->comboBox_drives->addItem("F:");
    ui->comboBox_drives->addItem("G:");
    ui->progressBar->setValue(percent);

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(db_path);
    if (!db.open()) { sqlerr(L"db.open-MainWindow constructor", db.lastError()); }
}

MainWindow::~MainWindow()
{
    delete ui;
}

QStringList scan_years(wstring wdrive)
{
    vector<wstring> year_folders = get_subfolders(wdrive);
    QStringList year_list;
    wstring year;
    size_t pos1;
    int YEAR;
    for (size_t ii = 0; ii < year_folders.size(); ii++)
    {
        pos1 = year_folders[ii].rfind(L'\\');
        year = year_folders[ii].substr(pos1 + 1);
        try
        {
            YEAR = stoi(year);
        }
        catch (invalid_argument& ia)
        {
            warn8(ia.what());
            continue;
        }
        year_list << QString::number(YEAR);
    }
    return year_list;
}

QSqlQuery select_column_title(QString column, QString table)
{
    QSqlQuery query;
    if (column == "*")
    {
        query.prepare("SELECT " + column + " FROM \"" + table + "\" ;");
    }
    else
    {
        query.prepare("SELECT \"" + column + "\" FROM \"" + table + "\" ;");
    }
    return query;
}

int CSV_DB::read()
{
    size_t posnl1, posnl2, pos1, pos2;
    wstring line, temp1, temp2;
    vector<wstring> sub_columns;
    vector<wstring> temp_columns;
    vector<wstring> temp_headings = { L"\0" };
    wchar_t math;
    int spaces = 0;
    int indentation = 0;

    // Populate the variables.
    pos1 = 0;
    pos2 = 0;
    while (1)
    {
        pos1 = wfile.find(L'=', pos1 + 1);
        if (pos1 > wfile.size()) { break; }
        math = wfile[pos1 - 1];
        if (math == L'<' || math == L'>')
        {
            continue;
        }
        variables.push_back(vector<string>());

        pos2 = wfile.rfind(L'"', pos1); pos2++;
        temp1 = wfile.substr(pos2, pos1 - pos2);
        clean(temp1, 0);
        temp2 = L"\"" + temp1 + L"\"";
        variables[variables.size() - 1].push_back(utf16to8(temp2));

        pos2 = wfile.find(L'"', pos1);
        temp1 = wfile.substr(pos1 + 1, pos2 - pos1 - 1);
        if (temp1.size() == 0) { err(L"csvread - GID " + to_wstring(GID)); return 1; }
        clean(temp1, 1);
        temp2 = L"\'" + temp1 + L"\'";
        variables[variables.size() - 1].push_back(utf16to8(temp2));
    }

    // Populate the columns and values.
    bool subtable = 0;
    int column_count = 0;
    posnl2 = wfile.find(L'\n', pos2);  // Start from the closing quotation mark of the last variable.
    while (1) // For every line in the csv file...
    {
        posnl1 = posnl2;
        posnl2 = wfile.find(L'\n', posnl1 + 1);
        if (posnl2 < wfile.size() - 1) { line = wfile.substr(posnl1, posnl2 - posnl1); }
        else { break; }  // Either make the wstring "line" for this line, or break if EoF.

        pos1 = 0;
        while (1) // For every set of quotation marks on this line...
        {
            pos1 = line.find(L'"', pos1 + 1);
            if (pos1 < line.size())
            {
                pos2 = line.find(L'"', pos1 + 1);
                temp1 = line.substr(pos1 + 1, pos2 - pos1 - 1);
                spaces = clean(temp1, 0);
                temp_columns.push_back(temp1);
                pos1 = pos2;
            }
            else { break; }
        }

        if (temp_columns.size() > 1)
        {
            subtable = 1;
            sub_columns = temp_columns;
            temp_columns.clear();
            continue;
        }
        else if (temp_columns[0] == L"Note")
        {
            break;
        }
        else
        {
            if (spaces == indentation)
            {
                temp_headings.back() = temp_columns[0];
            }
            else if (spaces > indentation)
            {
                indentation = spaces;
                temp_headings.push_back(temp_columns[0]);
            }
            else if (spaces == 0)
            {
                indentation = spaces;
                temp_headings.clear();
                temp_headings.push_back(temp_columns[0]);
            }
            else
            {
                indentation = spaces;
                temp_headings.pop_back();
                temp_headings.back() = temp_columns[0];
            }
            temp_columns.clear();

            for (size_t ii = 0; ii < temp_headings.size(); ii++)
            {
                if (ii == 0) { temp1 = temp_headings[ii]; }
                else
                {
                    temp1 += L" + ";
                    temp1 += temp_headings[ii];
                }
            }

            if (subtable)
            {
                for (size_t ii = 1; ii < sub_columns.size(); ii++)
                {
                    temp2 = L"\"" + temp1 + L" + " + sub_columns[ii] + L"\"";
                    columns.push_back(utf16to8(temp2));
                }

                pos1 = 0;
                for (size_t ii = 0; ii < sub_columns.size() - 1; ii++)
                {
                    pos1 = line.find(L',', pos1 + 1); pos1++;
                    pos2 = line.find(L',', pos1 + 1);
                    temp1 = line.substr(pos1, pos2 - pos1);

                    // Some Stats Canada spreadsheets have these "unknown" placeholders for values...
                    if (temp1 == L"..")
                    {
                        if (making_the_model)
                        {
                            return 2;
                        }
                        else
                        {
                            if (is_int[column_count])
                            {
                                temp1 = L"0";
                            }
                            else
                            {
                                temp1 = L"0.0";
                            }
                            column_count++;
                        }
                    }
                    else
                    {
                        if (is_int[column_count])
                        {
                            try { int_val.push_back(stoi(temp1)); }
                            catch (invalid_argument& ia) { err8(ia.what()); }
                        }
                        else
                        {
                            try { double_val.push_back(stod(temp1)); }
                            catch (invalid_argument& ia) { err8(ia.what()); }
                        }
                        column_count++;
                    }
                }
            }
            else
            {
                temp2 = L"\"" + temp1 + L"\"";
                columns.push_back(utf16to8(temp2));

                pos1 = line.rfind(L',', line.size() - 2); pos1++;
                pos2 = line.find(L'\r', pos1);
                temp1 = line.substr(pos1, pos2 - pos1);

                // Some Stats Canada spreadsheets have these ".." placeholders for unknown values...
                if (temp1 == L"..")
                {
                    if (making_the_model)
                    {
                        return 2;
                    }
                    else
                    {
                        if (is_int[column_count])
                        {
                            temp1 = L"0";
                            try { int_val.push_back(stoi(temp1)); }
                            catch (invalid_argument& ia) { err8(ia.what()); }
                        }
                        else
                        {
                            temp1 = L"0.0";
                            try { double_val.push_back(stod(temp1)); }
                            catch (invalid_argument& ia) { err8(ia.what()); }
                        }
                        column_count++;
                    }
                }
                else if (making_the_model)
                {
                    pos1 = temp1.find(L'.', 0);
                    if (pos1 < temp1.size())
                    {
                        is_int.push_back(0);
                        try { double_val.push_back(stod(temp1)); }
                        catch (invalid_argument& ia) { err8(ia.what()); }
                    }
                    else
                    {
                        is_int.push_back(1);
                        try { int_val.push_back(stoi(temp1)); }
                        catch (invalid_argument& ia) { err8(ia.what()); }
                    }
                }
                else
                {
                    if (is_int[column_count])
                    {
                        try { int_val.push_back(stoi(temp1)); }
                        catch (invalid_argument& ia) { err8(ia.what()); }
                    }
                    else
                    {
                        try { double_val.push_back(stod(temp1)); }
                        catch (invalid_argument& ia) { err8(ia.what()); }
                    }
                    column_count++;
                }
            }
        }
    }
    return 0;
}
vector<string> CSV_DB::get_varicols()
{
    vector<string> varicols = columns;
    vector<string>::iterator it;
    for (size_t ii = variables.size() - 1; ii >= 0; ii--)
    {
        it = varicols.begin();
        varicols.insert(it, variables[ii][0]);
    }
    return varicols;
}
string CSV_DB::get_insert()
{
    string statement = "INSERT INTO \"T" + cata_number + "\" ( GID, ";
    for (size_t ii = 0; ii < variables.size(); ii++)
    {
        statement += variables[ii][0];
        statement += ", ";
    }
    for (size_t ii = 0; ii < columns.size(); ii++)
    {
        statement += columns[ii];
        if (ii < columns.size() - 1) { statement += ", "; }
    }
    statement += ") VALUES (" + to_string(GID) + ", ";

    int int_counter = 0;
    int double_counter = 0;
    for (size_t ii = 0; ii < variables.size(); ii++)
    {
        statement += variables[ii][1] + ", ";
    }
    for (size_t ii = 0; ii < columns.size(); ii++)
    {
        if (is_int[ii])
        {
            statement += to_string(int_val[int_counter]);
            int_counter++;
        }
        else
        {
            statement += to_string(double_val[double_counter]);
            double_counter++;
        }

        if (ii < columns.size() - 1)
        {
            statement += ", ";
        }
    }
    statement += " ) ";

    statement += "ON CONFLICT ( GID ) WHERE true DO NOTHING ";

    statement += ";";

    return statement;
}
int CSV_DB::bad_apple(wstring apple_full_name)
{
    delete_file(apple_full_name);

    size_t pos1 = apple_full_name.rfind(L'\\', apple_full_name.size() - 1);
    wstring folder = apple_full_name.substr(0, pos1);
    wstring short_name = apple_full_name.substr(pos1 + 1);
    size_t pos2 = apple_full_name.rfind(L'\\', pos1 - 1);
    wstring archive = apple_full_name.substr(0, pos2);
    wstring temp1 = archive.substr(archive.size() - 4, 4);
    archive += L"\\" + temp1 + L" archive.bin";
    HANDLE hfile = CreateFileW(archive.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { warn(L"CreateFile-bad_apple"); return 1; }
    wstring wfile = bin_memory(hfile);
    if (!CloseHandle(hfile)) { warn(L"CloseHandle-bad_apple"); return 2; }

    wstring cata16 = utf8to16(cata_number);
    pos1 = wfile.find(cata16, 0);
    pos1 = wfile.find(L' ', pos1);
    pos1++;
    pos2 = wfile.find(L"||", pos1);
    pos2--;
    wstring url = wfile.substr(pos1, pos2 - pos1);
    pos1 = url.find(L"GID=", 0);
    pos1 += 4;
    pos2 = url.find(L'&', pos1);
    url.replace(pos1, pos2 - pos1, to_wstring(GID));

    int error = download(url, folder, short_name);
    if (error) { warn(L"download-bad_apple"); return 3; }

    return 0;
}

void CATA_DB::init(wstring folder_path)
{
    folder = folder_path;
    size_t pos1, pos2;
    wstring temp1, temp2;
    col_width = 0;

    wstring folder_search = folder + L"\\*.csv";
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { err(L"FindFirstFile-init"); }
    wstring sample_name = folder + L"\\" + info.cFileName;
    HANDLE hfile2 = CreateFileW(sample_name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile2 == INVALID_HANDLE_VALUE) { err(L"CreateFile-init"); }

    wstring wfile = bin_memory(hfile2);
    pos1 = wfile.rfind(L"Catalogue Number ", wfile.size() - 4);
    pos1 += 17;
    pos2 = wfile.find(L'.', pos1);
    temp1 = wfile.substr(pos1, pos2 - pos1);
    cata_number = utf16to8(temp1);
    table_name = "T" + cata_number;

    pos1 = wfile.find(L"\"", 0); pos1++;
    pos2 = wfile.find(L"\"", pos1);
    temp1 = wfile.substr(pos1, pos2 - pos1);
    pos1 = 0;
    do
    {
        pos1++;
        pos1 = temp1.find(L'(', pos1);
        if (pos1 < temp1.size())
        {
            pos2 = temp1.find(L')', pos1);
            temp1.erase(pos1, pos2 - pos1 + 1);
        }
    } while (pos1 < temp1.size());
    cata_name = utf16to8(temp1);

    CSV_DB model(wfile, -1, cata_name);
    int error = model.read();
    if (error > 0)
    {
        if (error == 2)
        {
            while (FindNextFileW(hfile1, &info))
            {
                sample_name = folder + L"\\" + info.cFileName;
                hfile2 = CreateFileW(sample_name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hfile2 == INVALID_HANDLE_VALUE) { err(L"CreateFile-init(bogus model-making)"); }

                wfile = bin_memory(hfile2);
                pos1 = wfile.rfind(L"Catalogue Number ", wfile.size() - 4);
                pos1 += 17;
                pos2 = wfile.find(L'.', pos1);
                temp1 = wfile.substr(pos1, pos2 - pos1);
                cata_number = utf16to8(temp1);
                table_name = "T" + cata_number;

                pos1 = wfile.find(L"\"", 0); pos1++;
                pos2 = wfile.find(L"\"", pos1);
                temp1 = wfile.substr(pos1, pos2 - pos1);
                pos1 = 0;
                do
                {
                    pos1++;
                    pos1 = temp1.find(L'(', pos1);
                    if (pos1 < temp1.size())
                    {
                        pos2 = temp1.find(L')', pos1);
                        temp1.erase(pos1, pos2 - pos1 + 1);
                    }
                } while (pos1 < temp1.size());
                cata_name = utf16to8(temp1);

                CSV_DB model(wfile, -1, cata_name);
                error = model.read();
                if (error == 0) { break; }
            }
        }
    }
    variables = model.get_variables();
    column_header = model.get_columns();
    is_int = model.get_value_types();
    if (!FindClose(hfile1)) { warn(L"FindClose-init"); }
    if (!CloseHandle(hfile2)) { warn(L"CloseHandle-init"); }
    log8(cata_number + " initialized.\r\n");
}
void CATA_DB::prepare_table(QSqlDatabase& db)
{
    string sql = "CREATE TABLE IF NOT EXISTS \"" + table_name + "\" ( GID INTEGER PRIMARY KEY, ";
    for (size_t ii = 0; ii < variables.size(); ii++)
    {
        sql += variables[ii][0];
        sql += " TEXT, ";
    }
    for (size_t ii = 0; ii < column_header.size(); ii++)
    {
        sql += column_header[ii];
        if (is_int[ii]) { sql += " INTEGER, "; }
        else { sql += " REAL, "; }
    }
    sql.pop_back(); sql.pop_back();
    sql.append(" );");
    QSqlQuery stmt(db);
    QString sql2 = QString::fromUtf8(sql.c_str());
    stmt.prepare(sql2);
    if (!stmt.exec())
    {
        sqlerr(L"prepare_table", stmt.lastError());
    }
    log8(cata_number + " table prepared.\r\n");
}
void CATA_DB::populate_table(QSqlDatabase& db)
{
    wstring folder_search = folder + L"\\*.csv";
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr(L"FindFirstFile-populate_table"); }
    HANDLE hfile2, hfile3;
    wstring short_name, full_name, wfile, temp1, sql;
    size_t pos1, pos2;
    int GID, error;

    do
    {
        short_name = info.cFileName;
        pos1 = short_name.find(L"(", 0); pos1++;
        pos2 = short_name.find(L")", pos1);
        temp1 = short_name.substr(pos1, pos2 - pos1);
        try { GID = stoi(temp1); }
        catch (invalid_argument& ia) { err8(ia.what()); }

        pos1 = short_name.find(L')', 0);
        pos1 += 2;
        pos2 = short_name.rfind(L'.', short_name.size() - 1);
        temp1 = short_name.substr(pos1, pos2 - pos1);

        full_name = folder + L"\\" + short_name;
        hfile2 = CreateFileW(full_name.c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hfile2 == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-populate_table"); }
        wfile = bin_memory(hfile2);
        if (!CloseHandle(hfile2)) { winwarn(L"CloseHandle-populate_table"); }

        CSV_DB cdb(wfile, GID, cata_number, is_int);
        error = cdb.read();
        if (error > 0) { err(L"CSV_DB read error #" + to_wstring(error)); }
        string error_msg;
        string sql;

        sql = cdb.get_insert();
        QSqlQuery stmt(db);
        QString sql2 = QString::fromUtf8(sql.c_str());
        stmt.prepare(sql2);
        if (!stmt.exec())
        {
            sqlwarn(L"populate_table", stmt.lastError());
            if (cdb.bad_apple(full_name))
            {
                err(L"really_bad_apple");
            }
            else
            {
                hfile3 = CreateFileW(full_name.c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hfile3 == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-bad_apple"); }
                wfile = bin_memory(hfile3);
                if (!CloseHandle(hfile3)) { winwarn(L"CloseHandle-populate"); }
                CSV_DB cdb2(wfile, GID, cata_number);
                error = cdb2.read();
                if (error > 0) { err(L"CSV_DB (bad_apple) read error #" + to_wstring(error)); }
                sql = cdb2.get_insert();
                QSqlQuery stmt2(db);
                QString sql3 = QString::fromUtf8(sql.c_str());
                stmt2.prepare(sql3);
                if (!stmt2.exec()) { sqlerr(L"populate_table-bad_apple", stmt2.lastError()); }
            }
        }
    } while (FindNextFileW(hfile1, &info));
    log8(cata_number + " table populated.\r\n");
}
string CATA_DB::get_table_name()
{
    return table_name;
}

CATA_DB TREE_DB::tablemaker(QSqlDatabase& db, wstring cata_folder)
{
    CATA_DB cata(cata_folder);
    cata.prepare_table(db);
    cata.populate_table(db);
    return cata;
}
void TREE_DB::tree_assembly(QSqlDatabase& db, vector<vector<wstring>>& cf, int& tw)
{
    int num_catalogues = 0;
    root_folder = cf[0][0].substr(0, 2);
    cata_folders = cf;
    for (size_t ii = 0; ii < cata_folders.size(); ii++)
    {
        trunk.push_back(vector<CATA_DB>());
        num_catalogues += (int)cata_folders[ii].size();
    }
    tw = num_catalogues;
    for (size_t ii = 0; ii < cata_folders.size(); ii++)
    {
        for (size_t jj = 0; jj < cata_folders[ii].size(); jj++)
        {
            trunk[ii].push_back(tablemaker(db, cata_folders[ii][jj]));
        }
    }

}
QStringList TREE_DB::get_table_list_years(vector<wstring>& year_folders)
{
    QStringList table_list;
    QString table_name;
    vector<wstring> cata_folder_list;
    wstring cata_name;
    size_t pos1;
    for (size_t ii = 0; ii < year_folders.size(); ii++)
    {
        cata_folder_list = get_subfolders(year_folders[ii]);
        for (size_t jj = 0; jj < cata_folder_list.size(); jj++)
        {
            pos1 = cata_folder_list[jj].rfind(L"\\");
            cata_name = cata_folder_list[jj].substr(pos1 + 1);
            table_name = QString::fromStdWString(L"T" + cata_name);
            table_list.append(table_name);
        }
    }
    return table_list;
}



void MainWindow::workload(int work)
{
    mpb.lock();
    total_work += work;
    mpb.unlock();
}

void MainWindow::progress()
{
    mpb.lock();
    work_done++;
    mpb.unlock();
}

void MainWindow::build_tree(QVector<QVector<QString>>& qtree)
{
    QVector<QTreeWidgetItem*> qroots;
    QTreeWidgetItem *item;
    for (qsizetype ii = 0; ii < qtree.size(); ii++)
    {
        item = new QTreeWidgetItem(ui->treeWidget_Scan);
        item->setText(0, qtree[ii][0]);
        item->setText(1, " ");
        qroots.append(item);
        add_children(item, qtree[ii]);
    }
    ui->treeWidget_Scan->addTopLevelItems(qroots);
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

void MainWindow::get_columns_from_table(QStringList& column_list, QString& table_name)
{
    QString column_name;
    QSqlQuery qselect = select_column_title("*", table_name);
    if (!qselect.exec()) { sqlerr(L"qselect.exec-get_columns_from_table", qselect.lastError()); }
    while (qselect.next())
    {
        column_name = qselect.value(0).toString();
        column_list.append(column_name);
    }
}

void MainWindow::set_model_string_list(QStringList& qstring_list)
{
    mdl.setStringList(qstring_list);
}

void MainWindow::on_pushButton_Scan_clicked()
{
    vector<vector<wstring>> wtree = get_subfolders2(wdrive);
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

    /*
    QStringList year_list = scan_years(wdrive);
    ui->listWidget_yearlist->addItems(year_list);
    */
}

void MainWindow::on_comboBox_drives_currentTextChanged(const QString &arg1)
{
    set_wdrive(arg1.toStdWString());
}

void MainWindow::on_pushButton_SelectAll_clicked()
{
    /*
    int year_count = ui->treeWidget_Scan->count();
    for (int ii = 0; ii < year_count; ii++)
    {
        ui->listWidget_yearlist->item(ii)->setSelected(true);
    }
    */
}

void MainWindow::on_pushButton_Commit_clicked()
{
    vector<vector<wstring>> cata_folders(1, vector<wstring>());
    QList<QTreeWidgetItem *> catas_to_do = ui->treeWidget_Scan->selectedItems();
    QVector<QString> years_loaded;
    QString qyear, qcata;
    wstring cata_path;
    qsizetype year_index = 0;
    for (qsizetype ii = 0; ii < catas_to_do.count(); ii++)
    {
        qyear = catas_to_do[ii]->text(0);
        qcata = catas_to_do[ii]->text(1);
        for (qsizetype jj = 0; jj < years_loaded.size(); jj++)
        {
            if (qyear == years_loaded[jj])
            {
                year_index = jj;
                break;
            }
            else if (jj == years_loaded.size() - 1)
            {
                year_index = years_loaded.size();
                years_loaded.append(qyear);
                cata_folders.push_back(vector<wstring>());
            }
        }
        cata_path = wdrive + L"\\" + qyear.toStdWString() + L"\\" + qcata.toStdWString();
        cata_folders[year_index].push_back(cata_path);
    }

    treebeard.tree_assembly(db, cata_folders, total_work);
}

void MainWindow::on_pushButton_ThrCommit_clicked()
{
    vector<wstring> cata_folders;
    QList<QTreeWidgetItem *> catas_to_do = ui->treeWidget_Scan->selectedItems();
    QStringList qlist;
    QString qyear, qcata;
    wstring cata_path;
    for (qsizetype ii = 0; ii < catas_to_do.count(); ii++)
    {
        qyear = catas_to_do[ii]->text(0);
        qcata = catas_to_do[ii]->text(1);
        cata_path = wdrive + L"\\" + qyear.toStdWString() + L"\\" + qcata.toStdWString();
        cata_folders.push_back(cata_path);
    }

    WORKTHREAD peon(db);
    peon.wpreload(cata_folders);
    connect(&peon, &WORKTHREAD::jobsdone, this, &MainWindow::progress);
    connect(&peon, &WORKTHREAD::setgoal, this, &MainWindow::workload);
    QFuture<void> bbq = QtConcurrent::run(QThreadPool::globalInstance(), &WORKTHREAD::begin, 1);
}

void MainWindow::on_pushButton_ShowTables_clicked()
{
    QStringList table_list = db.tables(QSql::Tables);
    if (table_list.size() < 1) { sqlerr(L"db.tables-pushButton_ShowTables", db.lastError()); }
    set_model_string_list(table_list);
    ui->listView_columns->setModel(&mdl);
}

void MainWindow::on_pushButton_ShowColumns_clicked()
{
    QList<QTreeWidgetItem *> catas_to_do = ui->treeWidget_Scan->selectedItems();
    QString qcata;
    QStringList table_list;
    for (qsizetype ii = 0; ii < catas_to_do.size(); ii++)
    {
        qcata = catas_to_do[ii]->text(1);
        table_list.append("T" + qcata);
    }

    QStringList column_list;
    for (qsizetype ii = 0; ii < table_list.size(); ii++)
    {
        get_columns_from_table(column_list, table_list[ii]);
    }
    set_model_string_list(column_list);
    ui->listView_columns->setModel(&mdl);
}

void MainWindow::on_treeWidget_Scan_itemClicked(QTreeWidgetItem *item, int column)
{
    // add "select all elements in year" if parent item is clicked.
}


