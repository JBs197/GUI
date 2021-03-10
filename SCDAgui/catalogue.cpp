#include "catalogue.h"

using namespace std;

// Return a subtable name, given GID and genealogy.
QString CATALOGUE::sublabelmaker(QString& gid, QVector<QVector<int>>& family)
{
    QString stname = "[" + qname + "$" + gid;
    int ancestry = family[0].size();
    int cheddar = 2;
    QString temp;
    for (int ii = 0; ii < ancestry; ii++)
    {
        for (int jj = 0; jj < cheddar; jj++)
        {
            stname += "$";
        }
        cheddar++;
        temp = QString::number(family[0][ii]);
        stname += temp;
    }
    stname += "]";
    return stname;
}

// Load on-disk folder paths for the catalogue folder.
void CATALOGUE::set_path(QString& cata_path)
{
    qpath = cata_path;
    wpath = cata_path.toStdWString();
}
void CATALOGUE::set_wpath(wstring& cata_wpath)
{
    wpath = cata_wpath;
    qpath = QString::fromStdWString(cata_wpath);
}

// Load the catalogue's significant variable names into the object.
void CATALOGUE::initialize_table()
{
    int pos1, pos2;
    size_t wpos1, wpos2;
    wstring wtemp;
    QString temp1;

    wstring folder_search = wpath + L"\\*.csv";
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr_bt("FindFirstFile-cata.initialize_table"); }
    wstring sample_name = wpath + L"\\" + info.cFileName;

    qfile = q_memory(sample_name);
    pos1 = qfile.lastIndexOf("Catalogue Number ", qfile.size() - 4);
    pos1 += 17;
    pos2 = qfile.indexOf('.', pos1);
    qname = qfile.mid(pos1, pos2 - pos1);
    wname = qname.toStdWString();
    tname = "T" + qname;
    sname = utf16to8(wname);

    wpos1 = sample_name.find(L'(');
    csv_trunk = sample_name.substr(0, wpos1);

    wpos1 = csv_trunk.find(L'\\');
    wpos2 = csv_trunk.find(L'\\', wpos1 + 1);
    wtemp = csv_trunk.substr(wpos1 + 1, wpos2 - wpos1 - 1);
    qyear = QString::fromStdWString(wtemp);

    pos1 = qfile.indexOf("\""); pos1++;
    pos2 = qfile.indexOf("\"", pos1);
    temp1 = qfile.mid(pos1, pos2 - pos1);
    pos1 = 0;
    do
    {
        pos1++;
        pos1 = temp1.indexOf('(', pos1);
        if (pos1 >= 0)
        {
            pos2 = temp1.indexOf(')', pos1);
            temp1.remove(pos1, pos2 - pos1 + 1);
        }
    } while (pos1 >= 0);
    qdescription = temp1;

    model.scan(qfile, qname);  // This will populate the embedded CSV object.
    make_name_tree();
    tree = model.get_model_tree();
    model_text_variables = model.get_text_variables();
    multi_column = model.get_multi_column();
    column_titles = model.get_column_titles();
    row_titles = model.get_row_titles();

    if (!FindClose(hfile1)) { winerr_bt("FindClose-cata.initialize_table"); }
}
void CATALOGUE::initialize_threading(int num)
{
    jobs.resize(num);
    thr_stmts.resize(num, QVector<QVector<QString>>());
    bot_top.resize(num, vector<int>(2));
}

// Populates the 'csv_branches' and 'gid_list' vectors.
void CATALOGUE::make_name_tree()
{
    size_t pos0 = csv_trunk.size();
    csv_branches = get_file_path_endings(wpath, pos0);
    pos0 = csv_branches.size();
    gid_list.resize(pos0);
    size_t pos1, pos2;
    wstring wtemp;
    for (size_t ii = 0; ii < pos0; ii++)
    {
        pos1 = csv_branches[ii].find(L'(');
        pos2 = csv_branches[ii].find(L')', pos1 + 1);
        wtemp = csv_branches[ii].substr(pos1 + 1, pos2 - pos1 - 1);
        gid_list[ii] = QString::fromStdWString(wtemp);
    }
}

