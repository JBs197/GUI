#include "csv.h"

using namespace std;

// For a given CSV string, extract all data desired by the model.
void CSV::scan(QString& qf, QString& qn)
{
    qfile = qf;
    qname = qn;
    unique_row_buffer.append("");
    int pos0 = extract_variables();
    int pos1 = extract_column_titles(pos0);
    extract_model_rows(pos1);
    tree_walker();
}
void CSV::quick_scan(QString& qf, wstring& trunk, wstring& branch)
{
    qfile = qf;
    size_t wpos1, wpos2;
    wpos2 = trunk.rfind(L'\\');
    wpos1 = trunk.rfind(L'\\', wpos2 - 1);
    wstring temp = trunk.substr(wpos1 + 1, wpos2 - wpos1 - 1);
    qname = QString::fromStdWString(temp);
    wpos1 = branch.find(L'(');
    wpos2 = branch.find(L')', wpos1 + 1);
    temp = branch.substr(wpos1 + 1, wpos2 - wpos1 - 1);
    gid = QString::fromStdWString(temp);
    int pos0 = extract_variables();
    int pos1 = extract_column_titles(pos0);
    extract_model_rows(pos1);
}

void CSV::set_gid(QString& gd)
{
    gid = gd;
}

// Find all possible subtrees for this CSV. Children have uniform indentation.
// Form [possibility index][index of progenitor, then descendents][list of children]
void CSV::tree_walker()
{
    int indentation = 0;
    int start_index = 0;
    int indent_index;
    QVector<int> genealogy;
    QVector<QVector<int>> pidgeonhole;  // Form [indentation][row_index]

    // Firstly, categorize each data row by its indentation, while keeping order.
    for (int ii = 0; ii < model_rows.size(); ii++)
    {
        indent_index = 0;
        while (model_rows[ii][0][indent_index] == '+') // Determine this data row's indentation.
        {
            indent_index++;
        }
        while (pidgeonhole.size() <= indent_index)  // If space is lacking in pidgeonhole, make more space.
        {
            pidgeonhole.append(QVector<int>());
        }
        pidgeonhole[indent_index].append(ii);
    }

    // Secondly, create lists of all parents and their direct children.
    QVector<QVector<QVector<int>>> subtables;  // Form [indentation][subtable_index][subtable_rows]
    subtables.resize(pidgeonhole.size() - 1);
    QVector<int> temp;
    int bot, top, child;
    int save_point = 0;
    for (int ii = 0; ii < subtables.size(); ii++)
    {
        save_point = 0;
        for (int jj = 0; jj < pidgeonhole[ii].size(); jj++)
        {
            if (jj == pidgeonhole[ii].size() - 1)  // Define min/max boundaries inside of which
            {                                      // we will look for children.
                top = model_rows.size() - 1;
                bot = pidgeonhole[ii][jj];
            }
            else
            {
                top = pidgeonhole[ii][jj + 1];
                bot = pidgeonhole[ii][jj];
            }

            temp.clear();
            temp.append(bot);  // The first integer of the vector is the parent's row index.
            for (int kk = save_point; kk < pidgeonhole[ii + 1].size(); kk++)
            {
                child = pidgeonhole[ii + 1][kk];
                if (child > bot && child <= top)  // Every child's parent is the largest row index
                {                                 // from the previous indentation's list, without
                    temp.append(child);           // exceeding the child's row index.
                }
                else if (child > top)
                {
                    save_point = kk;  // This is the first child index that was too large.
                    break;            // The next parental candidate can start here. Go go efficiency.
                }
            }
            if (temp.size() > 1)  // A row is added to the list of parents if it has at least one child.
            {
                subtables[ii].append(temp);
            }
        }
    }

    // Thirdly, create lists of all possible family trees, from first to last generation. Save to the model's tree.
    for (int ii = 0; ii < subtables[0].size(); ii++)  // For every parentless parent, begin a new family line.
    {
        genealogy = { subtables[0][ii][0] };                                 // Travel through the tree, and record
        start_index = is_parent(subtables, genealogy, indentation, start_index);  // the genealogy of every parent.
    }
}

