#include "catalogue.h"

using namespace std;

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
    model.tree_walker();
    model_subtable_names = model.get_subtable_names();
    make_name_tree();

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

// Load the unique portion of each CSV file name into a vector.
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

// Return a list of SQL statements to create the main table and all nested subtables for this catalogue.
QVector<QString> CATALOGUE::get_create_table_statements(int cores)
{
    QElapsedTimer timer;
    QVector<QString> work;
    model.create_table_cata(work);
    model.create_table_csvs(work, gid_list);
    timer.start();
    //model.create_table_subs(work, gid_list);
    qDebug() << "Create Table, subs: " << timer.restart();
    QString tc;
    qlog("Created a SQL statement list to insert " + tc.setNum(work.size()) + " tables from catalogue " + qname + ".");
    return work;
}

void CATALOGUE::create_table_taskmaster(QVector<QString>& work, QVector<QString>& gid_list, int cores)
{
    int workload = gid_list.size() / cores;
    //int remainder = gid_list.size() % cores;
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
