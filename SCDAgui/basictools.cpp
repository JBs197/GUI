#include "basictools.h"

vector<thread::id> thread_list;
vector<vector<wstring>> objects;
bool begun_logging = 0;
wstring db_root = L"F:";
string db_root8 = "F:";
QString db_qroot = "F:";
wstring db_name = L"SCDA";
QString db_path = "F:\\SCDA.db";
string proj_root = "$${_PRO_FILE_PWD_}";
vector<wstring> domains = { L".com", L".net", L".org", L".edu", L".ca" };

// Text encoder conversion functions.
string utf16to8(wstring in)
{
    auto& f = use_facet<codecvt<wchar_t, char, mbstate_t>>(locale());
    mbstate_t mb{};
    string out(in.size() * f.max_length(), '\0');
    const wchar_t* past;
    char* future;
    f.out(mb, &in[0], &in[in.size()], past, &out[0], &out[out.size()], future);
    out.resize(future - &out[0]);
    return out;
}
wstring utf8to16(string input)
{
    auto& f = use_facet<codecvt<wchar_t, char, mbstate_t>>(locale());
    mbstate_t mb{};
    wstring output(input.size() * f.max_length(), L'\0');
    const char* past;
    wchar_t* future;
    f.in(mb, &input[0], &input[input.size()], past, &output[0], &output[output.size()], future);
    output.resize(future - &output[0]);
    return output;
}

// Return a timestamp from the system clock.
string timestamperA()
{
    char buffer[26];
    string timestampA;
    chrono::system_clock::time_point today = chrono::system_clock::now();
    time_t tt = chrono::system_clock::to_time_t(today);
    ctime_s(buffer, 26, &tt);
    for (int ii = 0; ii < 26; ii++)
    {
        if (buffer[ii] == '\0') { break; }
        else { timestampA.push_back(buffer[ii]); }
    }
    return timestampA;
}

// Determine the type of number contained within the given string. 0 = error, 1 = int, 2 = double.
int qnum_test(QString num)
{
    bool fine;
    num.toInt(&fine);
    if (fine) { return 1; }
    num.toDouble(&fine);
    if (fine) { return 2; }
    return 0;
}

// Return a piece of the original vector, defined by the first and last positions.
QVector<QString> string_vector_slicer(QVector<QString>& bushy, int bot, int top)
{
    QVector<QString> trim(top - bot + 1);
    copy(bushy.begin() + bot, bushy.begin() + top + 1, trim.begin());
    return trim;
}

// Make an entry into the error log. If severe, terminate the application.
void err(wstring func)
{
    string func8 = utf16to8(func);
    string name = db_root8 + "\\SCDA Error Log.txt";
    string message = timestamperA() + " Generic error: " + func8 + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
    exit(EXIT_FAILURE);
}
void err8(string func8)
{
    string name = db_root8 + "\\SCDA Error Log.txt";
    string message = timestamperA() + " Generic error: " + func8 + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
    exit(EXIT_FAILURE);
}
/*
void sqlerr(wstring func, QSqlError qerror)
{
    string func8 = utf16to8(func);
    string name = db_root8 + "\\SCDA Error Log.txt";
    QString qmessage = qerror.text();
    string message = timestamperA() + " SQL error inside " + func8 + "\r\n" + qmessage.toStdString() + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
    exit(EXIT_FAILURE);
}
*/
void winerr(wstring func)
{
    DWORD num = GetLastError();
    LPSTR buffer = new CHAR[512];
    string mod = "wininet.dll";
    LPCSTR modul = mod.c_str();
    FormatMessageA((FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE), GetModuleHandleA(modul), num, 0, buffer, 512, NULL);
    string winmessage(buffer, 512);
    delete[] buffer;
    string func8 = utf16to8(func);
    string name = db_root8 + "\\SCDA Error Log.txt";
    string message = timestamperA() + " Windows error #" + to_string(num) + " inside " + func8 + "\r\n" + winmessage + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
    exit(EXIT_FAILURE);
}
void warn(wstring func)
{
    string func8 = utf16to8(func);
    string name = db_root8 + "\\SCDA Error Log.txt";
    string message = timestamperA() + " Generic warning: " + func8 + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}
void warn8(string func8)
{
    string name = db_root8 + "\\SCDA Error Log.txt";
    string message = timestamperA() + " Generic warning: " + func8 + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}
void qwarn(QString qfunc)
{
    string func8 = qfunc.toStdString();
    string name = db_root8 + "\\SCDA Error Log.txt";
    string message = timestamperA() + " Warning: " + func8 + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}
