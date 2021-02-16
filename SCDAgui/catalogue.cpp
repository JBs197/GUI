#include "catalogue.h"

using namespace std;

// Return a subtable name, given GID and genealogy.
QString CATALOGUE::sublabelmaker(QString& gid, QVector<QVector<int>>& family)
{
    QString stname = "T" + qname + "$" + gid;
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
    return stname;
}

// Load on-disk folder paths for the catalogue folder.
void CATALOGUE::set_path(QString& cata_path)
{
    qpath = cata_path;
    wpath = cata_path.toStdWString();
}

// Load the catalogue's significant variable names into the object.
void CATALOGUE::initialize_table()
{
    int pos1, pos2;
    size_t wpos1;
    QString temp1;

    wstring folder_search = wpath + L"\\*.csv";
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr(L"FindFirstFile-cata.initialize_table"); }
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

    if (!FindClose(hfile1)) { warn(L"FindClose-cata.initialize_table"); }
    log8(sname + " initialized.\r\n");
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
QString CATALOGUE::get_insert_row_template()
{
    QString row_template;
    model.insert_row_template(row_template);
    return row_template;
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

// Template fabrication functions.
QString CATALOGUE::get_primary_columns_template()
{
    int col_count = primary_table_column_titles.size();
    QString primary_template = "INSERT INTO \"" + tname + "\" ( ";
    for (int ii = 0; ii < col_count; ii++)
    {
        primary_template += "\"";
        primary_template += primary_table_column_titles[ii];
        primary_template += "\", ";
    }
    primary_template.remove(primary_template.size() - 2, 2);
    primary_template += " ) VALUES ( ";
    for (int ii = 0; ii < col_count; ii++)
    {
        primary_template += "?, ";
    }
    primary_template.remove(primary_template.size() - 2, 2);
    primary_template += " )";
    return primary_template;
}

// Returns <statement, tname> for the primary catalogue table (first index) and the CSV tables.
QVector<QVector<QString>> CATALOGUE::get_nobles()
{
    QVector<QVector<QString>> nobles;
    primary_table_column_titles = model.create_table_cata(nobles);
    model.create_table_csvs(nobles, gid_list);
    return nobles;
}

// Returns a CSV's data rows.
QVector<QVector<QString>> CATALOGUE::extract_data_rows()
{
    QVector<QVector<QString>> rows(row_titles.size(), QVector<QString>());
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
                        err8("pos error in extract_classic_rows");
                    }
                }
                temp1 = qfile.mid(pos1 + 1, pos3 - pos1 - 1);
                rows[row_index].append(temp1);
            }
            else
            {
                temp1 = qfile.mid(pos1 + 1, pos2 - pos1 - 1);
                rows[row_index].append(temp1);
            }
        } while (pos2 < nl2);  // All strings after the first are values, in string form.

        nl1 = nl2;
        nl2 = qfile.indexOf('\n', nl1 + 1);
    }
    return rows;
}
