#ifndef BASICTOOLS_H
#define BASICTOOLS_H

//#include <chrono>
//#include <ctime>
#include <windows.h>
//#include <locale>
#include <wininet.h>
#include <winuser.h>
#include <thread>
#include <QtSql>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib,"user32.lib")
using namespace std;

typedef struct {
    HWND       hWindow;
    int        nStatusList;
    HINTERNET hresource;
    char szMemo[512];
} REQUEST_CONTEXT;

// Text encoder conversion functions.
string utf16to8(wstring);
wstring utf8to16(string);

// Return a timestamp from the system clock.
string timestamperA();

// Determine the type of number contained within the given string. 0 = error, 1 = int, 2 = double.
int qnum_test(QString);

// Return a piece of the original vector, defined by the first and last positions.
QVector<QString> string_vector_slicer(QVector<QString>&, int, int);

// Make an entry into the error log. If severe, terminate the application.
void err(wstring);
void err8(string);
//void sqlerr(wstring, QSqlError);
void winerr(wstring);
void warn(wstring);
void warn8(string);
void qwarn(QString);
void sqlwarn(wstring, QSqlError);
void winwarn(wstring);

// Make an entry into the process log, for the most recent runtime.
void log(wstring);
void log8(string);
void qlog(QString);

// Remove string chars which cause problems with SQL formatting. Returns the number of blank spaces which
// were present at the string's start, indicating the string is a subheading or subsubheading.
// Mode 0 = identifier wstring, mode 1 = value wstring
int clean(wstring&, int);
int qclean(QString&, int);

// Given an INSERT statement template QString and the position of a parameter char ('?') inside it,
// replace the char with the given QString. The return value is the next such parameter char.
int insert_val(QString&, int, QString);
int insert_text(QString&, int, QString);

// Read into memory a local file.
wstring bin_memory(HANDLE&);
QString q_memory(wstring&);
wstring w_memory(wstring&);

// Given a full path name and a string, print that string to file (UTF-8 encoding).
void sprinter(string, string&);
void wprinter(wstring, wstring&);
void qprinter(QString, QString&);

// Given a full path name, delete the file/folder.
void delete_file(wstring);
void delete_folder(wstring);

// Contains pre-programmed responses to certain automated events during server-client communications.
void CALLBACK call(HINTERNET, DWORD_PTR, DWORD, LPVOID, DWORD);

// Given destination folder and filename, will download the file at the URL.
int download(wstring, wstring, wstring);

// Given a folder path, return a vector containing the file paths of all files within. Does not list subfolders.
vector<wstring> get_file_paths(wstring);
vector<wstring> get_file_path_endings(wstring, size_t);
int get_file_path_number(wstring, wstring);

// Given a root folder path, return a vector containing the full paths of all subfolders within.
vector<wstring> get_subfolders(wstring);
vector<vector<wstring>> get_subfolders2(wstring);

// Search a given vector for a particular value, and return the index. If not found, insert and return.
// Returns negative (error) if a new entry fails to match and is not the largest entry.
int index_card(vector<int>&, int);

// Returns a CSV's data rows.
QVector<QVector<QString>> extract_csv_data_rows(QString&, QVector<QString>, bool);

#endif