// Worker thread functions.
void CATALOGUE::basic_input(QVector<QString> input)
{
    qyear = input[0];
    qname = input[1];
    qdescription = input[2];
}
void CATALOGUE::set_tree(QVector<QVector<QVector<int>>> sapling)
{
    tree = sapling;
}
void CATALOGUE::set_gid_list(QVector<QString> list)
{
    gid_list = list;
}
void CATALOGUE::set_primary_columns_template(QString templa)
{
    primary_table_column_template = templa;
}
void CATALOGUE::set_csv_trunk(wstring trunk)
{
    csv_trunk = trunk;
    size_t pos2 = trunk.rfind(L'\\');
    size_t pos1 = trunk.rfind(L'\\', pos2 - 1);
    wstring temp = trunk.substr(pos1 + 1, pos2 - pos1 - 1);
    qname = QString::fromStdWString(temp);
}
void CATALOGUE::set_csv_branches(vector<wstring> branches)
{
    csv_branches = branches;
}
void CATALOGUE::set_multicol(bool mc)
{
    multi_column = mc;
}
void CATALOGUE::set_qfile(int csv_index)
{
    wstring csv_path = get_csv_path(csv_index);
    qfile = q_memory(csv_path);
}
void CATALOGUE::set_column_titles(QVector<QString> col)
{
    column_titles = col;
}
void CATALOGUE::set_row_titles(QVector<QString> rt)
{
    row_titles = rt;
}
void CATALOGUE::set_description(QString desc)
{
    qdescription = desc;
}
void CATALOGUE::cancel_insertion()
{
    remote_control = 1;
}
void CATALOGUE::set_gid_want_list(QVector<QString>& gwl)
{
    gid_want_list = gwl;
}
void CATALOGUE::add_statements(QVector<QString>& raw_stmts, int myid)
{
    thr_stmts[myid].append(raw_stmts);
}

// Fetch functions.
wstring CATALOGUE::get_csv_path(int csv_index)
{
    wstring csv_path = csv_trunk + csv_branches[csv_index];
    return csv_path;
}
QString CATALOGUE::get_csv_branch(int csv_index)
{
    wstring wbranch = csv_branches[csv_index];
    QString qbranch = QString::fromStdWString(wbranch);
    return qbranch;
}
wstring CATALOGUE::get_csv_trunk()
{
    return csv_trunk;
}
QString CATALOGUE::get_year()
{
    return qyear;
}
vector<wstring> CATALOGUE::get_csv_branches()
{
    return csv_branches;
}
QString CATALOGUE::get_gid(int csv_index)
{
    return gid_list[csv_index];
}
QString CATALOGUE::get_qname()
{
    return qname;
}
QVector<QVector<QVector<int>>> CATALOGUE::get_tree()
{
    return tree;
}
QVector<QString> CATALOGUE::get_gid_list()
{
    return gid_list;
}
QString CATALOGUE::get_create_sub_template()
{
    QString sub_template;
    model.create_sub_template(sub_template);
    return sub_template;
}
QString CATALOGUE::get_insert_csv_row_template()
{
    return ins_csv_row_template;
}
int CATALOGUE::get_gid_size()
{
    return gid_list.size();
}
bool CATALOGUE::get_multicol()
{
    multi_column = model.get_multi_column();
    return multi_column;
}
QVector<QString> CATALOGUE::get_column_titles()
{
    QVector<QString> col = model.get_column_titles();
    return col;
}
QVector<QString> CATALOGUE::get_row_titles()
#include "catalogue.h"

using namespace std;

// Return a subtable name, given GID and genealogy.
string CATALOGUE::sublabelmaker(string& gid, QVector<QVector<int>>& family)
{
    string tname = "[" + sname + "$" + gid;
    int ancestry = family[0].size();
    int cheddar = 2;
    for (int ii = 0; ii < ancestry; ii++)
    {
        for (int jj = 0; jj < cheddar; jj++)
        {
            tname += "$";
        }
        cheddar++;
        tname += to_string(family[0][ii]);
    }
    tname += "]";
    return tname;
}

// Load on-disk folder paths for the catalogue folder.
void CATALOGUE::set_path(QString& cata_path)
{
    qpath = cata_path;
    wpath = cata_path.toStdWString();
}
void CATALOGUE::set_wpath(wstring& cata_wpath)
{
    wpath = cata_wpath;
    qpath = QString::fromStdWString(cata_wpath);
}

