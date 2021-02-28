#pragma once
#ifndef BASICTOOLS_H
#define BASICTOOLS_H

#include <windows.h>
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

void err_bt(string);
void winerr_bt(string);

// Determine the type of number contained within the given string. 0 = error, 1 = int, 2 = double.
int qnum_test(QString);

// Return a piece of the original vector, defined by the first and last positions.
QVector<QString> string_vector_slicer(QVector<QString>&, int, int);

//template<typename S> void err(S&);
//template<typename S> void winerr(S&);
//template<typename S> void warn(S&);
//template<> void warn<wstring>(wstring&);
QString sqlerr_enum(QSqlError::ErrorType);
void sqlerr(QString&, QSqlError);

//template<typename S> void log(S);

//template<typename S> int clean(S&, int);
//template<> int clean<QString>(QString&, int);
int qclean(QString&, int);

// Given an INSERT statement template QString and the position of a parameter char ('?') inside it,
// replace the char with the given QString. The return value is the next such parameter char.
int insert_val(QString&, int, QString);
int insert_text(QString&, int, QString);

// From a given file path, writes that file's full content to the referenced (empty) string.
// Currently, can accomodate UTF8 strings, UTF16 wstrings, and QStrings.
//template<typename S1, typename S2> void load(S1&, S2&);
wstring bin_memory(HANDLE&);
QString q_memory(wstring&);
wstring w_memory(wstring&);

//template<typename S> void printer(S&, S&);

// Given a full path name, delete the file/folder.
template<typename S> void delete_file(S);
template<> void delete_file<string>(string);
template<> void delete_file<wstring>(wstring);
/*
template<> void delete_file<string>(string filename)
{
    HANDLE hfile = CreateFileA(filename.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr_bt("CreateFile-delete_sfile"); }
    if (!DeleteFileA(filename.c_str())) { winerr_bt("DeleteFile-delete_sfile"); }
    if (!CloseHandle(hfile)) { winerr_bt("CloseHandle-delete_sfile"); }
}
template<> void delete_file<wstring>(wstring filename)
{
    HANDLE hfile = CreateFileW(filename.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr_bt("CreateFile-delete_wfile"); }
    if (!DeleteFileW(filename.c_str())) { winerr_bt("DeleteFile-delete_wfile"); }
    if (!CloseHandle(hfile)) { winerr_bt("CloseHandle-delete_wfile"); }
}
*/
template<typename S> void delete_folder(S) {}
/*
template<> void delete_folder<string>(string folder_name)
{
    string folder_search = folder_name + "\\*";
    WIN32_FIND_DATAA info;
    HANDLE hfile1 = FindFirstFileA(folder_search.c_str(), &info);
    string file_name;
    do
    {
        file_name = folder_name + "\\" + info.cFileName;
        if (file_name.back() != '.') { delete_file(file_name); }
    } while (FindNextFileA(hfile1, &info));

    BOOL yesno = RemoveDirectoryA(folder_name.c_str());
    if (!yesno) { winerr_bt("RemoveDirectory-delete_sfolder"); }
}
template<> void delete_folder<wstring>(wstring folder_name)
{
    wstring folder_search = folder_name + L"\\*";
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    wstring file_name;
    do
    {
        file_name = folder_name + L"\\" + info.cFileName;
        if (file_name.back() != L'.') { delete_file(file_name); }
    } while (FindNextFileW(hfile1, &info));

    BOOL yesno = RemoveDirectoryW(folder_name.c_str());
    if (!yesno) { winerr_bt("RemoveDirectory-delete_wfolder"); }
}
*/

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