void sqlwarn(wstring func, QSqlError qerror)
{
    string func8 = utf16to8(func);
    string name = db_root8 + "\\SCDA Error Log.txt";
    QString qmessage = qerror.text();
    string message = timestamperA() + " SQL warning inside " + func8 + "\r\n" + qmessage.toStdString() + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}
void winwarn(wstring func)
{
    DWORD num = GetLastError();
    LPSTR buffer = new CHAR[512];
    string mod = "wininet.dll";
    LPCSTR modul = mod.c_str();
    FormatMessageA((FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE), GetModuleHandleA(modul), num, 0, buffer, 512, NULL);
    string winmessage(buffer, 512);
    delete[] buffer;
    string func8 = utf16to8(func);
    string name = db_root8 + "\\SCDA Error Log.txt";
    string message = timestamperA() + " Windows warning #" + to_string(num) + " inside " + func8 + "\r\n" + winmessage + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}

// Make an entry into the process log, for the most recent runtime.
void log(wstring note)
{
    string note8 = utf16to8(note);
    string name = db_root8 + "\\SCDA Process Log.txt";
    string message = timestamperA() + "  " + note8 + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hprinter == INVALID_HANDLE_VALUE) { warn(L"CreateFile-log"); }
    if (!begun_logging)
    {
        if (!DeleteFileA(name.c_str())) { warn(L"DeleteFile-log"); }
        if (!CloseHandle(hprinter)) { warn(L"CloseHandle-log"); }
        hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hprinter == INVALID_HANDLE_VALUE) { warn(L"CreateFile-log(after deletion)"); }
        begun_logging = 1;
    }
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}
void log8(string note8)
{
    string name = db_root8 + "\\SCDA Process Log.txt";
    string message = timestamperA() + "  " + note8 + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hprinter == INVALID_HANDLE_VALUE) { warn(L"CreateFile-log"); }
    if (!begun_logging)
    {
        if (!DeleteFileA(name.c_str())) { warn(L"DeleteFile-log"); }
        if (!CloseHandle(hprinter)) { warn(L"CloseHandle-log"); }
        hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hprinter == INVALID_HANDLE_VALUE) { warn(L"CreateFile-log(after deletion)"); }
        begun_logging = 1;
    }
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}
void qlog(QString qnote)
{
    string note8 = qnote.toStdString();
    string name = db_root8 + "\\SCDA Process Log.txt";
    string message = timestamperA() + "  " + note8 + "\r\n";
    HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hprinter == INVALID_HANDLE_VALUE) { warn(L"CreateFile-log"); }
    if (!begun_logging)
    {
        if (!DeleteFileA(name.c_str())) { warn(L"DeleteFile-log"); }
        if (!CloseHandle(hprinter)) { warn(L"CloseHandle-log"); }
        hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hprinter == INVALID_HANDLE_VALUE) { warn(L"CreateFile-log(after deletion)"); }
        begun_logging = 1;
    }
    SetFilePointer(hprinter, NULL, NULL, FILE_END);
    DWORD bytes;
    DWORD fsize = (DWORD)message.size();
    WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL);
    if (hprinter)
    {
        CloseHandle(hprinter);
    }
}