// Load the catalogue's significant variable names into the object.
void CATALOGUE::initialize_table()
{
    int pos1, pos2;
    size_t wpos1, wpos2;
    wstring wtemp;
    QString temp1;

    wstring folder_search = wpath + L"\\*.csv";
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr_bt("FindFirstFile-cata.initialize_table"); }
    wstring sample_name = wpath + L"\\" + info.cFileName;

    qfile = q_memory(sample_name);
    pos1 = qfile.lastIndexOf("Catalogue Number ", qfile.size() - 4);
    pos1 += 17;
    pos2 = qfile.indexOf('.', pos1);
    qname = qfile.mid(pos1, pos2 - pos1);
    wname = qname.toStdWString();
    tname = "T" + qname;
    sname = utf16to8(wname);

    wpos1 = sample_name.find(L'(');
    csv_trunk = sample_name.substr(0, wpos1);

    wpos1 = csv_trunk.find(L'\\');
    wpos2 = csv_trunk.find(L'\\', wpos1 + 1);
    wtemp = csv_trunk.substr(wpos1 + 1, wpos2 - wpos1 - 1);
    qyear = QString::fromStdWString(wtemp);
    syear = utf16to8(wtemp);

    pos1 = qfile.indexOf("\""); pos1++;
    pos2 = qfile.indexOf("\"", pos1);
    temp1 = qfile.mid(pos1, pos2 - pos1);
    pos1 = 0;
    do
    {
        pos1++;
        pos1 = temp1.indexOf('(', pos1);
        if (pos1 >= 0)
        {
            pos2 = temp1.indexOf(')', pos1);
            temp1.remove(pos1, pos2 - pos1 + 1);
        }
    } while (pos1 >= 0);
    qdescription = temp1;

    model.scan(qfile, qname);  // This will populate the embedded CSV object.
    make_name_tree();
    tree = model.get_model_tree();
    model_text_variables = model.get_text_variables();
    multi_column = model.get_multi_column();
    column_titles = model.get_column_titles();
    row_titles = model.get_row_titles();

    if (!FindClose(hfile1)) { winerr_bt("FindClose-cata.initialize_table"); }
}
void CATALOGUE::initialize_threading(int num)
{
    jobs.resize(num);
    thr_stmts.resize(num, QVector<QVector<QString>>());
    bot_top.resize(num, vector<int>(2));
}

// Populates the 'csv_branches' and 'gid_list' vectors.
void CATALOGUE::make_name_tree()
{
    size_t pos0 = csv_trunk.size();
    csv_branches = get_file_path_endings(wpath, pos0);
    pos0 = csv_branches.size();
    gid_list.resize(pos0);
    size_t pos1, pos2;
    wstring wtemp;
    for (size_t ii = 0; ii < pos0; ii++)
    {
        pos1 = csv_branches[ii].find(L'(');
        pos2 = csv_branches[ii].find(L')', pos1 + 1);
        wtemp = csv_branches[ii].substr(pos1 + 1, pos2 - pos1 - 1);
        gid_list[ii] = QString::fromStdWString(wtemp);
    }
}

// Worker thread functions.
void CATALOGUE::basic_input(QVector<QString> input)
{
    qyear = input[0];
    qname = input[1];
    qdescription = input[2];
}
void CATALOGUE::set_tree(QVector<QVector<QVector<int>>> sapling)
{
    tree = sapling;
}
void CATALOGUE::set_gid_list(QVector<QString> list)
{
    gid_list = list;
}
void CATALOGUE::set_primary_columns_template(QString templa)
{
    primary_table_column_template = templa;
}
void CATALOGUE::set_csv_trunk(wstring trunk)
{
    csv_trunk = trunk;
    size_t pos2 = trunk.rfind(L'\\');
    size_t pos1 = trunk.rfind(L'\\', pos2 - 1);
    wstring temp = trunk.substr(pos1 + 1, pos2 - pos1 - 1);
    qname = QString::fromStdWString(temp);
}
void CATALOGUE::set_csv_branches(vector<wstring> branches)
{
    csv_branches = branches;
}
void CATALOGUE::set_multicol(bool mc)
{
    multi_column = mc;
}
void CATALOGUE::set_qfile(int csv_index)
{
    wstring csv_path = get_csv_path(csv_index);
    qfile = q_memory(csv_path);
}
void CATALOGUE::set_column_titles(QVector<QString> col)
{
    column_titles = col;
}
void CATALOGUE::set_row_titles(QVector<QString> rt)
{
    row_titles = rt;
}
void CATALOGUE::set_description(QString desc)
{
    qdescription = desc;
}
void CATALOGUE::cancel_insertion()
{
    remote_control = 1;
}
void CATALOGUE::set_gid_want_list(QVector<QString>& gwl)
{
    gid_want_list = gwl;
}
void CATALOGUE::add_statements(QVector<QString>& raw_stmts, int myid)
{
    thr_stmts[myid].append(raw_stmts);
}

