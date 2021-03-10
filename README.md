# GUI
Qt-based GUI for the overarching SCDA project.

[2021/02/08]
Rewrote significant portions of "Database" such that the data parsed from CSV files is now organized in a tree structure, rather than a 1D vector. Every spreadsheet data row which has subordinate rows can now be identified as a parent node for a subtable within the CSV. Instead of having one table per catalogue, each CSV file now has its own primary table, with dozens of nested subtables. Hopefully this structural change will enable vastly more articulation when searching for data from the SQL database. 

[2021/02/15]
The GUI app is now multithreaded. Small tasks are given one new thread, so as to avoid freezing the GUI. Large tasks are given a few parallel threads through which they perform their share of the total workload. The app gained a new button "Benchmark" which can perform a trial run of some multithreaded task using a variety of different internal methods (single-threaded, std multithreaded, Qt multithreaded) and report on the runtime of each method tested. When testing the insertion of large quantities of SQL statements, std multithreaded has demonstrated the best performance. 

Additionally, a multithreaded progress bar was added to the GUI. It will receive signals from each thread performing the task, and display the task's overall progress towards completion. 

[2021/02/22]
Restructured key functions so that large operations will iterate along the hierarchal pattern "catalogue->CSV->primary table->subtables". Done this way, the app can resume an interrupted or cancelled operation without having to restart. Also, more general functionality has been added, for both the GUI and SQL. More widgets are displaying information visually, and the thread wait time to access the database (in use) has been decreased. 

[2021/03/01]
Centralized all database writing operations to a single thread, after discovering occasional SQL errors when multiple threads carried out their own insertions. SQLite states that it can accomodate multiple write requests using internal mutexes, but even after enabling "Write-Ahead Logging", some errors persisted. 

More benchmarking was done, comparing the std/Windows C++ functions with the Qt functions. After centralizing the database write-access, stress tests were performed using different versions of key functions. Qt could insert one SQL statement every 4.5ms (the average taken from a long workload), while the raw SQLite functions could only insert one statement every 20ms. This holds true while inserting one statement at a time, but if using transactions, raw SQLite dashes ahead to one statement every 0.1ms. Qt has some transaction capability, but it is currently limited to the scope of one table at a time. Compared to SQLite inserting hundreds of thousands of statements per transaction, Qt cannot compete.

Given the slower performance of Qt, as well as its occasionally-stifling high-level API, the project will be migrating to std C++/Windows for most functions. The GUI will continue to mature in Qt, as it is simple and effective. 

[2021/03/10]
Planned final update for this repository. New region tables were added to the database, so that the database would contain the tree structure specified by Stats Canada for each catalogue. For example, some catalogues would structure their regions as country->province->city/municipality->subdistrict, while others would use country->postal code. These region tables are added during catalogue insertion, like the normal (data value) tables are. Also added a new tab to the catalogue display (list of tables), as well as a button to delete a specific catalogue from the database. 

As mentioned previously, part of the rationale for a new repository/code base is to move away from Qt-centric functions and variables which are not directly needed for the GUI. Another strong reason for the rewrite is to put emphasis on code generalizability and resuability, as this will be more efficient in the long-term for my future work, both in the overarching SCDA project and beyond.