// Remove string chars which cause problems with SQL formatting. Returns the number of blank spaces which
// were present at the string's start, indicating the string is a subheading or subsubheading.
// Mode 0 = identifier wstring, mode 1 = value wstring
int clean(wstring& out, int mode)
{
    int count = 0;
    size_t pos1, pos2;
    pos1 = out.find(L'[', 0);
    if (pos1 < out.size())
    {
        pos2 = out.find(L']', pos1);
        out.erase(pos1, pos2 - pos1 + 1);
    }
    /*
    pos1 = out.find(L'(', 0);
    if (pos1 < out.size())
    {
        pos2 = out.find(L')', pos1);
        out.erase(pos1, pos2 - pos1 + 1);
    }
    */
    if (mode == 1)
    {
        pos1 = out.find(L'\'', 0);
        while (pos1 < out.size())
        {
            out.replace(pos1, 1, L"''");
            pos1 = out.find(L'\'', pos1 + 2);
        }
    }

    /*
    pos1 = out.find(L'-', 0);
    while (pos1 < out.size())
    {
        out.replace(pos1, 1, L"\-");
        pos1 = out.find(L'-', pos1 + 1);
    }
    pos1 = out.find(L',', 0);
    while (pos1 < out.size())
    {
        out.replace(pos1, 1, L"_");
        pos1 = out.find(L',', pos1 + 1);
    }
    pos1 = out.find(L'<', 0);
    while (pos1 < out.size())
    {
        out.replace(pos1, 1, L"less_than_");
        pos1 = out.find(L'<', pos1 + 10);
    }
    pos1 = out.find(L'>', 0);
    while (pos1 < out.size())
    {
        out.replace(pos1, 1, L"greater_than_");
        pos1 = out.find(L'<', pos1 + 13);
    }
    pos1 = out.find(L'/', 0);
    while (pos1 < out.size())
    {
        out.replace(pos1, 1, L"_or_");
        pos1 = out.find(L'/', pos1 + 4);
    }
    pos1 = out.find(L'+', 0);
    while (pos1 < out.size())
    {
        out.replace(pos1, 1, L"_and_");
        pos1 = out.find(L'+', pos1 + 5);
    }
    */
    while (1)
    {
        if (out.front() == L' ') { out.erase(0, 1); count++; }
        else { break; }
    }
    while (1)
    {
        if (out.back() == L' ') { out.pop_back(); }
        else { break; }
    }
    /*
    pos1 = out.find(L' ', 0);
    while (pos1 < out.size())
    {
        out.replace(pos1, 1, L"_");
        pos1 = out.find(L' ', pos1 + 1);
    }
    */
    return count;
}
int qclean(QString& bbq, int mode)
{
    int count = 0;
    int pos1, pos2;
    pos1 = bbq.indexOf('[');
    if (pos1 > 0)
    {
        pos2 = bbq.indexOf(']', pos1);
        bbq.remove(pos1, pos2 - pos1 + 1);
    }
    if (mode == 1)
    {
        pos1 = bbq.indexOf('\'');
        while (pos1 > 0)
        {
            bbq.replace(pos1, 1, "''");
            pos1 = bbq.indexOf('\'', pos1 + 2);
        }
    }
    while (1)
    {
        if (bbq.front() == ' ') { bbq.remove(0, 1); count++; }
        else { break; }
    }
    while (1)
    {
        if (bbq.back() == ' ') { bbq.remove(bbq.size() - 1, 1); }
        else { break; }
    }
    return count;
}


