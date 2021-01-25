#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <sqlite3.h>
#include <windows.h>
#include <wininet.h>
#include <thread>
#include <mutex>
#include <locale>
#include "Database.h"

#pragma comment(lib, "wininet.lib")

mutex mcout8, mcout16, mpo, mcallback, merror;
vector<thread::id> thread_list;
vector<vector<wstring>> objects;
ofstream ERR(error_log_path, ios_base::out | ios_base::app);

typedef struct {
	HWND       hWindow;
	int        nStatusList;
	HINTERNET hresource;
	char szMemo[512];
} REQUEST_CONTEXT;

// Thread-safe messaging functions.
inline void mtcout16(wstring message)
{
	mcout16.lock();
	wcout << message << endl;
	mcout16.unlock();
}
inline void mtcout8(string message)
{
	mcout8.lock();
	cout << message << endl;
	mcout8.unlock();
}

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
	errno_t error;
	string timestampA;
	chrono::system_clock::time_point today = chrono::system_clock::now();
	time_t tt = chrono::system_clock::to_time_t(today);
	error = ctime_s(buffer, 26, &tt);
	for (int ii = 0; ii < 26; ii++)
	{
		if (buffer[ii] == '\0') { break; }
		else { timestampA.push_back(buffer[ii]); }
	}
	return timestampA;
}