// Fetch functions.
wstring CATALOGUE::get_csv_path(int csv_index)
{
    wstring csv_path = csv_trunk + csv_branches[csv_index];
    return csv_path;
}
QString CATALOGUE::get_csv_branch(int csv_index)
{
    wstring wbranch = csv_branches[csv_index];
    QString qbranch = QString::fromStdWString(wbranch);
    return qbranch;
}
wstring CATALOGUE::get_csv_trunk()
{
    return csv_trunk;
}
QString CATALOGUE::get_qyear()
{
    return qyear;
}
string CATALOGUE::get_syear()
{
    return syear;
}
vector<wstring> CATALOGUE::get_csv_branches()
{
    return csv_branches;
}
string CATALOGUE::get_gid(int csv_index)
{
    string gid = gid_list[csv_index].toStdString();
    return gid;
}
QString CATALOGUE::get_qname()
{
    return qname;
}
string CATALOGUE::get_sname()
{
    return sname;
}
QVector<QVector<QVector<int>>> CATALOGUE::get_tree()
{
    return tree;
}
QVector<QString> CATALOGUE::get_gid_list()
{
    return gid_list;
}
string CATALOGUE::get_create_sub_template()
{
    QString sub_template;
    model.create_sub_template(sub_template);
    string templa = sub_template.toStdString();
    return templa;
}
string CATALOGUE::get_insert_csv_row_template()
{
    string templa = ins_csv_row_template.toStdString();
    return templa;
}
int CATALOGUE::get_gid_size()
{
    return gid_list.size();
}
bool CATALOGUE::get_multicol()
{
    multi_column = model.get_multi_column();
    return multi_column;
}
QVector<QString> CATALOGUE::get_column_titles()
{
    QVector<QString> col = model.get_column_titles();
    return col;
}
QVector<QString> CATALOGUE::get_row_titles()
{
    row_titles = model.get_row_titles();
    return row_titles;
}
QVector<QVector<QString>> CATALOGUE::get_model_text_variables()
{
    QVector<QVector<QString>> tv = model.get_text_variables();
    return tv;
}
QString CATALOGUE::get_description()
{
    return qdescription;
}
string CATALOGUE::get_csv_template()
{
    string templa = csv_tables_template.toStdString();
    return templa;
}
string CATALOGUE::get_primary_template()
{
    string templa = primary_table_column_template.toStdString();
    return templa;
}
int CATALOGUE::get_status()
{
    return remote_control;
}
QVector<QString> CATALOGUE::get_gid_want_list()
{
    return gid_want_list;
}
QVector<QVector<QString>> CATALOGUE::take_statements(int myid)
{
    QVector<QVector<QString>> stmts = thr_stmts[myid];
    thr_stmts[myid].clear();
    return stmts;
}

// Update or retrieve this catalogue's list of CSVs containing 'missing data' placeholders.
/*
void CATALOGUE::set_damaged_gid(QString& gid)
{
    damaged_gid_list.append(gid);
}
QVector<QString> CATALOGUE::get_damaged_gid_list()
{
    return damaged_gid_list;
}
*/

