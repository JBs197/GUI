#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QtSql>
#include "catalogue.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QSqlDatabase db;
    void sqlerr(QString, QSqlError);
    void logger(QString);

signals:

public slots:

private slots:

    void on_cB_drives_currentTextChanged(const QString &arg1);

    void on_pB_scan_clicked();

    void on_pB_insert_clicked();

    void on_pB_test_clicked();

    void on_pB_benchmark_clicked();

    void on_pB_viewdata_clicked();

    void on_pB_cancel_clicked();

private:
    Ui::MainWindow *ui;
    int location = 0;  // 0 = home, 1 = inn.
    int cores = 3;
    int bar_threads;
    int jobs_max;
    int jobs_done;
    int jobs_percent;
    int threads_working = 0;
    int remote_controller = 0;  // 0 = standard, 1 = cancel.
    bool begun_logging = 0;
    std::wstring wdrive;
    vector<mutex> m_jobs;
    QString qdrive;
    QMutex m_db, m_err, m_bar, m_namegen, m_id, m_io;
    vector<string> sroots = { "F:", "D:" };
    vector<wstring> wroots = { L"F:", L"D:" }; //  NOTE: REMOVE HARDCODING LATER.
    QVector<QString> qroots = { "F:", "D:" };
    QString db_path;
    QVector<QVector<QVector<QString>>> cata_tree;  // Form [year][catalogue][qyear, qname, qdescription].
    QMap<QString, int> map_tree_year;  // For a given qyear, return that cata_tree index.
    QMap<QString, int> map_tree_cata;  // For a given qname, return that cata_tree index.
    void build_ui_tree(QVector<QVector<QVector<QString>>>&, int);
    void add_children(QTreeWidgetItem*, QVector<QVector<QString>>&);
    void executor(QSqlQuery&);
    QSqlError executor_select(QString&, QSqlQuery&);
    void initialize_catalogue(CATALOGUE&, QString&, QString&);
    void clear_log();
    void reset_db(QString&);
    void update_bar();
    void reset_bar(int, QString);
    void initialize();
    QString sqlerr_enum(QSqlError::ErrorType);
    void sqlerr(QString&, QSqlError);
    //void nobles_st(CATALOGUE&);
    //void subtables_mt(CATALOGUE&);
    //void subtables_mapped(CATALOGUE&);
    //void populate_cata_mt(int);
    void update_text_vars(QVector<QVector<QString>>&, QString&);
    void update_cata_tree();
    void create_cata_index_table();
    void insert_cata_index(CATALOGUE&);
    void finish_cata_index(CATALOGUE&);
    void insert_cata_csvs_mt(CATALOGUE&);
    void create_csv_tables(CATALOGUE&, QString);
    void populate_csv_tables(CATALOGUE&, int);
    void insert_primary_row(QSqlQuery&, CATALOGUE&, QString&, QVector<QVector<QString>>&, QVector<QVector<QString>>&);
    void create_insert_csv_table(QSqlQuery&, CATALOGUE&, QString&, QVector<QVector<QString>>&);
    void create_insert_csv_subtables(QSqlQuery&, CATALOGUE&, QString&, QVector<QVector<QString>>&);
    void all_cata_db(QVector<QVector<QString>>&, QMap<QString, int>&);
    void scan_incomplete_cata(CATALOGUE&);
    void judicator(int&, int&, QString, QString);
    //void insert_catalogue_st(int&, int&, QString, QString);
    void insert_csvs(QVector<QString>&, int&, int&, wstring, vector<int>);
    void insert_primary_row_db(QVector<QString>&, int, CATALOGUE&, QString&, QVector<QVector<QString>>&, QVector<QVector<QString>>&);
    void create_insert_csv_table_db(QVector<QString>&, int, CATALOGUE&, QString&, QVector<QVector<QString>>&);
    void create_insert_csv_subtables_db(QVector<QString>&, int, CATALOGUE&, QString&, QVector<QVector<QString>>&);

    // TEMPLATES

    // Given a full path name and the content, print (threadsafely) to file with UTF-8 encoding.
    // Input strings can be simple/UTF8 strings, or wstrings, or QStrings.
    template<typename S> void printer(S&, S&) {}
    template<> void printer<string>(string& full_path, string& content)
    {
        lock_guard lock (m_io);
        HANDLE hfile = CreateFileA(full_path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, CREATE_ALWAYS, 0, NULL);
        if (hfile == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-sprinter"); }
        DWORD bytes_written;
        DWORD file_size = (DWORD)content.size();
        if (!WriteFile(hfile, content.c_str(), file_size, &bytes_written, NULL)) { winerr(L"WriteFile-sprinter"); }
        if (!CloseHandle(hfile)) { winerr(L"CloseHandle-sprinter"); }
    }
    template<> void printer<wstring>(wstring& full_path, wstring& content)
    {
        lock_guard lock (m_io);
        string path8 = utf16to8(full_path);
        string content8 = utf16to8(content);
        HANDLE hfile = CreateFileA(path8.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, CREATE_ALWAYS, 0, NULL);
        if (hfile == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-wprinter"); }
        DWORD bytes_written;
        DWORD file_size = (DWORD)content8.size();
        if (!WriteFile(hfile, content8.c_str(), file_size, &bytes_written, NULL)) { winerr(L"WriteFile-wprinter"); }
        if (!CloseHandle(hfile)) { winerr(L"CloseHandle-wprinter"); }
    }
    template<> void printer<QString>(QString& full_path, QString& content)
    {
        lock_guard lock (m_io);
        string path8 = full_path.toStdString();
        string content8 = content.toStdString();
        HANDLE hfile = CreateFileA(path8.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, CREATE_ALWAYS, 0, NULL);
        if (hfile == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-qprinter"); }
        DWORD bytes_written;
        DWORD file_size = (DWORD)content8.size();
        if (!WriteFile(hfile, content8.c_str(), file_size, &bytes_written, NULL)) { winerr(L"WriteFile-qprinter"); }
        if (!CloseHandle(hfile)) { winerr(L"CloseHandle-qprinter"); }
    }

    // (THREADSAFE) Make an entry into the error log. If the error is severe, terminate the application.
    template<typename S> void err(S&) {}
    template<> void err<string>(string& message)
    {
        string spath = sroots[location] + "\\SCDA Error Log.txt";
        string smessage = timestamperA() + " Generic Error: " + message + "\r\n\r\n";
        lock_guard lock(m_io);
        HANDLE hprinter = CreateFileA(spath.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointer(hprinter, NULL, NULL, FILE_END);
        DWORD bytes;
        DWORD fsize = (DWORD)smessage.size();
        WriteFile(hprinter, smessage.c_str(), fsize, &bytes, NULL);
        if (hprinter)
        {
            CloseHandle(hprinter);
        }
        exit(EXIT_FAILURE);
    }
    template<> void err<wstring>(wstring& message)
    {
        string spath = sroots[location] + "\\SCDA Error Log.txt";
        string smessage = timestamperA() + " Generic Error: " + utf16to8(message) + "\r\n\r\n";
        lock_guard lock(m_io);
        HANDLE hprinter = CreateFileA(spath.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointer(hprinter, NULL, NULL, FILE_END);
        DWORD bytes;
        DWORD fsize = (DWORD)smessage.size();
        WriteFile(hprinter, smessage.c_str(), fsize, &bytes, NULL);
        if (hprinter)
        {
            CloseHandle(hprinter);
        }
        exit(EXIT_FAILURE);
    }
    template<> void err<QString>(QString& message)
    {
        string spath = sroots[location] + "\\SCDA Error Log.txt";
        string smessage = timestamperA() + " Generic Error: " + message.toStdString() + "\r\n\r\n";
        lock_guard lock(m_io);
        HANDLE hprinter = CreateFileA(spath.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointer(hprinter, NULL, NULL, FILE_END);
        DWORD bytes;
        DWORD fsize = (DWORD)smessage.size();
        WriteFile(hprinter, smessage.c_str(), fsize, &bytes, NULL);
        if (hprinter)
        {
            CloseHandle(hprinter);
        }
        exit(EXIT_FAILURE);
    }
    template<typename S> void winerr(S&) {}
    template<> void winerr<string>(string& message)
    {
        DWORD num = GetLastError();
        string spath = sroots[location] + "\\SCDA Error Log.txt";
        string smessage = timestamperA() + " Windows Error #" + to_string(num) + ", from " + message + "\r\n\r\n";
        lock_guard lock(m_io);
        HANDLE hprinter = CreateFileA(spath.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointer(hprinter, NULL, NULL, FILE_END);
        DWORD bytes;
        DWORD fsize = (DWORD)smessage.size();
        WriteFile(hprinter, smessage.c_str(), fsize, &bytes, NULL);
        if (hprinter)
        {
            CloseHandle(hprinter);
        }
        exit(EXIT_FAILURE);
    }
    template<> void winerr<wstring>(wstring& message)
    {
        DWORD num = GetLastError();
        string spath = sroots[location] + "\\SCDA Error Log.txt";
        string smessage = timestamperA() + " Windows Error #" + to_string(num) + ", from " + utf16to8(message) + "\r\n\r\n";
        lock_guard lock(m_io);
        HANDLE hprinter = CreateFileA(spath.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointer(hprinter, NULL, NULL, FILE_END);
        DWORD bytes;
        DWORD fsize = (DWORD)smessage.size();
        WriteFile(hprinter, smessage.c_str(), fsize, &bytes, NULL);
        if (hprinter)
        {
            CloseHandle(hprinter);
        }
        exit(EXIT_FAILURE);
    }
    template<> void winerr<QString>(QString& message)
    {
        DWORD num = GetLastError();
        string spath = sroots[location] + "\\SCDA Error Log.txt";
        string smessage = timestamperA() + " Windows Error #" + to_string(num) + ", from " + message.toStdString() + "\r\n\r\n";
        lock_guard lock(m_io);
        HANDLE hprinter = CreateFileA(spath.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointer(hprinter, NULL, NULL, FILE_END);
        DWORD bytes;
        DWORD fsize = (DWORD)smessage.size();
        WriteFile(hprinter, smessage.c_str(), fsize, &bytes, NULL);
        if (hprinter)
        {
            CloseHandle(hprinter);
        }
        exit(EXIT_FAILURE);
    }
    template<typename S> void warn(S&) {}
    template<> void warn<string>(string& message)
    {
        string name = sroots[location] + "\\SCDA Error Log.txt";
        string smessage = timestamperA() + " Generic Warning: " + message + "\r\n\r\n";
        lock_guard lock(m_io);
        HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointer(hprinter, NULL, NULL, FILE_END);
        DWORD bytes;
        DWORD fsize = (DWORD)smessage.size();
        WriteFile(hprinter, smessage.c_str(), fsize, &bytes, NULL);
        if (hprinter)
        {
            CloseHandle(hprinter);
        }
    }
    template<> void warn<wstring>(wstring& message)
    {
        string name = sroots[location] + "\\SCDA Error Log.txt";
        string smessage = timestamperA() + " Generic Warning: " + utf16to8(message) + "\r\n\r\n";
        lock_guard lock(m_io);
        HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointer(hprinter, NULL, NULL, FILE_END);
        DWORD bytes;
        DWORD fsize = (DWORD)smessage.size();
        WriteFile(hprinter, smessage.c_str(), fsize, &bytes, NULL);
        if (hprinter)
        {
            CloseHandle(hprinter);
        }
    }
    template<> void warn<QString>(QString& message)
    {
        string name = sroots[location] + "\\SCDA Error Log.txt";
        string smessage = timestamperA() + " Generic Warning: " + message.toStdString() + "\r\n\r\n";
        lock_guard lock(m_io);
        HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointer(hprinter, NULL, NULL, FILE_END);
        DWORD bytes;
        DWORD fsize = (DWORD)smessage.size();
        WriteFile(hprinter, smessage.c_str(), fsize, &bytes, NULL);
        if (hprinter)
        {
            CloseHandle(hprinter);
        }
    }
    template<typename S> void winwarn(S&) {}
    template<> void winwarn<string>(string& message)
    {
        DWORD num = GetLastError();
        string spath = sroots[location] + "\\SCDA Error Log.txt";
        string smessage = timestamperA() + " Windows Error #" + to_string(num) + ", from " + message + "\r\n\r\n";
        lock_guard lock(m_io);
        HANDLE hprinter = CreateFileA(spath.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointer(hprinter, NULL, NULL, FILE_END);
        DWORD bytes;
        DWORD fsize = (DWORD)smessage.size();
        WriteFile(hprinter, smessage.c_str(), fsize, &bytes, NULL);
        if (hprinter)
        {
            CloseHandle(hprinter);
        }
    }
    template<> void winwarn<wstring>(wstring& message)
    {
        DWORD num = GetLastError();
        string spath = sroots[location] + "\\SCDA Error Log.txt";
        string smessage = timestamperA() + " Windows Error #" + to_string(num) + ", from " + utf16to8(message) + "\r\n\r\n";
        lock_guard lock(m_io);
        HANDLE hprinter = CreateFileA(spath.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointer(hprinter, NULL, NULL, FILE_END);
        DWORD bytes;
        DWORD fsize = (DWORD)smessage.size();
        WriteFile(hprinter, smessage.c_str(), fsize, &bytes, NULL);
        if (hprinter)
        {
            CloseHandle(hprinter);
        }
    }
    template<> void winwarn<QString>(QString& message)
    {
        DWORD num = GetLastError();
        string spath = sroots[location] + "\\SCDA Error Log.txt";
        string smessage = timestamperA() + " Windows Error #" + to_string(num) + ", from " + message.toStdString() + "\r\n\r\n";
        lock_guard lock(m_io);
        HANDLE hprinter = CreateFileA(spath.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointer(hprinter, NULL, NULL, FILE_END);
        DWORD bytes;
        DWORD fsize = (DWORD)smessage.size();
        WriteFile(hprinter, smessage.c_str(), fsize, &bytes, NULL);
        if (hprinter)
        {
            CloseHandle(hprinter);
        }
    }

    // Make an entry into the process log for the current runtime.
    // (THREADSAFE) Make an entry into the process log, for the most recent runtime.
    template<typename S> void log(S) {}
    template<> void log<string>(string note)
    {
        lock_guard lock(m_io);
        string name = sroots[location] + "\\SCDA Process Log.txt";
        string message = timestamperA() + "  " + note + "\r\n\r\n";
        HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hprinter == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-slog"); }
        if (!begun_logging)
        {
            if (!DeleteFileA(name.c_str())) { winerr(L"DeleteFile-slog"); }
            if (!CloseHandle(hprinter)) { winerr(L"CloseHandle-slog"); }
            hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hprinter == INVALID_HANDLE_VALUE) { winerr(L"CreateFile2-slog"); }
            begun_logging = 1;
        }
        SetFilePointer(hprinter, NULL, NULL, FILE_END);
        DWORD bytes;
        DWORD fsize = (DWORD)message.size();
        if (!WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL))
        {
            winerr("writefile-slog");
        }
        if (!CloseHandle(hprinter))
        {
            winerr("closehandle2-slog");
        }
    }
    template<> void log<wstring>(wstring wnote)
    {
        lock_guard lock(m_io);
        string name = sroots[location] + "\\SCDA Process Log.txt";
        string message = timestamperA() + "  " + utf16to8(wnote) + "\r\n\r\n";
        HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hprinter == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-wlog"); }
        if (!begun_logging)
        {
            if (!DeleteFileA(name.c_str())) { winerr(L"DeleteFile-wlog"); }
            if (!CloseHandle(hprinter)) { winerr(L"CloseHandle-wlog"); }
            hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hprinter == INVALID_HANDLE_VALUE) { winerr(L"CreateFile2-wlog"); }
            begun_logging = 1;
        }
        SetFilePointer(hprinter, NULL, NULL, FILE_END);
        DWORD bytes;
        DWORD fsize = (DWORD)message.size();
        if (!WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL))
        {
            winerr("writefile-wlog");
        }
        if (!CloseHandle(hprinter))
        {
            winerr("closehandle2-wlog");
        }
    }
    template<> void log<QString>(QString qnote)
    {
        lock_guard lock(m_io);
        string name = sroots[location] + "\\SCDA Process Log.txt";
        string message = timestamperA() + "  " + qnote.toStdString() + "\r\n\r\n";
        HANDLE hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hprinter == INVALID_HANDLE_VALUE) { winerr(L"CreateFile-qlog"); }
        if (!begun_logging)
        {
            if (!DeleteFileA(name.c_str())) { winerr(L"DeleteFile-qlog"); }
            if (!CloseHandle(hprinter)) { winerr(L"CloseHandle-qlog"); }
            hprinter = CreateFileA(name.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hprinter == INVALID_HANDLE_VALUE) { winerr(L"CreateFile2-qlog"); }
            begun_logging = 1;
        }
        SetFilePointer(hprinter, NULL, NULL, FILE_END);
        DWORD bytes;
        DWORD fsize = (DWORD)message.size();
        if (!WriteFile(hprinter, message.c_str(), fsize, &bytes, NULL))
        {
            winerr("writefile-qlog");
        }
        if (!CloseHandle(hprinter))
        {
            winerr("closehandle2-qlog");
        }
    }

    // Fill a string with the contents of a local file.
    // The path string type should match the file's encoding.
    template<typename S1, typename S2> void load(S1& full_path, S2& output)
    {
        /*
        int path_type = -1;
        int output_type = -1;

        if (is_same<S1, string>::value) { path_type = 1; }
        if (is_same<S1, wstring>::value) { path_type = 2; }
        if (is_same<S1, QString>::value) { path_type = 3; }

        if (is_same<S2, string>::value) { output_type = 10; }
        if (is_same<S2, wstring>::value) { output_type = 20; }
        if (is_same<S2, QString>::value) { output_type = 30; }

        if (path_type < 0 || output_type < 0) { err("is_same-load"); }
        int code = path_type + output_type;
        switch(code)
        {
        case 13:
            load<string, QString>(full_path, output);
            break;

        case 23:
            load<wstring, QString>(full_path, output);
            break;
        }
        */
    }
    template<> void load<string, string>(string& full_path, string& output)
    {
        HANDLE hfile = CreateFileA(full_path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, 0, NULL);
        if (hfile == INVALID_HANDLE_VALUE) { winerr("CreateFile-ssload"); }
        DWORD size = GetFileSize(hfile, NULL);
        LPSTR bufferA = new CHAR[size];
        DWORD bytes_read;
        if (!ReadFile(hfile, bufferA, size, &bytes_read, NULL)) { winerr("ReadFile-ssload"); }
        string sfile(bufferA, size);
        delete[] bufferA;
        output = sfile;
    }
    template<> void load<wstring, string>(wstring& full_path, string& output)
    {
        HANDLE hfile = CreateFileW(full_path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, 0, NULL);
        if (hfile == INVALID_HANDLE_VALUE) { winerr("CreateFile-wsload"); }
        DWORD size = GetFileSize(hfile, NULL);
        LPWSTR bufferW = new WCHAR[size / 2];
        DWORD bytes_read;
        if (!ReadFile(hfile, bufferW, size, &bytes_read, NULL)) { winerr("ReadFile-wsload"); }
        wstring wfile(bufferW, size / 2);
        delete[] bufferW;
        string sfile = utf16to8(wfile);
        output = sfile;
    }
    template<> void load<string, wstring>(string& full_path, wstring& output)
    {
        HANDLE hfile = CreateFileA(full_path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, 0, NULL);
        if (hfile == INVALID_HANDLE_VALUE) { winerr("CreateFile-ssload"); }
        DWORD size = GetFileSize(hfile, NULL);
        LPSTR bufferA = new CHAR[size];
        DWORD bytes_read;
        if (!ReadFile(hfile, bufferA, size, &bytes_read, NULL)) { winerr("ReadFile-swload"); }
        string sfile(bufferA, size);
        delete[] bufferA;
        wstring wfile = utf8to16(sfile);
        output = wfile;
    }
    template<> void load<wstring, wstring>(wstring& full_path, wstring& output)
    {
        HANDLE hfile = CreateFileW(full_path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, 0, NULL);
        if (hfile == INVALID_HANDLE_VALUE) { winerr("CreateFile-wsload"); }
        DWORD size = GetFileSize(hfile, NULL);
        LPWSTR bufferW = new WCHAR[size / 2];
        DWORD bytes_read;
        if (!ReadFile(hfile, bufferW, size, &bytes_read, NULL)) { winerr("ReadFile-wwload"); }
        wstring wfile(bufferW, size / 2);
        delete[] bufferW;
        output = wfile;
    }
    template<> void load<string, QString>(string& full_path, QString& output)
    {
        HANDLE hfile = CreateFileA(full_path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, 0, NULL);
        if (hfile == INVALID_HANDLE_VALUE) { winerr("CreateFile-ssload"); }
        DWORD size = GetFileSize(hfile, NULL);
        LPSTR bufferA = new CHAR[size];
        DWORD bytes_read;
        if (!ReadFile(hfile, bufferA, size, &bytes_read, NULL)) { winerr("ReadFile-sqload"); }
        string sfile(bufferA, size);
        delete[] bufferA;
        QString qfile = QString::fromStdString(sfile);
        output = qfile;
    }
    template<> void load<wstring, QString>(wstring& full_path, QString& output)
    {
        HANDLE hfile = CreateFileW(full_path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, 0, NULL);
        if (hfile == INVALID_HANDLE_VALUE) { winerr("CreateFile-wsload"); }
        DWORD size = GetFileSize(hfile, NULL);
        LPWSTR bufferW = new WCHAR[size / 2];
        DWORD bytes_read;
        if (!ReadFile(hfile, bufferW, size, &bytes_read, NULL)) { winerr("ReadFile-wqload"); }
        wstring wfile(bufferW, size / 2);
        delete[] bufferW;
        QString qfile = QString::fromStdWString(wfile);
        output = qfile;
    }

    // Remove string chars which cause problems with SQL formatting. Returns the number of blank spaces which
    // were present at the string's start, indicating the string is a subheading or subsubheading.
    // Mode 0 = identifier xstring, mode 1 = value xstring
    template<typename S> int clean(S& piece, int mode) {}
    template<> int clean<string>(string& piece, int mode)
    {
        int count = 0;
        size_t pos1, pos2;
        pos1 = 0;
        do
        {
            pos1 = piece.find('[', pos1);
            if (pos1 < piece.size())
            {
                pos2 = piece.find(']', pos1);
                piece.erase(pos1, pos2 - pos1 + 1);
            }
        } while (pos1 < piece.size());

        if (mode == 1)
        {
            pos1 = piece.find('\'');
            while (pos1 < piece.size())
            {
                piece.replace(pos1, 1, "''");
                pos1 = piece.find('\'', pos1 + 2);
            }
        }

        while (1)
        {
            if (piece.front() == ' ') { piece.erase(0, 1); count++; }
            else { break; }
        }

        while (1)
        {
            if (piece.back() == ' ') { piece.pop_back(); }
            else { break; }
        }

        return count;
    }
    template<> int clean<wstring>(wstring& piece, int mode)
    {
        int count = 0;
        size_t pos1, pos2;
        pos1 = 0;
        do
        {
            pos1 = piece.find(L'[', pos1);
            if (pos1 < piece.size())
            {
                pos2 = piece.find(L']', pos1);
                piece.erase(pos1, pos2 - pos1 + 1);
            }
        } while (pos1 < piece.size());

        if (mode == 1)
        {
            pos1 = piece.find(L'\'');
            while (pos1 < piece.size())
            {
                piece.replace(pos1, 1, L"''");
                pos1 = piece.find(L'\'', pos1 + 2);
            }
        }

        while (1)
        {
            if (piece.front() == L' ') { piece.erase(0, 1); count++; }
            else { break; }
        }

        while (1)
        {
            if (piece.back() == L' ') { piece.pop_back(); }
            else { break; }
        }

        return count;
    }
    template<> int clean<QString>(QString& piece, int mode)
    {
        int count = 0;
        int pos1, pos2;
        pos1 = 0;
        do
        {
            pos1 = piece.indexOf('[', pos1);
            if (pos1 < 0)
            {
                pos2 = piece.indexOf(']', pos1);
                piece.remove(pos1, pos2 - pos1 + 1);
            }
        } while (pos1 < piece.size());

        if (mode == 1)
        {
            pos1 = piece.indexOf('\'');
            while (pos1 < 0)
            {
                piece.replace(pos1, 1, "''");
                pos1 = piece.indexOf('\'', pos1 + 2);
            }
        }

        while (1)
        {
            if (piece.front() == ' ') { piece.remove(0, 1); count++; }
            else { break; }
        }

        while (1)
        {
            if (piece.back() == ' ') { piece.remove(piece.size() - 1, 1); }
            else { break; }
        }

        return count;
    }


};

#endif // MAINWINDOW_H
