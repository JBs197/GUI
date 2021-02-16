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
    HANDLE hfile2 = CreateFileW(sample_name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile2 == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-cata.initialize_table"); }

    qfile = q_memory(hfile2);
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
    model_tree = model.get_model_tree();

    if (!FindClose(hfile1)) { warn(L"FindClose-cata.initialize_table"); }
    if (!CloseHandle(hfile2)) { warn(L"CloseHandle-cata.initialize_table"); }
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
    return model_tree;
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
QString CATALOGUE::get_create_row_template()
{
    QString row_template;
    model.create_row_template(row_template);
    return row_template;
}

// Returns <statement, tname> for the primary catalogue table (first index) and the CSV tables.
QVector<QVector<QString>> CATALOGUE::get_nobles()
{
    QVector<QVector<QString>> nobles;
    model.create_table_cata(nobles);
    model.create_table_csvs(nobles, gid_list);
    return nobles;
}