// Return a subtable name, given GID and genealogy.
QString CSV::sublabelmaker(QString& gid, QVector<QVector<int>>& family)
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

// Fetch value functions.
bool CSV::get_multi_column()
{
    return multi_column;
}
QVector<QString> CSV::get_column_titles()
{
    return column_titles;
}
QVector<QVector<QString>> CSV::get_text_variables()
{
    return text_variables;
}
QVector<QVector<QVector<int>>> CSV::get_model_tree()
{
    return tree;
}
QVector<QString> CSV::get_row_titles()
{
    int num_rows = model_rows.size();
    QVector<QString> titles(num_rows);
    for (int ii = 0; ii < num_rows; ii++)
    {
        titles[ii] = model_rows[ii][0];
    }
    return titles;
}

// Recursively determine if the given row is a parent. If so, add it to the tree and do the same for all
// its children. Returns first yet-to-be checked index in the current indentation's subtable list.
int CSV::is_parent(QVector<QVector<QVector<int>>>& subtables, QVector<int> genealogy, int indentation, int start_index)
{
    int row = genealogy[genealogy.size() - 1];  // Current candidate for parenthood.
    QVector<int> new_genealogy;
    int new_start_index = 0;
    int num_children, current_pos;
    for (int ii = start_index; ii < subtables[indentation].size(); ii++)  // For all parents at this indentation...
    {
        if (row == subtables[indentation][ii][0])  // ...if the candidate is a parent...
        {
            tree.append(QVector<QVector<int>>(2));  // ... give the candidate its own possibility branch in the tree.
            current_pos = tree.size() - 1;
            tree[current_pos][0] = genealogy;
            tree[current_pos][1] = subtables[indentation][ii];
            tree[current_pos][1].removeFirst();

            if (indentation < subtables.size() - 1)  // ... and if not currently examining the final generation...
            {
                num_children = tree[current_pos][1].size();
                for (int jj = 0; jj < num_children; jj++)  // ... then repeat the process with the candidate's children.
                {
                    new_genealogy = genealogy;
                    new_genealogy.append(tree[current_pos][1][jj]);
                    new_start_index = is_parent(subtables, new_genealogy, indentation + 1, new_start_index);  // ...
                }
            }

            return ii + 1;
        }
        else if (row < subtables[indentation][ii][0])
        {
            return ii;
        }
    }
    return subtables[indentation].size();
}

// Populate the CSV object with the CSV file's text variables.
int CSV::extract_variables()
{
    int pos1, pos2;
    QChar math;
    QString temp1, temp2;
    pos1 = 0;
    pos2 = 0;

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
        temp2 = "\'" + temp1 + "\'";
        text_variables[text_variables.size() - 1].push_back(temp2);
    }
    return pos2;  // Position of the final quotation mark on the final "text variable" row.
}

// Populate the CSV object with the CSV file's column titles.
int CSV::extract_column_titles(int pos0)
{
    int pos1, pos2, nl1, nl2;
    QString temp1;

    column_titles.clear();
    nl1 = qfile.indexOf('\n', pos0);
    nl2 = qfile.indexOf('\n', nl1 + 1);
    pos1 = qfile.indexOf('"', nl1);
    do
    {
        pos2 = qfile.indexOf('"', pos1 + 1);
        if (pos2 < nl2)
        {
            temp1 = qfile.mid(pos1 + 1, pos2 - pos1 - 1);
            qclean(temp1, 0);
            column_titles.append(temp1);
            pos1 = qfile.indexOf('"', pos2 + 1);
        }
    } while (pos1 < nl2);

    if (column_titles.size() < 2)
    {
        multi_column = 0;
        return nl1;  // This was a data line - redo it later.
    }

    multi_column = 1;
    return nl2;
}