// Template fabrication functions.
void CATALOGUE::insert_primary_columns_template()
{
    int col_count = primary_table_column_titles.size();
    QString primary_template = "INSERT INTO [" + qname + "] (GID, ";
    for (int ii = 1; ii < primary_table_column_titles.size(); ii++)
    {
        primary_template += "[";
        primary_template += primary_table_column_titles[ii];
        primary_template += "], ";
    }
    primary_template.remove(primary_template.size() - 2, 2);
    primary_template += ") VALUES (";
    for (int ii = 0; ii < col_count; ii++)
    {
        primary_template += "?, ";
    }
    primary_template.remove(primary_template.size() - 2, 2);
    primary_template += ");";
    primary_table_column_template = primary_template;
}
void CATALOGUE::create_csv_tables_template()
{
    QString sql = "CREATE TABLE IF NOT EXISTS !!! (";
    if (multi_column)
    {
        sql += "[";
        sql += column_titles[0];
        sql += "] TEXT, ";
        for (int ii = 1; ii < column_titles.size(); ii++)
        {
            sql += "[";
            sql += column_titles[ii];
            sql += "] NUMERIC, ";
        }
    }
    else
    {
        sql += "[Description] TEXT, ";
        sql += "[Value] NUMERIC, ";
    }
    sql.remove(sql.size() - 2, 2);
    sql.append(");");
    csv_tables_template = sql;
}
void CATALOGUE::insert_csv_row_template()
{
    QString sql = "INSERT INTO !!! (";
    for (int ii = 0; ii < column_titles.size(); ii++)
    {
        sql += "[";
        sql += column_titles[ii];
        sql += "], ";
    }
    sql.remove(sql.size() - 2, 2);
    sql.append(") VALUES (");
    for (int ii = 0; ii < column_titles.size(); ii++)
    {
        sql += "?, ";
    }
    sql.remove(sql.size() - 2, 2);
    sql.append(");");
    ins_csv_row_template = sql;
}

// Return a complete (non-template) SQL statement to create this catalogue's primary/column table.
string CATALOGUE::create_primary_table()
{
    QVector<QString> linearized_titles;
    QVector<QString> family_line;
    QString base, title;
    int indent;
    int old_indent = 0;

    // Don't forget the preliminaries...
    linearized_titles.append("GID");
    for (int ii = 0; ii < model_text_variables.size(); ii++)
    {
        linearized_titles.append(model_text_variables[ii][0]);
    }

    // Linearize the CSV spreadsheet, and save the title list for later use.
    if (multi_column)
    {
        for (int ii = 0; ii < row_titles.size(); ii++)
        {
            base.clear();
            indent = 0;
            while (row_titles[ii][indent] == '+')
            {
                indent++;
            }
            if (indent > old_indent)
            {
                family_line.append(row_titles[ii - 1]);
                old_indent = indent;
            }
            else if (indent < old_indent)
            {
                for (int jj = 0; jj < old_indent - indent; jj++)
                {
                    family_line.removeLast();
                }
                old_indent = indent;
            }
            for (int jj = 0; jj < family_line.size(); jj++)
            {
                base.append(family_line[jj]);
                base += " ";
            }
            base.append(row_titles[ii]);

            for (int jj = 1; jj < column_titles.size(); jj++)
            {
                // Note: the first column title is dropped, as it is not useful.
                title = base + " @ ";  // The 'at' symbol separates rows from columns.
                title += column_titles[jj];
                linearized_titles.append(title);
            }
        }
    }
    else
    {
        for (int ii = 0; ii < row_titles.size(); ii++)
        {
            base.clear();
            indent = 0;
            while (row_titles[ii][indent] == '+')
            {
                indent++;
            }
            if (indent > old_indent)
            {
                family_line.append(row_titles[ii - 1]);
                old_indent = indent;
            }
            else if (indent < old_indent)
            {
                for (int jj = 0; jj < old_indent - indent; jj++)
                {
                    family_line.removeLast();
                }
                old_indent = indent;
            }
            for (int jj = 0; jj < family_line.size(); jj++)
            {
                base.append(family_line[jj]);
                base += " ";
            }
            base.append(row_titles[ii]);
            linearized_titles.append(base);
        }
    }
    primary_table_column_titles = linearized_titles;

    // Build the statement to create the catalogue's primary table in the database.
    int deviants = 1;  // GID
    string temp;
    string stmt = "CREATE TABLE IF NOT EXISTS [" + sname + "] (";
    stmt += "GID INTEGER PRIMARY KEY, ";
    for (int ii = 0; ii < model_text_variables.size(); ii++)
    {
        temp = model_text_variables[ii][0].toStdString();
        stmt += "[" + temp + "] TEXT, ";
        deviants++;
    }
    for (int ii = deviants; ii < linearized_titles.size(); ii++)
    {
        temp = linearized_titles[ii].toStdString();
        stmt += "[" + temp + "] NUMERIC, ";
    }
    stmt.erase(stmt.size() - 2, 2);
    stmt.append(");");
    return stmt;
}
string CATALOGUE::create_column_table()
{
    vector<string> column_titles;
    QString qtemp;

    // First, get the text variable titles.
    for (int ii = 0; ii < model_text_variables.size(); ii++)
    {
        qtemp = model_text_variables[ii][0];
        column_titles.push_back(qtemp.toStdString());
    }

    // Second, add the row titles.
    for (int ii = 0; ii < row_titles.size(); ii++)
    {
        qtemp = row_titles[ii];
        column_titles.push_back(qtemp.toStdString());
    }

    // Third, make the statement.
    string tname = "[" + sname + "$Columns]";
    string stmt = "!!!CREATE TABLE IF NOT EXISTS " + tname + " (Column, TEXT);";
    for (int ii = 0; ii < column_titles.size(); ii++)
    {
        sclean(column_titles[ii], 1);
        stmt += "!!!INSERT INTO " + tname + " (Column) VALUES ('";
        stmt += column_titles[ii];
        stmt += "');";
    }
    stmt += "!!!";
    return stmt;
}

