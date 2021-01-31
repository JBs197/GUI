#ifndef DBCLASSES_H
#define DBCLASSES_H

#include <string>
#include <windows.h>
#include <QtSql>
//#include "basictools.h"

using namespace std;

// This object represents one CSV spreadsheet file.
class CSV_DB {
    wstring wfile; // Full CSV file.
    int GID; // Numeric code for the geographical region.
    string cata_number; // Stats Can's alphanumeric designation for the catalogue.
    bool making_the_model;  // If true, will determine each column value's integer/decimal status for the CATA_DB.
    vector<bool> is_int; // Its length is equal to the number of columns.
    vector<int> int_val; // All the integer values of the column, in order.
    vector<double> double_val; // All the decimal values of the column, in order.
    vector<vector<string>> variables;  // Form [variable][variable type, variable shown in CSV]
    vector<string> columns; // Descriptor title for each value in the 1D CSV row.
public:
    CSV_DB(wstring& wf, int gid, string cn, vector<bool> is_integer) { wfile = wf; GID = gid; cata_number = cn; is_int = is_integer; making_the_model = 0; }
    CSV_DB(wstring& wf, int gid, string cn) { wfile = wf; GID = gid; cata_number = cn; making_the_model = 1; }
    ~CSV_DB() {}
    int read(); // Populates the internal variables. Returns greater than zero for errors.
    vector<string> get_varicols(); // Returns a list of the column titles.
    string get_insert(); // Returns a SQL INSERT statement string for the complete CSV.
    int bad_apple(wstring); // Given the full path of a corrupt CSV file, re-download it using the archive bin.
    vector<vector<string>> get_variables() { return variables; }
    vector<string> get_columns() { return columns; }
    vector<bool> get_value_types() { return is_int; }
};

// This object represents one catalogue folder, as geographically categorized by Statistics Canada.
class CATA_DB {
    wstring folder; // Full path of the catalogue folder containing the CSVs.
    int col_width;
    string cata_number; // Stats Can's alphanumeric designation for the catalogue.
    string table_name; // The catalogue number, immediately proceeded by a 'T'.
    string cata_name; // The phrase describing the catalogue's topic.
    vector<bool> is_int; // Its length is equal to the number of columns.
    vector<string> column_header; // Descriptor title for each value in the 1D CSV row.
    vector<vector<string>> variables; // Form [variable][variable type, variable shown in CSV]
public:
    CATA_DB(wstring folder_path) { init(folder_path); }
    ~CATA_DB() {}
    void init(wstring); // Given a catalogue folder, populate the new CATA_DB object's internal variables.
    void prepare_table(QSqlDatabase&); // Each table in the database corresponds to one catalogue.
    void populate_table(QSqlDatabase&); // Read every CSV's values into the table.
};

// This object represents the organizational union of all catalogues chosen to be loaded.
class TREE_DB {
    wstring root_folder;
    vector<wstring> year_folders; // Indices match the first dimension of the 'catalogue_paths' matrix.
    vector<vector<wstring>> cata_folders;
    vector<vector<CATA_DB>> trunk; // Form [Year][Catalogue]
public:
    TREE_DB() {}
    ~TREE_DB() {}
    void tree_assembly(QSqlDatabase&, vector<wstring>&); // Given a root folder, build the database using all CSVs in all subfolders within.
    CATA_DB tablemaker(QSqlDatabase&, wstring); // Given a database and a folder containing CSV files, make a table for that catalogue.
};

// This object controls all QTSQL functions.
class NEXUS_DB {
    QSqlDatabase db;
    TREE_DB treebeard;

public:
    NEXUS_DB(const QString&);
    ~NEXUS_DB() {}
    void tree_planting(vector<wstring>&);
    QStringList get_table_list();
};

#endif