// Populate the CSV object with the model CSV file's row titles and row values.
// Also, populate the 'is_int' vector for this catalogue, so that 'is_int' and 'model_rows' share indices.
void CSV::extract_model_rows(int pos0)
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
        pos1 = qfile.indexOf('"', nl1);
        pos2 = qfile.indexOf('"', pos1 + 1);
        temp1 = qfile.mid(pos1 + 1, pos2 - pos1 - 1);
        spaces = qclean(temp1, 0);
        if (temp1 == "Note") { break; }  // Primary exit from the loop.
        model_rows.append(QVector<QString>());
        row_index = model_rows.size() - 1;
        indentation = index_card(space_index, spaces);
        if (indentation < 0) { err(L"index_card-csv.extract_model_rows"); }

        temp2.clear();
        for (int ii = 0; ii < indentation; ii++)
        {
            temp2.push_back('+');
        }
        temp2 += temp1;
        model_rows[row_index].append(temp2);  // First string is the row title.

        pos2 = qfile.indexOf(',', pos2);
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
                model_rows[row_index].append(temp1);
            }
            else
            {
                temp1 = qfile.mid(pos1 + 1, pos2 - pos1 - 1);
                model_rows[row_index].append(temp1);
            }
        } while (pos2 < nl2);  // All strings after the first are values, in string form.

        model_is_int.append(qnum_test(temp1));  // Index is identical to 'model_rows'.

        nl1 = nl2;
        nl2 = qfile.indexOf('\n', nl1 + 1);
    } while (nl2 > 0);  // Secondary exit from the loop.
}

// For a given row (with values), return a unique row name by front-pushing the child's genealogy.
// Uses a persistent buffer in the CSV object. Remember to clear it when done!
QString CSV::unique_row_title(int row_index)
{
    int current_indent = unique_row_buffer.size() - 1;
    QString terminal = model_rows[row_index][0];
    int new_indent = 0;
    int pos1 = 0;
    while (terminal[pos1] == '+')
    {
        pos1++;
        new_indent++;
    }

    if (new_indent == current_indent)
    {
        unique_row_buffer[current_indent] = terminal;
    }
    else if (new_indent > current_indent)
    {
        unique_row_buffer.append(terminal);
    }
    else
    {
        pos1 = current_indent - new_indent;
        unique_row_buffer.remove(unique_row_buffer.size() - pos1, pos1);
        unique_row_buffer[new_indent] = terminal;
    }

    QString unique;
    for (int ii = 0; ii <= new_indent; ii++)
    {
        unique += unique_row_buffer[ii];
        if (ii < new_indent)
        {
            unique += " ";
            for (int jj = 0; jj < ii + 1; jj++)
            {
                unique += "$";
            }
            unique += " ";
        }
    }
    return unique;
}
QString CSV::unique_row_title_multicol(int row_index, int& highest_indent)
{
    int current_indent = unique_row_buffer.size() - 1;
    QString terminal = model_rows[row_index][0];
    int new_indent = 0;
    int pos1 = 0;
    int max_indent = 0;
    while (terminal[pos1] == '+')
    {
        pos1++;
        new_indent++;
    }

    if (new_indent == current_indent)
    {
        unique_row_buffer[current_indent] = terminal;
    }
    else if (new_indent > current_indent)
    {
        unique_row_buffer.append(terminal);
    }
    else
    {
        pos1 = current_indent - new_indent;
        unique_row_buffer.remove(unique_row_buffer.size() - pos1, pos1);
        unique_row_buffer[new_indent] = terminal;
    }

    QString unique;
    for (int ii = 0; ii <= new_indent; ii++)
    {
        unique += unique_row_buffer[ii];
        if (ii < new_indent)
        {
            unique += " ";
            for (int jj = 1; jj <= ii + 1; jj++)
            {
                unique += "$";
                if (jj > max_indent)
                {
                    max_indent = jj;
                }
            }
            unique += " ";
        }
    }
    highest_indent = max_indent;
    return unique;
}

