#include "basictools.h"

vector<thread::id> thread_list;
vector<vector<wstring>> objects;
vector<wstring> domains = { L".com", L".net", L".org", L".edu", L".ca" };
bool begun_logging = 0;


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

// Local error functions for 'basictools'.
void err_bt(string func)
{
    string message = timestamperA() + " Generic error inside " + func;
    int error = MessageBoxA(NULL, message.c_str(), NULL, MB_OK);
    exit(EXIT_FAILURE);
}
void winerr_bt(string func)
{
    DWORD num = GetLastError();    
    string message = timestamperA() + " Windows error #" + to_string(num) + " inside " + func;
    int error = MessageBoxA(NULL, message.c_str(), NULL, MB_OK);
    exit(EXIT_FAILURE);
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


/*
// Given an INSERT statement template QString and the position of a parameter char ('?') inside it,
// replace the char with the given QString (wrapped in ''). The return value is the next such parameter char.
int insert_val(QString& stmt, int pos, QString val)
{
    clean(val, 1);
    stmt.replace(pos, 1, val);
    int pos1 = stmt.indexOf('?', pos);
    return pos1;
}
int insert_text(QString& stmt, int pos, QString text)
{
    QString temp = "[" + text + "]";
    stmt.replace(pos, 1, temp);
    int pos1 = stmt.indexOf('?', pos);
    return pos1;
}
*/

wstring bin_memory(HANDLE& hfile)
{
    DWORD size = GetFileSize(hfile, NULL);
    DWORD bytes_read;
    LPWSTR buffer = new WCHAR[size / 2];
    if (!ReadFile(hfile, buffer, size, &bytes_read, NULL)) { winerr_bt("ReadFile-bin_memory"); }
    wstring bin(buffer, size / 2);
    delete[] buffer;
    return bin;
}
QString q_memory(wstring& full_path)
{
    HANDLE hfile = CreateFileW(full_path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr_bt("CreateFile-q_memory"); }
    DWORD size = GetFileSize(hfile, NULL);
    DWORD bytes_read;
    LPWSTR buffer = new WCHAR[size / 2];
    if (!ReadFile(hfile, buffer, size, &bytes_read, NULL)) { winerr_bt("ReadFile-q_memory"); }
    QString qfile = QString::fromWCharArray(buffer, size / 2);
    delete[] buffer;
    return qfile;
}
wstring w_memory(wstring& full_path)
{
    HANDLE hfile = CreateFileW(full_path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE) { winerr_bt("CreateFile-w_memory"); }
    DWORD size = GetFileSize(hfile, NULL);
    DWORD bytes_read;
    LPWSTR buffer = new WCHAR[size / 2];
    if (!ReadFile(hfile, buffer, size, &bytes_read, NULL)) { winerr_bt("ReadFile-w_memory"); }
    wstring wfile(buffer, size / 2);
    delete[] buffer;
    return wfile;
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
    else { err_bt("InternetOpen"); return 1; }
    if (hconnect)
    {
        hrequest = HttpOpenRequestW(hconnect, NULL, object_name.c_str(), NULL, NULL, NULL, 0, context);
    }
    else { err_bt("InternetConnect"); return 2; }
    if (hrequest)
    {
        yesno = HttpSendRequest(hrequest, NULL, 0, NULL, 0);
    }
    else { err_bt("HttpOpenRequest"); return 3; }
    if (yesno)
    {
        do
        {
            bytes_available = 0;
            InternetQueryDataAvailable(hrequest, &bytes_available, 0, 0);
            ubufferA = new unsigned char[bytes_available];
            if (!InternetReadFile(hrequest, ubufferA, bytes_available, &bytes_read))
            {
                err_bt("InternetReadFile");
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
    else { err_bt("HttpSendRequest"); return 5; }

    HANDLE hprinter = CreateFileW(filepath.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, CREATE_ALWAYS, 0, NULL);
    if (hprinter == INVALID_HANDLE_VALUE) { err_bt("CreateFile"); return 6; }
    DWORD bytes_written;
    DWORD file_size = (DWORD)fileW.size() * 2;
    if (!WriteFile(hprinter, fileW.c_str(), file_size, &bytes_written, NULL)) { err_bt("WriteFile"); return 7; }

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
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr_bt("FindFirstFile-get_file_paths"); }
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

    if (!FindClose(hfile1)) { winerr_bt("FindClose-get_file_paths"); }
    return file_paths;
}
vector<wstring> get_file_path_endings(wstring folder_path, size_t pos0)
{
    vector<wstring> file_paths;
    wstring folder_search = folder_path + L"\\*";
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr_bt("FindFirstFile-get_file_path_endings"); }
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

    if (!FindClose(hfile1)) { winerr_bt("FindClose-get_file_path_endings"); }
    return file_paths;
}

// Given a folder path, return the number of files within that have the given extension suffix (param needs dot).
int get_file_path_number(wstring folder_path, wstring file_extension)
{
    int count = 0;
    wstring folder_search = folder_path + L"\\*" + file_extension;
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr_bt("FindFirstFile-get_file_path_number"); }
    do
    {
        count++;
    } while (FindNextFileW(hfile1, &info));

    if (!FindClose(hfile1)) { winerr_bt("FindClose-get_file_path_number"); }
    return count;
}

// Given a root folder path, return a vector containing the short names of all subfolders within.
vector<wstring> get_subfolder_shortnames(wstring root_folder)
{
    vector<wstring> subfolders;
    wstring folder_name;
    wstring folder_search = root_folder + L"\\*";
    WIN32_FIND_DATAW info;
    HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr_bt("FindFirstFile-get_subfolder_shortnames"); }
    DWORD attributes;

    do
    {
        attributes = info.dwFileAttributes;
        if (attributes == FILE_ATTRIBUTE_DIRECTORY)
        {
            subfolders.push_back(info.cFileName);
        }
    } while (FindNextFileW(hfile1, &info));

    if (!FindClose(hfile1)) { winerr_bt("FindClose-get_subfolder_shortnames"); }
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
    if (hfile1 == INVALID_HANDLE_VALUE) { winerr_bt("FindFirstFile-get_subfolders"); }
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
    if (!FindClose(hfile1)) { winerr_bt("FindClose-get_subfolders"); }

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
                        err_bt("pos error in extract_classic_rows");
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