// Returns a CSV's text variable rows.
QVector<QVector<QString>> CATALOGUE::extract_text_vars(QString& qfile)
{
    QVector<QVector<QString>> text_variables;
    QString temp1;
    int pos1, pos2;
    QChar math;
    pos1 = 0;
    while (1)
    {
        pos1 = qfile.indexOf('=', pos1 + 1);
        if (pos1 < 0) { break; }  // Primary loop exit.
        math = qfile[pos1 - 1];
        if (math == '<' || math == '>') { continue; }

        text_variables.push_back(QVector<QString>());
        pos2 = qfile.lastIndexOf('"', pos1); pos2++;
        temp1 = qfile.mid(pos2, pos1 - pos2);
        qclean(temp1, 0);
        text_variables[text_variables.size() - 1].push_back(temp1);

        pos2 = qfile.indexOf('"', pos1);
        temp1 = qfile.mid(pos1 + 1, pos2 - pos1 - 1);
        qclean(temp1, 1);
        text_variables[text_variables.size() - 1].push_back(temp1);
    }
    return text_variables;
}
vector<vector<string>> CATALOGUE::extract_text_vars8(string& sfile)
{
    vector<vector<string>> text_variables;
    string temp1;
    int pos1, pos2;
    char math;
    pos1 = 0;
    while (1)
    {
        pos1 = sfile.find('=', pos1 + 1);
        if (pos1 > sfile.size()) { break; }  // Primary loop exit.
        math = sfile[pos1 - 1];
        if (math == '<' || math == '>') { continue; }

        text_variables.push_back(vector<string>());
        pos2 = sfile.rfind('"', pos1); 
        pos2++;
        temp1 = sfile.substr(pos2, pos1 - pos2);
        sclean(temp1, 0);
        text_variables[text_variables.size() - 1].push_back(temp1);

        pos2 = sfile.find('"', pos1);
        temp1 = sfile.substr(pos1 + 1, pos2 - pos1 - 1);
        sclean(temp1, 1);
        text_variables[text_variables.size() - 1].push_back(temp1);
    }
    return text_variables;
}


