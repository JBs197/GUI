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
    extract_rows(pos1);
    extract_classic_rows(pos1);
    last_classic_row = row_titles.size() - 1;
    organize_subtables(pos1);
}

void CSV::set_gid(QString& gd)
{
    gid = gd;
}

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

// Find all possible subtrees for this CSV. Children have uniform indentation.
// Form [possibility index][parent genealogy, children][list of parents / list of children]
void CSV::tree_walker()
{
    int indentation = 0;
    int start_index = 0;
    QVector<int> genealogy;

    for (int ii = 0; ii < subtables[0].size(); ii++)  // For every parentless parent...
    {
        genealogy = { subtables[0][ii][0] };
        start_index = is_parent(tree, genealogy, indentation, start_index);
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

QVector<QString> CSV::get_subtable_names()
{
    return sub_tname_list;
}

// Recursively determine if the given row is a parent. If so, add it to the tree and do the same for all
// its children. Returns first yet-to-be checked index in the current indentation's subtable list.
int CSV::is_parent(QVector<QVector<QVector<int>>>& tree, QVector<int> genealogy, int indentation, int start_index)
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
                    new_start_index = is_parent(tree, new_genealogy, indentation + 1, new_start_index);  // ...
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

// Populate the CSV object with the CSV file's row titles, storing subtables (but not columns).
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

// For a given row (with values), return a unique row name by front-pushing the child's genealogy.
// Uses a persistent buffer in the CSV object. Remember to clear it when done!
QString CSV::unique_row_title(int row)
{
    int current_indent = unique_row_buffer.size() - 1;
    QString terminal = row_titles[row];
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

// Build a SQL statement to create the primary table, which encompasses the whole catalogue.
void CSV::create_table_cata(QVector<QString>& work)
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

// Build a list of SQL statements to create all the secondary (CSV) tables for this catalogue.
void CSV::create_table_csvs(QVector<QString>& work, QVector<QString>& gid_list)
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
void CSV::create_table_subs(QVector<QString>& work, QVector<QString>& gid_list)
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
        for (int jj = 0; jj < tree.size(); jj++)
        {
            sql1 = sublabelmaker(gid_list[ii], tree[jj]);
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
