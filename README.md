# GUI
Qt-based GUI for the overarching SCDA project.

[2021/02/08]
Rewrote significant portions of "Database" such that the data parsed from CSV files is now organized in a tree structure, rather than a 1D vector. Every spreadsheet data row which has subordinate rows can now be identified as a parent node for a subtable within the CSV. Instead of having one table per catalogue, each CSV file now has its own primary table, with dozens of nested subtables. Hopefully this structural change will enable vastly more articulation when searching for data from the SQL database. 

[2021/-2/15]
The GUI app is now multithreaded. Small tasks are given one new thread, so as to avoid freezing the GUI. Large tasks are given a few parallel threads through which they perform their share of the total workload. The app gained a new button "Benchmark" which can perform a trial run of some multithreaded task using a variety of different internal methods (single-threaded, std multithreaded, Qt multithreaded) and report on the runtime of each method tested. When testing the insertion of large quantities of SQL statements, std multithreaded has demonstrated the best performance. 

Additionally, a multithreaded progress bar was added to the GUI. It will receive signals from each thread performing the task, and display the task's overall progress towards completion. 