// Read into memory a local file.
wstring bin_memory(HANDLE& hfile)
{
    DWORD size = GetFileSize(hfile, NULL);
    DWORD bytes_read;
    LPWSTR buffer = new WCHAR[size / 2];
    if (!ReadFile(hfile, buffer, size, &bytes_read, NULL)) { winerr(L"ReadFile-bin_memory"); }
    wstring bin(buffer, size / 2);
    delete[] buffer;
    return bin;
}
QString q_memory(wstring& full_path)
{
    HANDLE hfile = CreateFileW(full_path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-q_memory"); }
    DWORD size = GetFileSize(hfile, NULL);
    DWORD bytes_read;
    LPWSTR buffer = new WCHAR[size / 2];
    if (!ReadFile(hfile, buffer, size, &bytes_read, NULL)) { winerr(L"ReadFile-q_memory"); }
    QString qfile = QString::fromWCharArray(buffer, size / 2);
    delete[] buffer;
    return qfile;
}
wstring w_memory(wstring& full_path)
{
    HANDLE hfile = CreateFileW(full_path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-w_memory"); }
    DWORD size = GetFileSize(hfile, NULL);
    DWORD bytes_read;
    LPWSTR buffer = new WCHAR[size / 2];
    if (!ReadFile(hfile, buffer, size, &bytes_read, NULL)) { winerr(L"ReadFile-w_memory"); }
    wstring wfile(buffer, size / 2);
    delete[] buffer;
    return wfile;
}

// Given a full path name and a string, print that string to file (UTF-8 encoding).
void sprinter(string full_path, string& content)
{
    HANDLE hfile = CreateFileA(full_path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, CREATE_ALWAYS, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-sprinter"); }
    DWORD bytes_written;
    DWORD file_size = (DWORD)content.size();
    if (!WriteFile(hfile, content.c_str(), file_size, &bytes_written, NULL)) { winerr(L"WriteFile-sprinter"); }
    if (!CloseHandle(hfile)) { winerr(L"CloseHandle-sprinter"); }
}
void wprinter(wstring full_path, wstring& content)
{
    string path8 = utf16to8(full_path);
    string content8 = utf16to8(content);
    HANDLE hfile = CreateFileA(path8.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, CREATE_ALWAYS, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-wprinter"); }
    DWORD bytes_written;
    DWORD file_size = (DWORD)content8.size();
    if (!WriteFile(hfile, content8.c_str(), file_size, &bytes_written, NULL)) { winerr(L"WriteFile-wprinter"); }
    if (!CloseHandle(hfile)) { winerr(L"CloseHandle-wprinter"); }
}
void qprinter(QString full_path, QString& content)
{
    string path8 = full_path.toStdString();
    string content8 = content.toStdString();
    HANDLE hfile = CreateFileA(path8.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, CREATE_ALWAYS, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-qprinter"); }
    DWORD bytes_written;
    DWORD file_size = (DWORD)content8.size();
    if (!WriteFile(hfile, content8.c_str(), file_size, &bytes_written, NULL)) { winerr(L"WriteFile-qprinter"); }
    if (!CloseHandle(hfile)) { winerr(L"CloseHandle-qprinter"); }
}

// Given a full path name, delete the file/folder.
void delete_file(wstring filename)
{
    HANDLE hfile = CreateFileW(filename.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-delete_file"); }
    if (!DeleteFileW(filename.c_str())) { winerr(L"DeleteFile-delete_file"); }
    if (!CloseHandle(hfile)) { winerr(L"CloseHandle-delete_file"); }
}
void delete_folder(wstring folder_name)
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
    if (yesno) { log(L"Succeeded in deleting folder " + folder_name + L"\r\n"); }
    else { winerr(L"RemoveDirectory-delete_folder"); }
}

// Contains pre-programmed responses to certain automated events during server-client communications.
void CALLBACK call(HINTERNET hint, DWORD_PTR dw_context, DWORD dwInternetStatus, LPVOID status_info, DWORD status_info_length)
{
    UNREFERENCED_PARAMETER(hint);
    UNREFERENCED_PARAMETER(dw_context);
    UNREFERENCED_PARAMETER(dwInternetStatus);
    UNREFERENCED_PARAMETER(status_info_length);

    REQUEST_CONTEXT* cpContext;
    cpContext = (REQUEST_CONTEXT*)status_info;
    string temp;
    wstring wtemp;
    bool yesno = 0;
    thread::id my_id = this_thread::get_id();

    switch (dwInternetStatus)
    {
    case INTERNET_STATUS_CLOSING_CONNECTION:

        break;
    case INTERNET_STATUS_CONNECTED_TO_SERVER:

        break;
    case INTERNET_STATUS_CONNECTING_TO_SERVER:

        break;
    case INTERNET_STATUS_CONNECTION_CLOSED:

        break;
    case INTERNET_STATUS_HANDLE_CLOSING:

        break;
    case INTERNET_STATUS_HANDLE_CREATED:

        break;
    case INTERNET_STATUS_INTERMEDIATE_RESPONSE:

        break;
    case INTERNET_STATUS_NAME_RESOLVED:

        break;
    case INTERNET_STATUS_RECEIVING_RESPONSE:

        break;
    case INTERNET_STATUS_RESPONSE_RECEIVED:

        break;
    case INTERNET_STATUS_REDIRECT:
        //mcallback.lock();
        for (int ii = 0; ii < 512; ii++)
        {
            if ((cpContext->szMemo)[ii] != '\0') { temp.push_back((cpContext->szMemo)[ii]); }
        }
        //mcallback.unlock();
        for (size_t ii = 0; ii < temp.size(); ii++)
        {
            if (temp[ii] == '/') { yesno = 1; }
            else if (temp[ii] < 0)
            {
                for (size_t jj = 0; jj < thread_list.size(); jj++)
                {
                    if (thread_list[jj] == my_id)
                    {
                        objects[jj].push_back(wtemp);
                        break;
                    }
                }
                break;
            }
            if (yesno)
            {
                wtemp.push_back((wchar_t)temp[ii]);
            }
        }
        break;
    case INTERNET_STATUS_REQUEST_COMPLETE:

        break;
    case INTERNET_STATUS_REQUEST_SENT:

        break;
    case INTERNET_STATUS_RESOLVING_NAME:

        break;
    case INTERNET_STATUS_SENDING_REQUEST:

        break;
    case INTERNET_STATUS_STATE_CHANGE:

        break;
    }
}

// Given destination folder and filename, will download the file at the URL.
int download(wstring url, wstring folder, wstring filename)
{
    wstring filepath = folder + L"\\" + filename;
    wstring server_name;
    wstring object_name;
    size_t cut_here;
    for (size_t ii = 0; ii < domains.size(); ii++)
    {
        cut_here = url.rfind(domains[ii]);
        if (cut_here <= url.size())
        {
            server_name = url.substr(0, cut_here + domains[ii].size());
            object_name = url.substr(cut_here + domains[ii].size(), url.size() - cut_here - domains[ii].size());
            break;
        }
    }

    INTERNET_STATUS_CALLBACK InternetStatusCallback;
    DWORD context = 1;
    BOOL yesno = 0;
    wstring agent = L"downloader";
    HINTERNET hint = NULL;
    HINTERNET hconnect = NULL;
    HINTERNET hrequest = NULL;
    DWORD bytes_available;
    DWORD bytes_read = 0;
    unsigned char* ubufferA = new unsigned char[1];
    wstring fileW;
    bool special_char = 0;

    hint = InternetOpenW(agent.c_str(), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hint)
    {
        InternetStatusCallback = InternetSetStatusCallback(hint, (INTERNET_STATUS_CALLBACK)call);
        hconnect = InternetConnectW(hint, server_name.c_str(), INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, context);
    }
    else { warn(L"InternetOpen"); return 1; }
    if (hconnect)
    {
        hrequest = HttpOpenRequestW(hconnect, NULL, object_name.c_str(), NULL, NULL, NULL, 0, context);
    }
    else { warn(L"InternetConnect"); return 2; }
    if (hrequest)
    {
        yesno = HttpSendRequest(hrequest, NULL, 0, NULL, 0);
    }
    else { warn(L"HttpOpenRequest"); return 3; }
    if (yesno)
    {
        do
        {
            bytes_available = 0;
            InternetQueryDataAvailable(hrequest, &bytes_available, 0, 0);
            ubufferA = new unsigned char[bytes_available];
            if (!InternetReadFile(hrequest, ubufferA, bytes_available, &bytes_read))
            {
                warn(L"InternetReadFile");
                return 4;
            }
            for (size_t ii = 0; ii < bytes_available; ii++)
            {
                fileW.push_back((wchar_t)ubufferA[ii]);
                if (fileW.back() == L'\x00C3' && !special_char)
                {
                    special_char = 1;
                }
                else if (special_char)
                {
                    fileW[fileW.size() - 2] = fileW[fileW.size() - 1] + 64;
                    fileW.pop_back();
                    special_char = 0;
                }
            }
        } while (bytes_available > 0);
        delete[] ubufferA;
    }
    else { warn(L"HttpSendRequest"); return 5; }

    HANDLE hprinter = CreateFileW(filepath.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, CREATE_ALWAYS, 0, NULL);
    if (hprinter == INVALID_HANDLE_VALUE) { warn(L"CreateFile"); return 6; }
    DWORD bytes_written;
    DWORD file_size = (DWORD)fileW.size() * 2;
    if (!WriteFile(hprinter, fileW.c_str(), file_size, &bytes_written, NULL)) { warn(L"WriteFile"); return 7; }

    if (hrequest) { InternetCloseHandle(hrequest); }
    if (hconnect) { InternetCloseHandle(hconnect); }
    if (hint) { InternetCloseHandle(hint); }
    return 0;
}

// Given a folder path, return a vector containing the file paths of all files within. Does not list subfolders.
vector<wstring> get_file_paths(wstring folder_path)
{
    vector<wstring> file_paths;
    wstring folder_search = folder_path + L"\\*";
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr(L"FindFirstFile-get_file_paths"); }
    wstring file_path;
    DWORD attributes;

    do
    {
        attributes = info.dwFileAttributes;
        if (attributes == FILE_ATTRIBUTE_NORMAL)
        {
            file_path = folder_path + L"\\" + info.cFileName;
            file_paths.push_back(file_path);
        }
    } while (FindNextFileW(hfile1, &info));

    if (!FindClose(hfile1)) { winwarn(L"FindClose-get_file_paths"); }
    return file_paths;
}
vector<wstring> get_file_path_endings(wstring folder_path, size_t pos0)
{
    vector<wstring> file_paths;
    wstring folder_search = folder_path + L"\\*";
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr(L"FindFirstFile-get_file_path_endings"); }
    wstring temp1, file_path;
    DWORD attributes;

    do
    {
        attributes = info.dwFileAttributes;
        if (attributes == FILE_ATTRIBUTE_NORMAL || attributes == FILE_ATTRIBUTE_ARCHIVE)
        {
            temp1 = folder_path + L"\\" + info.cFileName;
            file_path = temp1.substr(pos0);
            file_paths.push_back(file_path);
        }
    } while (FindNextFileW(hfile1, &info));

    if (!FindClose(hfile1)) { winwarn(L"FindClose-get_file_path_endings"); }
    return file_paths;
}

// Given a root folder path, return a vector containing the short names of all subfolders within.
vector<wstring> get_subfolder_shortnames(wstring root_folder)
{
    vector<wstring> subfolders;
    wstring folder_name;
    wstring folder_search = root_folder + L"\\*";
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr(L"FindFirstFile-get_subfolder_shortnames"); }
    DWORD attributes;

    do
    {
        attributes = info.dwFileAttributes;
        if (attributes == FILE_ATTRIBUTE_DIRECTORY)
        {
            subfolders.push_back(info.cFileName);
        }
    } while (FindNextFileW(hfile1, &info));

    if (!FindClose(hfile1)) { winwarn(L"FindClose-get_subfolder_shortnames"); }
    return subfolders;
}

// Given a root folder path, return a vector containing the full paths of all subfolders within.
vector<wstring> get_subfolders(wstring root_folder)
{
    vector<wstring> subfolders;
    wstring folder_name;
    wstring folder_search = root_folder + L"\\*";
    size_t pos1;
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr(L"FindFirstFile-get_subfolders"); }
    do
    {
        folder_name = root_folder + L"\\" + info.cFileName;
        pos1 = folder_name.find(L'.', 0);
        if (pos1 < folder_name.size()) { continue; }
        else
        {
            subfolders.push_back(folder_name);
        }
    } while (FindNextFileW(hfile1, &info));
    if (!FindClose(hfile1)) { winwarn(L"FindClose-get_subfolders"); }

    for (int ii = (int)subfolders.size() - 1; ii >= 0; ii--)
    {
        pos1 = subfolders[ii].find(L"System Volume Information", 0);
        if (pos1 < subfolders[ii].size())
        {
            subfolders.erase(subfolders.begin() + ii);
        }
    }

    return subfolders;
}
vector<vector<wstring>> get_subfolders2(wstring root_folder)
{
    vector<vector<wstring>> subfolders2;
    vector<wstring> subfolders_temp;
    vector<wstring> subfolders1 = get_subfolders(root_folder);

    for (size_t ii = 0; ii < subfolders1.size(); ii++)
    {
        subfolders_temp = get_subfolders(subfolders1[ii]);
        subfolders2.push_back(subfolders_temp);
    }

    return subfolders2;
}

// Search a given vector for a particular value, and return the index. If not found, insert and return.
// Returns negative (error) if a new entry fails to match and is not the largest entry.
int index_card(vector<int>& shelf, int book)
{
    int size = (int)shelf.size();
    for (int ii = 0; ii < size; ii++)
    {
        if (book == shelf[ii])
        {
            return ii;
        }
    }
    if (book < shelf[size - 1])
    {
        return -1;
    }
    shelf.push_back(book);
    return size;
}

// Returns a CSV's data rows.
QVector<QVector<QString>> extract_csv_data_rows(QString& qfile, QVector<QString> row_titles, bool multi_column)
{
    QVector<QVector<QString>> rows(row_titles.size(), QVector<QString>());
    int pos1, pos2, pos3, nl1, nl2;
    QChar math;
    pos1 = qfile.size() - 1;
    do
    {
        pos1 = qfile.lastIndexOf('=', pos1 - 1);
        math = qfile[pos1 - 1];
        if (math == '<' || math == '>')
        {
            pos2 = 1;
        }
        else
        {
            pos2 = 0;
        }
    } while (pos2);

    nl1 = qfile.indexOf('\n', pos1);
    if (multi_column)
    {
        nl1 = qfile.indexOf('\n', nl1 + 1);
    }
    nl2 = qfile.indexOf('\n', nl1 + 1);

    int row_index = -1;
    QString temp1;
    for (int ii = 0; ii < row_titles.size(); ii++)
    {
        row_index++;
        rows[row_index].append(row_titles[row_index]);

        pos1 = qfile.lastIndexOf('"', nl2);
        pos2 = qfile.indexOf(',', pos1);
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
                rows[row_index].append(temp1);
            }
            else
            {
                temp1 = qfile.mid(pos1 + 1, pos2 - pos1 - 1);
                rows[row_index].append(temp1);
            }
        } while (pos2 < nl2);  // All strings after the first are values, in string form.

        nl1 = nl2;
        nl2 = qfile.indexOf('\n', nl1 + 1);
    }
    return rows;
}