// Build a SQL statement to create the primary table, which encompasses the whole catalogue.
// Returns a list of column titles for the primary (linearized) catalogue table.
QVector<QString> CSV::create_table_cata(QVector<QVector<QString>>& work)
{
    int base_indent;
    QString base, temp;
    QVector<QString> primary_columns = { "GID" };
    QString sql = "CREATE TABLE IF NOT EXISTS \"T" + qname + "\" ( GID INTEGER PRIMARY KEY, ";
    for (int ii = 0; ii < text_variables.size(); ii++)
    {
        sql += "\"";
        sql += text_variables[ii][0];
        sql += "\" TEXT, ";
        primary_columns.append(text_variables[ii][0]);
    }

    unique_row_buffer.clear();
    if (multi_column)  // Multi-column spreadsheets must be crushed into 1D vectors.
    {
        for (int ii = 0; ii < model_rows.size(); ii++)
        {
            base = unique_row_title_multicol(ii, base_indent);
            for (int jj = 1; jj < column_titles.size(); jj++)
            {
                temp = base;
                temp += " ";
                for (int kk = 0; kk <= base_indent; kk++)
                {
                    temp += "$";
                }
                temp += " ";
                temp += column_titles[jj];

                sql += "\"";
                sql += temp;
                sql += "\" NUMERIC, ";
                primary_columns.append(temp);
            }
        }
    }
    else
    {
        for (int ii = 0; ii < model_rows.size(); ii++)
        {
            temp = unique_row_title(ii);
            sql += "\"";
            sql += temp;
            sql += "\" NUMERIC, ";
            primary_columns.append(temp);
        }
    }

    sql.remove(sql.size() - 2, 2);
    sql.append(" );");
    work.append(QVector<QString>(2));
    work[work.size() - 1][0] = sql;
    work[work.size() - 1][1] = "T" + qname;
    return primary_columns;
}

// Build a list of SQL statements to create all the secondary (CSV) tables for this catalogue.
void CSV::create_table_csvs(QVector<QVector<QString>>& work, QVector<QString>& gid_list)
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
        work.append(QVector<QString>(2));
        work[work.size() - 1][0] = sql0 + sql1 + sql2;
        work[work.size() - 1][1] = "T" + qname + sql1;
    }
}

// Build a SQL statement to create a subtable, with the table name missing.
void CSV::create_sub_template(QString& work)
{
    work = "CREATE TABLE IF NOT EXISTS \"!!!\" ( ";
    for (int ii = 0; ii < text_variables.size(); ii++)
    {
        work += "\"";
        work += text_variables[ii][0];
        work += "\" TEXT, ";
    }
    if (multi_column)
    {
        work += "\"";
        work += column_titles[0];
        work += "\" TEXT, ";
        for (int ii = 1; ii < column_titles.size(); ii++)
        {
            work += "\"";
            work += column_titles[ii];
            work += "\" NUMERIC, ";
        }
    }
    else
    {
        work += "\"Description\" TEXT, ";
        work += "\"Value\" NUMERIC, ";
    }
    work.remove(work.size() - 2, 2);
    work.append(" );");
}

// Build a SQL statement to insert a row into the (non-primary) table, with the table name missing.
void CSV::insert_row_template(QString& work)
{
    int col_count = 0;
    work = "INSERT INTO \"!!!\" ( ";
    for (int ii = 0; ii < text_variables.size(); ii++)
    {
        work += "\"";
        work += text_variables[ii][0];
        work += "\", ";
        col_count++;
    }
    if (multi_column)
    {
        for (int ii = 0; ii < column_titles.size(); ii++)
        {
            work += "\"";
            work += column_titles[ii];
            work += "\", ";
            col_count++;
        }
    }
    else
    {
        work += "\"Description\", ";
        work += "\"Value\", ";
        col_count += 2;
    }
    work.remove(work.size() - 2, 2);
    work.append(" ) VALUES ( ");
    for (int ii = 0;  ii < col_count; ii++)
    {
        work += "?, ";
    }
    work.remove(work.size() - 2, 2);
    work.append(" )");
}

//void CSV::insert_row_primary_table(QString& qfile, )


