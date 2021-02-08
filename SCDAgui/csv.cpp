#include "csv.h"

using namespace std;

// Function to setup the CSV object, in case 'scan' is overkill.
void CSV::initialize(QString& qf, QString& qn)
{
    qfile = qf;
    qname = qn;
    subqname = "000000";
}

// For a given CSV string, extract all desired data. Automatically performs 'initialize' as well.
void CSV::scan(QString& qf, QString& qn)
{
    qfile = qf;
    qname = qn;
    subqname = "000000";
    int pos0 = extract_variables();
    int pos1 = extract_column_titles(pos0);
    extract_rows(pos1);
    organize_subtables(pos1);

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

// Find all possible trees for a given parent row. Children have uniform indentation.
// Form [possibility index][parent, middle objects...][children]
int CSV::tree_walker(QVector<QVector<QVector<int>>>& tree, int parent)
{
    tree.clear();
    int indentation = indent[parent];
    int subtable_index, max_indent, num1, num2;
    QVector<QVector<int>> children;  // Form [generations from parent][children of ACTIVE row]
    max_indent = subtables.size() - 1;
    for (int ii = 0; ii < subtables[indentation].size(); ii++)
    {
        num1 = subtables[indentation][ii][0];
        if (num1 == parent)
        {
            subtable_index = ii;
            children.append(subtables[indentation][ii]);
            children.removeFirst();
            while (indentation < max_indent)
            {
                indentation++;
                for (int jj = 0;)  // RESUME HERE
            }
            break;
        }
        else if (num1 > parent)
        {
            return 1;  // This row is not a parent.
        }
        else if (ii == subtables[indentation].size() - 1)
        {
            return 1;
        }
    }


    return 0;
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

    /*
    if (column_titles.size() < 2)
    {
        column_titles.clear();
        column_titles.append("Description");
        column_titles.append("Value");
        return nl1;  // This was a data line - redo it later.
    }
    */

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

// Generate a list of SQL statements to create the main table and all subtables for this catalogue.
// The main table is always the first list element, and the returned integer is the number of tables made.
int CSV::create_all_table_statements(QVector<QString>& work)
{
    // Make the main table first...
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
        sql += row_titles[row];
        if (is_int[ii][1] == 1) { sql += "\" INTEGER, "; }
        else if (is_int[ii][1] == 0) { sql += "\" REAL, "; }
        else { err8("Missing is_int values-csv.create_all_table_statements"); }
    }
    sql.remove(sql.size() - 2, 2);
    sql.append(" );");
    work.append(sql);

    // And now we go nuts...
    QString sub_tname = "T" + qname + "$" + subqname_gen();
    QVector<QVector<QVector<int>>> tree_paths; // Form [path possibility][trunk, branches...][leaves]



    return 0;
}

