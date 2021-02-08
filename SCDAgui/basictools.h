#ifndef BASICTOOLS_H
#define BASICTOOLS_H

//#include <chrono>
//#include <ctime>
#include <windows.h>
//#include <locale>
#include <wininet.h>
#include <thread>
#include <QtSql>

#pragma comment(lib, "wininet.lib")
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

// Make an entry into the error log. If severe, terminate the application.
void err(wstring);
void err8(string);
//void sqlerr(wstring, QSqlError);
void winerr(wstring);
void warn(wstring);
void warn8(string);
void sqlwarn(wstring, QSqlError);
void winwarn(wstring);

// Make an entry into the process log, for the most recent runtime.
void log(wstring);
void log8(string);

// Remove string chars which cause problems with SQL formatting. Returns the number of blank spaces which
// were present at the string's start, indicating the string is a subheading or subsubheading.
// Mode 0 = identifier wstring, mode 1 = value wstring
int clean(wstring&, int);
int qclean(QString&, int);

// Read into memory a local file.
wstring bin_memory(HANDLE&);
QString q_memory(HANDLE&);
wstring w_memory(wstring&);

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

// Given a root folder path, return a vector containing the full paths of all subfolders within.
vector<wstring> get_subfolders(wstring);
vector<vector<wstring>> get_subfolders2(wstring);

// Search a given vector for a particular value, and return the index. If not found, insert and return.
// Returns negative (error) if a new entry fails to match and is not the largest entry.
int index_card(vector<int>&, int);

#endif
