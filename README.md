# GUI
Qt-based GUI for the overarching SCDA project.

[2021/02/08]
Rewrote significant portions of "Database" such that the data parsed from CSV files is now organized in a tree structure, rather than a 1D vector. Every spreadsheet data row which has subordinate rows can now be identified as a parent node for a subtable within the CSV. Instead of having one table per catalogue, each CSV file now has its own primary table, with dozens of nested subtables. Hopefully this structural change will enable vastly more articulation when searching for data from the SQL database. 