// For a given function name, will retrieve the most current Windows error code and log 
// the code's English description in a text file. "err" will also terminate the application.
void sqlerr(wstring func, int error)
{
	mtcout16(func + L" error# " + to_wstring(error));
}
void err(wstring func)
{
	merror.lock();
	DWORD num = GetLastError();
	LPWSTR buffer = new WCHAR[512];
	wstring mod = L"wininet.dll";
	LPCWSTR modul = mod.c_str();
	DWORD buffer_length = FormatMessageW((FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE), GetModuleHandleW(modul), num, 0, buffer, 512, NULL);
	DWORD a = GetLastError();
	wstring message(buffer, 512);
	delete[] buffer;
	string temp1 = timestamperA();
	wstring temp2 = func + L" caused error " + to_wstring(num) + L": " + message;
	ERR << temp1 << " " << utf16to8(temp2) << endl;
	merror.unlock();
	exit(EXIT_FAILURE);
}
void warn(wstring func)
{
	merror.lock();
	DWORD num = GetLastError();
	LPWSTR buffer = new WCHAR[512];
	DWORD buffer_length = FormatMessageW((FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER), NULL, num, LANG_SYSTEM_DEFAULT, buffer, 0, NULL);
	wstring message(buffer, 512);
	delete[] buffer;
	string temp1 = timestamperA();
	wstring temp2 = func + L" caused error " + to_wstring(num) + L": " + message;
	ERR << temp1 << " " << utf16to8(temp2) << endl;
	merror.unlock();
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

// Read into memory a local file.
wstring bin_memory(HANDLE& hfile)
{
	DWORD size = GetFileSize(hfile, NULL);
	DWORD bytes_read;
	LPWSTR buffer = new WCHAR[size / 2];
	if (!ReadFile(hfile, buffer, size, &bytes_read, NULL)) { err(L"ReadFile-bin_memory"); }
	wstring bin(buffer, size / 2);
	delete[] buffer;
	return bin;
}
wstring bin_memory_nospaces(HANDLE& hfile)
{
	DWORD size = GetFileSize(hfile, NULL);
	DWORD bytes_read;
	LPWSTR buffer = new WCHAR[size / 2];
	if (!ReadFile(hfile, buffer, size, &bytes_read, NULL)) { err(L"ReadFile-bin_memory"); }
	wstring bin(buffer, size / 2);
	delete[] buffer;
	for (int ii = 0; ii < bin.size(); ii++)
	{
		if (bin[ii] == L' ') { bin[ii] = L'_'; }
	}
	return bin;
}

// Given a full path name, delete the file/folder.
void delete_file(wstring filename)
{
	HANDLE hfile = CreateFileW(filename.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile == INVALID_HANDLE_VALUE) { warn(L"CreateFile-delete_file"); }
	if (!DeleteFileW(filename.c_str())) { warn(L"DeleteFile-delete_file"); }
	if (!CloseHandle(hfile)) { warn(L"CloseHandle-delete_file"); }
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
	if (yesno) { wcout << L"Succeeded in deleting folder " << folder_name << endl; }
	else { wcout << L"Failed to delete folder " << folder_name << endl; }
}

// Given a root folder, return a vector containing the full paths of all subfolders within.
vector<wstring> get_subfolders(wstring root_folder)
{
	vector<wstring> subfolders;
	wstring folder_name;
	wstring folder_search = root_folder + L"\\*";
	size_t pos1;
	WIN32_FIND_DATAW info;
	HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
	if (hfile1 == INVALID_HANDLE_VALUE) { err(L"FindFirstFile-get_subfolders"); }
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
	if (!FindClose(hfile1)) { warn(L"FindClose-get_subfolders"); }

	for (int ii = subfolders.size() - 1; ii >= 0; ii--)
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

	for (int ii = 0; ii < subfolders1.size(); ii++)
	{
		subfolders_temp = get_subfolders(subfolders1[ii]);
		subfolders2.push_back(subfolders_temp);
	}

	return subfolders2;
}

// Query the user, and return the full path of the chosen catalogue folder.
wstring list_maker(vector<wstring> list, int number_of_columns, int spaces_per_entry)
{
	int size;
	int width;
	int count = 0;
	wstring output;
	for (int ii = 0; ii < list.size(); ii++)
	{
		if (count < number_of_columns)
		{
			output.append(to_wstring(ii + 1));
			output.append(L")  ");
			output.append(list[ii]);
			size = list[ii].size();
			for (int jj = 0; jj < (spaces_per_entry - size); jj++)
			{
				output.push_back(L' ');
			}
			count++;
		}
		else
		{
			output.append(L"\r\n");
			output.append(to_wstring(ii + 1));
			output.append(L")  ");
			output.append(list[ii]);
			size = list[ii].size();
			for (int jj = 0; jj < (spaces_per_entry - size); jj++)
			{
				output.push_back(L' ');
			}
			count = 1;
		}
	}
	return output;
}
wstring query_catalogue(wstring root_folder)
{
	vector<wstring> year_folders = get_subfolders(root_folder);
	vector<wstring> years;
	size_t pos1;
	wstring list, chosen_number, chosen_year, temp1;
	int NUMBER;

	for (int ii = 0; ii < year_folders.size(); ii++)
	{
		pos1 = year_folders[ii].rfind(L'\\');
		temp1 = year_folders[ii].substr(pos1 + 1);
		years.push_back(temp1);
	}
	list = list_maker(years, 3, 20);
	wcout << L"From the listed years, choose a number." << endl << endl;
	wcout << list << endl << endl;
	wcin >> chosen_number;
	try
	{
		NUMBER = stoi(chosen_number);
	}
	catch (invalid_argument& ia)
	{
		wcout << L"Invalid value for the year. Press any key, then try again.";
		system("pause");
		exit(EXIT_FAILURE);
	}
	chosen_year = year_folders[NUMBER - 1];

	vector<wstring> cata_folders = get_subfolders(chosen_year);
	vector<wstring> catalogues;
	wstring chosen_catalogue;
	for (int ii = 0; ii < cata_folders.size(); ii++)
	{
		pos1 = cata_folders[ii].rfind(L'\\');
		temp1 = cata_folders[ii].substr(pos1 + 1);
		catalogues.push_back(temp1);
	}
	list = list_maker(catalogues, 3, 20);
	wcout << L"From the listed catalogues, choose a number." << endl << endl;
	wcout << list << endl << endl;
	wcin >> chosen_number;
	try
	{
		NUMBER = stoi(chosen_number);
	}
	catch (invalid_argument& ia)
	{
		wcout << L"Invalid value for the catalogue. Press any key, then try again.";
		system("pause");
		exit(EXIT_FAILURE);
	}
	chosen_catalogue = cata_folders[NUMBER - 1];
	return chosen_catalogue;
}

int callback(void* NotUsed, int argc, char** argv, char** column_name)
{
	for (int ii = 0; ii < argc; ii++)
	{
		mtcout8(column_name[ii]);
		mtcout8(argv[ii]);
	}
	return 0;
}

// Contains pre-programmed responses to certain automated events during server-client communications.
void CALLBACK call(HINTERNET hint, DWORD_PTR dw_context, DWORD dwInternetStatus, LPVOID status_info, DWORD status_info_length)
{
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
		mcallback.lock();
		for (int ii = 0; ii < 512; ii++)
		{
			if ((cpContext->szMemo)[ii] != '\0') { temp.push_back((cpContext->szMemo)[ii]); }
		}
		mcallback.unlock();
		for (int ii = 0; ii < temp.size(); ii++)
		{
			if (temp[ii] == '/') { yesno = 1; }
			else if (temp[ii] < 0)
			{
				for (int jj = 0; jj < thread_list.size(); jj++)
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
	for (int ii = 0; ii < domains.size(); ii++)
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
	int size1, size2;
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
			for (int ii = 0; ii < bytes_available; ii++)
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
	DWORD file_size = fileW.size() * 2;
	if (!WriteFile(hprinter, fileW.c_str(), file_size, &bytes_written, NULL)) { warn(L"WriteFile"); return 7; }

	if (hrequest) { InternetCloseHandle(hrequest); }
	if (hconnect) { InternetCloseHandle(hconnect); }
	if (hint) { InternetCloseHandle(hint); }
	return 0;
}

class CSV_DB {
	wstring wfile;
	string cata_number;
	int GID;
	vector<vector<string>> variables;  // Form [variable][variable type, variable shown in CSV]
	vector<string> columns;
	vector<bool> is_int;
	vector<int> int_val;
	vector<double> double_val;
	bool making_the_model;
public:
	CSV_DB(wstring& wf, int gid, string cn, vector<bool> is_integer) { wfile = wf; GID = gid; cata_number = cn; is_int = is_integer; making_the_model = 0; }
	CSV_DB(wstring& wf, int gid, string cn) { wfile = wf; GID = gid; cata_number = cn; making_the_model = 1; }
	~CSV_DB() {}
	int read();
	vector<vector<string>> get_variables() { return variables; }
	vector<string> get_columns() { return columns; }
	vector<string> get_varicols();
	vector<bool> get_value_types() { return is_int; }
	string get_insert();
	int bad_apple(wstring);
};
int CSV_DB::read()
{
	size_t posnl1, posnl2, pos1, pos2, pos3;
	wstring line, temp1, temp2;
	vector<wstring> sub_columns;
	vector<wstring> temp_columns;
	vector<wstring> temp_headings = { L"\0" };
	wchar_t math;
	int spaces = 0;
	int indentation = 0;

	// Populate the variables.
	pos1 = 0;
	pos2 = 0;
	while (1)  
	{
		pos1 = wfile.find(L'=', pos1 + 1);
		if (pos1 > wfile.size()) { break; }
		math = wfile[pos1 - 1];
		if (math == L'<' || math == L'>')
		{
			continue;
		}
		variables.push_back(vector<string>());

		pos2 = wfile.rfind(L'"', pos1); pos2++;
		temp1 = wfile.substr(pos2, pos1 - pos2);
		clean(temp1, 0);
		temp2 = L"\"" + temp1 + L"\"";
		variables[variables.size() - 1].push_back(utf16to8(temp2));

		pos2 = wfile.find(L'"', pos1);
		temp1 = wfile.substr(pos1 + 1, pos2 - pos1 - 1);
		if (temp1.size() == 0) { sqlerr(L"csvread - GID " + to_wstring(GID), -1); return 1; }
		clean(temp1, 1);
		temp2 = L"\'" + temp1 + L"\'";
		variables[variables.size() - 1].push_back(utf16to8(temp2));
	}

	// Populate the columns and values.
	bool subtable = 0;
	int column_count = 0;
	posnl2 = wfile.find(L'\n', pos2);  // Start from the closing quotation mark of the last variable.
	while (1) // For every line in the csv file...
	{
		posnl1 = posnl2;
		posnl2 = wfile.find(L'\n', posnl1 + 1);
		if (posnl2 < wfile.size() - 1) { line = wfile.substr(posnl1, posnl2 - posnl1); } 
		else { break; }  // Either make the wstring "line" for this line, or break if EoF.

		pos1 = 0;
		while (1) // For every set of quotation marks on this line...
		{
			pos1 = line.find(L'"', pos1 + 1);
			if (pos1 < line.size()) 
			{
				pos2 = line.find(L'"', pos1 + 1);
				temp1 = line.substr(pos1 + 1, pos2 - pos1 - 1);
				spaces = clean(temp1, 0);
				temp_columns.push_back(temp1);
				pos1 = pos2;
			}
			else { break; }
		}

		if (temp_columns.size() > 1) 
		{
			subtable = 1;
			sub_columns = temp_columns;
			temp_columns.clear();
			continue;
		}
		else if (temp_columns[0] == L"Note") 
		{
			break;
		}
		else
		{
			if (spaces == indentation) 
			{
				temp_headings.back() = temp_columns[0];
			}
			else if (spaces > indentation)
			{
				indentation = spaces;
				temp_headings.push_back(temp_columns[0]);
			}
			else if (spaces == 0)
			{
				indentation = spaces;
				temp_headings.clear();
				temp_headings.push_back(temp_columns[0]);
			}
			else
			{
				indentation = spaces;
				temp_headings.pop_back();
				temp_headings.back() = temp_columns[0];
			}
			temp_columns.clear();

			for (int ii = 0; ii < temp_headings.size(); ii++)
			{
				if (ii == 0) { temp1 = temp_headings[ii]; }
				else
				{
					temp1 += L" + ";
					temp1 += temp_headings[ii];
				}
			}

			if (subtable)
			{
				for (int ii = 1; ii < sub_columns.size(); ii++)
				{
					temp2 = L"\"" + temp1 + L" + " + sub_columns[ii] + L"\"";
					columns.push_back(utf16to8(temp2));
				}

				pos1 = 0;
				for (int ii = 0; ii < sub_columns.size() - 1; ii++)
				{
					pos1 = line.find(L',', pos1 + 1); pos1++;
					pos2 = line.find(L',', pos1 + 1);
					temp1 = line.substr(pos1, pos2 - pos1);

					// Some Stats Canada spreadsheets have these "unknown" placeholders for values...
					if (temp1 == L"..")
					{
						if (making_the_model)
						{
							return 2;
						}
						else
						{
							if (is_int[column_count])
							{
								temp1 = L"0";
							}
							else
							{
								temp1 = L"0.0";
							}
							column_count++;
						}
					}
					else
					{
						if (is_int[column_count])
						{
							try { int_val.push_back(stoi(temp1)); }
							catch (invalid_argument& ia) { sqlerr(L"ia.what()", 0); }
						}
						else
						{
							try { double_val.push_back(stod(temp1)); }
							catch (invalid_argument& ia) { sqlerr(L"ia.what()", 0); }
						}
						column_count++;
					}
				}
			}
			else
			{
				temp2 = L"\"" + temp1 + L"\"";
				columns.push_back(utf16to8(temp2));

				pos1 = line.rfind(L',', line.size() - 2); pos1++;
				pos2 = line.find(L'\r', pos1);
				temp1 = line.substr(pos1, pos2 - pos1);

				// Some Stats Canada spreadsheets have these ".." placeholders for unknown values...
				if (temp1 == L"..")
				{
					if (making_the_model)
					{
						return 2;
					}
					else
					{
						if (is_int[column_count])
						{
							temp1 = L"0";
							try { int_val.push_back(stoi(temp1)); }
							catch (invalid_argument& ia) { sqlerr(L"ia.what()", 0); }
						}
						else
						{
							temp1 = L"0.0";
							try { double_val.push_back(stod(temp1)); }
							catch (invalid_argument& ia) { sqlerr(L"ia.what()", 0); }
						}
						column_count++;
					}
				}
				else if (making_the_model)
				{
					pos1 = temp1.find(L'.', 0);
					if (pos1 < temp1.size())
					{
						is_int.push_back(0);
						try { double_val.push_back(stod(temp1)); }
						catch (invalid_argument& ia) { sqlerr(L"ia.what()", 0); }
					}
					else
					{
						is_int.push_back(1);
						try { int_val.push_back(stoi(temp1)); }
						catch (invalid_argument& ia) { sqlerr(L"ia.what()", 0); }
					}
				}
				else
				{
					if (is_int[column_count])
					{
						try { int_val.push_back(stoi(temp1)); }
						catch (invalid_argument& ia) { sqlerr(L"ia.what()", 0); }
					}
					else
					{
						try { double_val.push_back(stod(temp1)); }
						catch (invalid_argument& ia) { sqlerr(L"ia.what()", 0); }
					}
					column_count++;
				}
			}
		}
	}
	return 0;
}
vector<string> CSV_DB::get_varicols()
{
	vector<string> varicols = columns;
	vector<string>::iterator it;
	for (int ii = variables.size() - 1; ii >= 0; ii--)
	{
		it = varicols.begin();
		varicols.insert(it, variables[ii][0]);
	}
	return varicols;
}
string CSV_DB::get_insert()
{
	string statement = "INSERT INTO \"T" + cata_number + "\" ( GID, ";
	for (int ii = 0; ii < variables.size(); ii++)
	{
		statement += variables[ii][0];
		statement += ", ";
	}
	for (int ii = 0; ii < columns.size(); ii++)
	{
		statement += columns[ii];
		if (ii < columns.size() - 1) { statement += ", "; }
	}
	statement += ") VALUES (" + to_string(GID) + ", ";

	int int_counter = 0;
	int double_counter = 0;
	for (int ii = 0; ii < variables.size(); ii++)
	{
		statement += variables[ii][1] + ", ";
	}
	for (int ii = 0; ii < columns.size(); ii++)
	{
		if (is_int[ii])
		{
			statement += to_string(int_val[int_counter]);
			int_counter++;
		}
		else
		{
			statement += to_string(double_val[double_counter]);
			double_counter++;
		}

		if (ii < columns.size() - 1)
		{
			statement += ", ";
		}
	}	
	statement += " ) ";

	statement += "ON CONFLICT ( GID ) WHERE true DO NOTHING ";

	statement += ";";

	return statement;
}
int CSV_DB::bad_apple(wstring apple_full_name)
{
	// wstring apple_full_name = cata_folder + L"\\" + utf8to16(cata_number) + L" (";
	// apple_full_name += to_wstring(GID) + L") " + utf8to16(geo_name) + L".csv";

	delete_file(apple_full_name);

	size_t pos1 = apple_full_name.rfind(L'\\', apple_full_name.size() - 1);
	wstring folder = apple_full_name.substr(0, pos1);
	wstring short_name = apple_full_name.substr(pos1 + 1);
	size_t pos2 = apple_full_name.rfind(L'\\', pos1 - 1);
	wstring archive = apple_full_name.substr(0, pos2);
	wstring temp1 = archive.substr(archive.size() - 4, 4);
	archive += L"\\" + temp1 + L" archive.bin";
	HANDLE hfile = CreateFileW(archive.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile == INVALID_HANDLE_VALUE) { warn(L"CreateFile-bad_apple"); return 1; }
	wstring wfile = bin_memory(hfile);
	if (!CloseHandle(hfile)) { warn(L"CloseHandle-bad_apple"); return 2; }

	wstring cata16 = utf8to16(cata_number);
	pos1 = wfile.find(cata16, 0);
	pos1 = wfile.find(L' ', pos1);
	pos1++;
	pos2 = wfile.find(L"||", pos1);
	pos2--;
	wstring url = wfile.substr(pos1, pos2 - pos1);
	pos1 = url.find(L"GID=", 0);
	pos1 += 4;
	pos2 = url.find(L'&', pos1);
	url.replace(pos1, pos2 - pos1, to_wstring(GID));

	int error = download(url, folder, short_name);
	if (error) { warn(L"download-bad_apple"); return 3; }

	return 0;
}

class CATA_DB {
	vector<string> column_header;
	vector<bool> is_int;  //For each column, is the value an integer?
	string cata_number;
	string cata_name;
	string table_name;
	wstring folder;
	wstring insert;
	vector<vector<string>> variables;
	sqlite3_stmt* statement;
	vector<vector<string>> out;
	int col_width;
public:
	CATA_DB(wstring folder_path) { init(folder_path); }
	~CATA_DB() {}
	void init(wstring);
	string get_name() { return cata_name; }
	string get_number() { return cata_number; }
	vector<string> get_columns() { return column_header; }
	void populate(sqlite3*&);
	void close(sqlite3*&);
	void prepareTABLE(sqlite3*&);
	int prepareINSERT(sqlite3*&, string, string&);
	void prepareSELECTcolumn(sqlite3*&, string);
	void step();
	void print_out();
	void update_width();
};
void CATA_DB::init(wstring folder_path)
{
	folder = folder_path;
	size_t pos1, pos2;
	wstring temp1, temp2;
	col_width = 0;

	wstring folder_search = folder + L"\\*.csv";
	WIN32_FIND_DATAW info;
	HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
	if (hfile1 == INVALID_HANDLE_VALUE) { err(L"FindFirstFile-init"); }
	wstring sample_name = folder + L"\\" + info.cFileName;
	HANDLE hfile2 = CreateFileW(sample_name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile2 == INVALID_HANDLE_VALUE) { err(L"CreateFile-init"); }
	
	wstring wfile = bin_memory(hfile2);	
	pos1 = wfile.rfind(L"Catalogue Number ", wfile.size() - 4);
	pos1 += 17;
	pos2 = wfile.find(L'.', pos1);
	temp1 = wfile.substr(pos1, pos2 - pos1);
	cata_number = utf16to8(temp1);
	table_name = "T" + cata_number;

	pos1 = wfile.find(L"\"", 0); pos1++;
	pos2 = wfile.find(L"\"", pos1);
	temp1 = wfile.substr(pos1, pos2 - pos1);
	pos1 = 0;
	do
	{
		pos1++;
		pos1 = temp1.find(L'(', pos1);
		if (pos1 < temp1.size())
		{
			pos2 = temp1.find(L')', pos1);
			temp1.erase(pos1, pos2 - pos1 + 1);
		}
	} while (pos1 < temp1.size());
	cata_name = utf16to8(temp1);

	CSV_DB model(wfile, -1, cata_name);
	int error = model.read();
	if (error > 0)
	{
		if (error == 2)
		{
			while (FindNextFileW(hfile1, &info))
			{
				sample_name = folder + L"\\" + info.cFileName;
				hfile2 = CreateFileW(sample_name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hfile2 == INVALID_HANDLE_VALUE) { err(L"CreateFile-init(bogus model-making)"); }
				
				wfile = bin_memory(hfile2);
				pos1 = wfile.rfind(L"Catalogue Number ", wfile.size() - 4);
				pos1 += 17;
				pos2 = wfile.find(L'.', pos1);
				temp1 = wfile.substr(pos1, pos2 - pos1);
				cata_number = utf16to8(temp1);
				table_name = "T" + cata_number;

				pos1 = wfile.find(L"\"", 0); pos1++;
				pos2 = wfile.find(L"\"", pos1);
				temp1 = wfile.substr(pos1, pos2 - pos1);
				pos1 = 0;
				do
				{
					pos1++;
					pos1 = temp1.find(L'(', pos1);
					if (pos1 < temp1.size())
					{
						pos2 = temp1.find(L')', pos1);
						temp1.erase(pos1, pos2 - pos1 + 1);
					}
				} while (pos1 < temp1.size());
				cata_name = utf16to8(temp1);

				CSV_DB model(wfile, -1, cata_name);
				error = model.read();
				if (error == 0) { break; }
			}
		}
	}
	variables = model.get_variables();
	column_header = model.get_columns();
	is_int = model.get_value_types();

	if (!FindClose(hfile1)) { warn(L"FindClose-init"); }
	if (!CloseHandle(hfile2)) { warn(L"CloseHandle-init"); }
}
void CATA_DB::step()
{
	int type, col_count, size;  // Type: 1(int), 2(double), 3(string)
	int error = sqlite3_step(statement);
	int ivalue;
	double dvalue;
	string svalue;

	while (error == 100)
	{
		col_count = sqlite3_column_count(statement);
		out.push_back(vector<string>(col_count));
		for (int ii = 0; ii < col_count; ii++)
		{
			type = sqlite3_column_type(statement, ii);
			switch (type)
			{
			case 1:
				ivalue = sqlite3_column_int(statement, ii);
				out[out.size() - 1][ii] = to_string(ivalue);
				break;
			case 2:
				dvalue = sqlite3_column_double(statement, ii);
				out[out.size() - 1][ii] = to_string(dvalue);
				break;
			case 3:
				size = sqlite3_column_bytes(statement, ii);
				char* buffer = (char*)sqlite3_column_text(statement, ii);
				svalue.assign(buffer, size);
				out[out.size() - 1][ii] = svalue;
				break;
			}
		}
		error = sqlite3_step(statement);
	}
	if (error > 0 && error != 101) { sqlerr(L"step", error); cin.get(); }
}
void CATA_DB::populate(sqlite3*& database)
{
	wstring folder_search = folder + L"\\*.csv";
	WIN32_FIND_DATAW info;
	HANDLE hfile1 = FindFirstFileW(folder_search.c_str(), &info);
	if (hfile1 == INVALID_HANDLE_VALUE) { err(L"FindFirstFile-populate"); }
	HANDLE hfile2, hfile3;
	wstring short_name, full_name, wfile, temp1, sql;
	size_t pos1, pos2;
	int GID, error;
	bool second_chance = 0;

	do
	{
		short_name = info.cFileName;
		pos1 = short_name.find(L"(", 0); pos1++;
		pos2 = short_name.find(L")", pos1);
		temp1 = short_name.substr(pos1, pos2 - pos1);
		try { GID = stoi(temp1); }
		catch (invalid_argument& ia) { sqlerr(L"ia.what()", 0); }

		pos1 = short_name.find(L')', 0);
		pos1 += 2;
		pos2 = short_name.rfind(L'.', short_name.size() - 1);
		temp1 = short_name.substr(pos1, pos2 - pos1);
		
		full_name = folder + L"\\" + short_name;
		hfile2 = CreateFileW(full_name.c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hfile2 == INVALID_HANDLE_VALUE) { err(L"CreateFile-populate"); }
		wfile = bin_memory(hfile2);
		if (!CloseHandle(hfile2)) { warn(L"CloseHandle-populate"); }

		CSV_DB cdb(wfile, GID, cata_number, is_int);
		error = cdb.read();
		if (error > 0) 
		{
			ERR << "GID " << GID << " failed to read." << endl;
			continue;
		}
		string error_msg;
		string sql;

		sql = cdb.get_insert();
		error = prepareINSERT(database, sql, error_msg);
		if (error == 1) 
		{
			if (cdb.bad_apple(full_name)) 
			{ 
				mtcout8(error_msg); cin.get();
			}
			else
			{
				hfile3 = CreateFileW(full_name.c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_DELETE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hfile3 == INVALID_HANDLE_VALUE) { err(L"CreateFile-populate"); }
				wfile = bin_memory(hfile3);
				if (!CloseHandle(hfile3)) { warn(L"CloseHandle-populate"); }
				CSV_DB cdb2(wfile, GID, cata_number);
				error = cdb2.read();
				if (error > 0)
				{
					ERR << "GID " << GID << " failed to read." << endl;
					continue;
				}
				sql = cdb2.get_insert();
				sqlite3_finalize(statement);
				error = prepareINSERT(database, sql, error_msg);
			}
		}
		step();
	} while (FindNextFileW(hfile1, &info));
} 
void CATA_DB::close(sqlite3*& database)
{
	int error = 0;
	error = sqlite3_close_v2(database);
	if (error) { sqlerr(L"close", error); cin.get(); }
}
void CATA_DB::prepareTABLE(sqlite3*& database)
{
	string sql = "CREATE TABLE IF NOT EXISTS \"" + table_name + "\" ( GID INTEGER PRIMARY KEY, ";
	for (int ii = 0; ii < variables.size(); ii++)
	{
		sql += variables[ii][0];
		sql += " TEXT, ";
	}
	for (int ii = 0; ii < column_header.size(); ii++)
	{
		sql += column_header[ii];
		if (is_int[ii]) { sql += " INTEGER, "; }
		else { sql += " REAL, "; }
	}
	sql.pop_back(); sql.pop_back();
	sql.append(" );");

	char* errormsg = 0;
	int error = sqlite3_prepare_v2(database, sql.c_str(), -1, &statement, NULL);
	if (error) 
	{
		sqlerr(L"prepareTABLE", error);
		error = sqlite3_exec(database, sql.c_str(), callback, NULL, &errormsg);
		system("pause");
	}
	out.push_back(vector<string>());
	step();
}
int CATA_DB::prepareINSERT(sqlite3*& database, string sql, string& error_msg)
{
	char* errormsg = 0;
	int count = 0;
	int error = sqlite3_prepare_v2(database, sql.c_str(), -1, &statement, NULL);
	if (error) 
	{
		sqlerr(L"prepareINSERT", error);
		error = sqlite3_exec(database, sql.c_str(), callback, NULL, &errormsg);
		while (errormsg[count] != '\0')
		{
			error_msg.push_back(errormsg[count]);
			count++;
		}
		return 1;
	}
	return 0;
}
void CATA_DB::prepareSELECTcolumn(sqlite3*& database, string col)
{
	char* errormsg = 0;
	string sql = "SELECT \"Geography\", \"" + col + "\" FROM \"" + table_name + "\" ;";
	int error = sqlite3_prepare_v2(database, sql.c_str(), -1, &statement, NULL);
	if (error) 
	{
		sqlerr(L"prepareSELECTcolumn", error);
		error = sqlite3_exec(database, sql.c_str(), callback, NULL, &errormsg);
		cin.get();
	}
	out[0] = { "Geography", col };
	update_width();
}
void CATA_DB::print_out()
{
	mpo.lock();
	cout << endl;
	for (int ii = 0; ii < out.size(); ii++)
	{
		cout << setw(1) << "|";
		for (int jj = 0; jj < out[ii].size(); jj++)
		{
			cout << setw(col_width) << out[ii][jj];
			cout << setw(1) << " | ";
		}
		cout << endl;
		if (ii == 0)
		{
			cout << setw(1) << "--";
			for (int jj = 0; jj < out[ii].size(); jj++)
			{
				for (int kk = 0; kk < col_width + 2; kk++) { cout << "-"; }
			}
			cout << endl;
		}
	} 
	mpo.unlock();
}
void CATA_DB::update_width()
{
	for (int ii = 0; ii < out[0].size(); ii++)
	{
		if (out[0][ii].size() > col_width) { col_width = out[0][ii].size(); }
	}
}

// Creates the database file, and populates the subfolders vector with the full paths 
// for all catalogue folders present for the given year. Graveyard folder is ignored. 
sqlite3* createDB(wstring root_folder)
{
	sqlite3* db;
	size_t pos1;
	wstring temp1;

	temp1 = root_folder + L"\\database.db";
	string filename = utf16to8(temp1);
	int error = 0;
	error = sqlite3_open_v2(filename.c_str(), &db, (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE), NULL);
	if (error) { sqlerr(L"SQLite_open-createDB", error); }
	else { mtcout16(L"Database successfully created."); }

	return db;
}

// Given a database file and a folder containing CSV files, make a table for that catalogue. 
CATA_DB tablemaker(sqlite3*& database, wstring folder_path)
{
	CATA_DB cata(folder_path);
	cata.prepareTABLE(database);
	cata.populate(database);
	//cata.prepareSELECTcolumn(database, "Total population by sex and age");
	//cata.step();
	//cata.print_out();
	return cata;
}

// Given a root folder, build the database using all CSVs in all subfolders within.
vector<vector<CATA_DB>> initialize(sqlite3*& database, wstring root_folder)
{
	vector<vector<wstring>> folder_tree = get_subfolders2(root_folder);
	vector<vector<CATA_DB>> cata_tree;
	wstring temp1;

	for (int ii = 0; ii < folder_tree.size(); ii++)
	{
		cata_tree.push_back(vector<CATA_DB>());
		for (int jj = 0; jj < folder_tree[ii].size(); jj++)
		{
			cata_tree[ii].push_back(tablemaker(database, folder_tree[ii][jj]));
			temp1 = folder_tree[ii][jj] + L" added to database.";
			mtcout16(temp1);
		}
	}
	return cata_tree;
}

// Query the user for a single catalogue, then build a table and extract data from it.
vector<vector<CATA_DB>> init_debug(sqlite3*& database, wstring cata_folder)
{
	vector<vector<CATA_DB>> cata_tree(1, vector<CATA_DB>());
	cata_tree[0].push_back(tablemaker(database, cata_folder));
	return cata_tree;
}

int main()
{
	setlocale(LC_ALL, ".UTF8");
	sqlite3* database = createDB(local_directory);
	vector<vector<CATA_DB>> catalogue_tree;
	wstring catalogue_path;
	if (debug)
	{
		catalogue_path = query_catalogue(local_directory);
		catalogue_tree = init_debug(database, catalogue_path);
	}
	else
	{
		catalogue_tree = initialize(database, local_directory);
	}
	system("pause");
	return 0;
}

/* void CATA_DB::select(string query)
{
	int error = 0;
	string sql = "SELECT " + query + " FROM T" + cata_number;
	char* errormsg = 0;
	error = sqlite3_exec(database, sql.c_str(), callback, NULL, &errormsg);
	if (error) { sqlerr(L"select", error); }
} */

/* void CATA_DB::createTABLE()
{
	string sql = "CREATE TABLE IF NOT EXISTS T" + cata_number + " ( GID INT PRIMARY KEY, ";
	for (int ii = 0; ii < variables.size(); ii++)
	{
		sql += variables[ii][0];
		sql += " TEXT, ";
	}
	for (int ii = 0; ii < column_header.size(); ii++)
	{
		sql += column_header[ii];
		if (is_int[ii]) { sql += " INT, "; }
		else { sql += " REAL, "; }
	}
	sql.pop_back(); sql.pop_back();
	sql.append(" );");

	int error = 0;
	char* errormsg = 0;
	error = sqlite3_exec(database, sql.c_str(), callback, NULL, &errormsg);
	if (error) { sqlerr(L"createTABLE", error); }
}
*/