// Returns a CSV's data rows.
QVector<QVector<QString>> CATALOGUE::extract_data_rows(QString& qfile, int& damaged)
{
    QVector<QVector<QString>> rows;
    int pos1, pos2, pos3, nl1, nl2;
    QChar math;
    pos1 = qfile.size() - 1;
    do
    {
        pos1 = qfile.lastIndexOf('=', pos1 - 1);
        math = qfile[pos1 - 1];
        if (math == '<' || math == '>')
        {
            pos2 = 1;
        }
        else
        {
            pos2 = 0;
        }
    } while (pos2);

    nl1 = qfile.indexOf('\n', pos1);
    if (multi_column)
    {
        nl1 = qfile.indexOf('\n', nl1 + 1);
    }
    nl2 = qfile.indexOf('\n', nl1 + 1);

    int row_index = -1;
    QString temp1;
    while (nl2 > 0)
    {
        pos1 = qfile.indexOf('"', nl1);
        pos2 = qfile.lastIndexOf('"', nl2);
        temp1 = qfile.mid(pos1 + 1, pos2 - pos1 - 1);
        qclean(temp1, 0);
        if (temp1 == "Note") { break; }

        rows.append(QVector<QString>());
        row_index++;
        rows[row_index].append(row_titles[row_index]);

        pos1 = qfile.lastIndexOf('"', nl2);
        pos2 = qfile.indexOf(',', pos1);
        do
        {
            pos1 = pos2;
            pos2 = qfile.indexOf(',', pos1 + 1);
            if (pos2 > nl2)  // If we have reached the last value on this line...
            {
                pos3 = qfile.indexOf(' ', pos1 + 1);  // ... check for a space before newline.
                if (pos3 > nl2)
                {
                    pos3 = qfile.indexOf('\r', pos1 + 1);  // ... confirm end of line.
                    if (pos3 > nl2)
                    {
                        err_bt("pos error in extract_classic_rows");
                    }
                }
                temp1 = qfile.mid(pos1 + 1, pos3 - pos1 - 1);               
            }
            else
            {
                temp1 = qfile.mid(pos1 + 1, pos2 - pos1 - 1);
            }

            if (temp1 == "..")  // Stats Canada uses '..' as a placeholder for 'no data available here'.
            {
                damaged++;
            }
            rows[row_index].append(temp1);
        } while (pos2 < nl2);  // All strings after the first are values, in string form.

        nl1 = nl2;
        nl2 = qfile.indexOf('\n', nl1 + 1);
    }
    return rows;
}
vector<vector<string>> CATALOGUE::extract_data_rows8(string& sfile, int& damaged)
{
    vector<vector<string>> rows;
    size_t pos1, pos2, pos3, nl1, nl2;
    char math;
    QString qtemp;
    pos1 = sfile.size() - 1;
    do
    {
        pos1 = sfile.rfind('=', pos1 - 1);
        math = sfile[pos1 - 1];
        if (math == '<' || math == '>')
        {
            pos2 = 1;
        }
        else
        {
            pos2 = 0;
        }
    } while (pos2);

    nl1 = sfile.find('\n', pos1);
    if (multi_column)
    {
        nl1 = sfile.find('\n', nl1 + 1);
    }
    nl2 = sfile.find('\n', nl1 + 1);

    int row_index = -1;
    string temp1;
    while (nl2 < sfile.size())
    {
        pos1 = sfile.find('"', nl1);
        pos2 = sfile.rfind('"', nl2);
        temp1 = sfile.substr(pos1 + 1, pos2 - pos1 - 1);
        sclean(temp1, 0);
        if (temp1 == "Note") { break; }

        rows.push_back(vector<string>());
        row_index++;
        qtemp = row_titles[row_index];
        rows[row_index].push_back(qtemp.toStdString());

        pos1 = sfile.rfind('"', nl2);
        pos2 = sfile.find(',', pos1);
        do
        {
            pos1 = pos2;
            pos2 = sfile.find(',', pos1 + 1);
            if (pos2 > nl2)  // If we have reached the last value on this line...
            {
                pos3 = sfile.find(' ', pos1 + 1);  // ... check for a space before newline.
                if (pos3 > nl2)
                {
                    pos3 = sfile.find('\r', pos1 + 1);  // ... confirm end of line.
                    if (pos3 > nl2)
                    {
                        err_bt("pos error in extract_classic_rows");
                    }
                }
                temp1 = sfile.substr(pos1 + 1, pos3 - pos1 - 1);
            }
            else
            {
                temp1 = sfile.substr(pos1 + 1, pos2 - pos1 - 1);
            }

            if (temp1 == "..")  // Stats Canada uses '..' as a placeholder for 'no data available here'.
            {
                damaged++;
            }
            rows[row_index].push_back(temp1);
        } while (pos2 < nl2);  // All strings after the first are values, in string form.

        nl1 = nl2;
        nl2 = sfile.find('\n', nl1 + 1);
    }
    return rows;
}

void CATALOGUE::print_stuff()
{
    QString path1 = "F:\\primary_table_column_template.txt";
    QString path2 = "F:\\csv_tables_template.txt";
    QString path3 = "F:\\ins_csv_row_template.txt";

}

