#include <stdio.h>
#include <windows.h>
#include "atlbase.h"
#include "atlstr.h"

#include "service.h"
#include "registry.h"
#include "process.h"

HANDLE pid;
WCHAR exe_path[MAX_PATH];
WCHAR command_line[1024];


void log_event(WCHAR* format, ...)
{
	//WCHAR message[4096] = {'\0'};
	int n = 0;
	va_list arg;

	// Construct the message
	va_start(arg, format);
	int size = _vscwprintf(format, arg) + 1;
	wchar_t *message = new wchar_t[size];
	n = _vsnwprintf_s(message, size, size, format, arg);
	va_end(arg);

	// Check success
	if (n < 0 || n >= size) return;

	// Log the message
	OutputDebugStringW(message);
	delete [] message;
}


DWORD get_service_status()
{
	//if( STILL_ACTIVE == dwMark) //running
	//if( PROCESS_EXIT_CODE == dwMark) //stopped
	return get_service_process_status();
}

void start_service()
{
	if (get_service_status() == STILL_ACTIVE) {
		return;
	}

	// Build path and command line parameters
	if (!build_exe_path(exe_path, MAX_PATH))
	{
		log_event(L"%s:%d (%s) - Error building executable path.\n", __FILEW__, __LINE__, __FUNCTIONW__);
		return;
	}
	int ret = 0;

	ret = build_command_line_edge(exe_path, command_line, 1024);

	log_event(L"%s:%d (%s) - building command line: %s \n", __FILEW__, __LINE__, __FUNCTIONW__, command_line);

	log_event(L"\n->Start of parent execution.\n");


	// Create the child process. 
	create_service_process(command_line); 
}

void stop_service()
{
	grace_stop_service_process();
	Sleep(1000);
	terminal_service_process();
}


int build_exe_path(WCHAR* exe_path, int buf_len)
{
	DWORD exe_buf_len = buf_len * sizeof(WCHAR);

	WCHAR* ptr = command_line;
	swprintf_s(exe_path, buf_len, L"");
	return 1;
}


int build_command_line_edge(WCHAR* exe_path, WCHAR* command_line, int buf_len)
{
	command_line[0] = 0;
	WCHAR ret_val[512];
	DWORD ret_dword = 0;

	// Use 'ptr' to append to the end of the command line
	WCHAR* ptr = command_line;
	ptr += swprintf_s(command_line, buf_len, L"\"%shappynedge.exe\"", exe_path);

	// Open registry key
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Happynet\\Parameters", NULL, KEY_READ, &hkey) != ERROR_SUCCESS)
	{
		log_event(L"%s:%d (%s) - Error opening registry key.\n", __FILEW__, __LINE__, __FUNCTIONW__);
		return 0;
	}

	// Community
	if (!reg_get_string(hkey, L"community", ret_val, 512)) return 0;
	ptr += swprintf_s(ptr, buf_len - (ptr - command_line), L" -c %s", ret_val);

	// Encryption key
	if (!reg_get_string(hkey, L"enckey", ret_val, 512)) return 0;
	if (wcslen(ret_val) != 0)
	{
		ptr += swprintf_s(ptr, buf_len - (ptr - command_line), L" -k %s", ret_val);
	}

	// IP address
	if (!reg_get_string(hkey, L"ip_address", ret_val, 512)) return 0;
	if (!reg_get_dword(hkey, L"packet_forwarding", &ret_dword)) return 0;
	if (wcslen(ret_val) != 0 && ret_dword == 0)
	{
		ptr += swprintf_s(ptr, buf_len - (ptr - command_line), L" -a %s", ret_val);
	}
	
	if (wcslen(ret_val) == 0 && ret_dword == 0)
	{
		ptr += swprintf_s(ptr, buf_len - (ptr - command_line), L" -r -a dhcp:0.0.0.0");
	}

	// Encryption key file
	if (!reg_get_string(hkey, L"keyfile", ret_val, 512)) return 0;
	if (wcslen(ret_val) != 0)
	{
		ptr += swprintf_s(ptr, buf_len - (ptr - command_line), L" -K %s", ret_val);
	}

	// Local Port
	if (!reg_get_dword(hkey, L"local_port", &ret_dword)) return 0;
	if (ret_dword != 0)
	{
		ptr += swprintf_s(ptr, buf_len - (ptr - command_line), L" -p %d", ret_dword);
	}


	// MAC address
	if (!reg_get_string(hkey, L"mac_address", ret_val, 512)) return 0;
	if (wcslen(ret_val) != 0)
	{
		ptr += swprintf_s(ptr, buf_len - (ptr - command_line), L" -m %s", ret_val);
	}

	// MTU
	if (!reg_get_dword(hkey, L"mtu", &ret_dword)) return 0;
	if (ret_dword != 0)
	{
		ptr += swprintf_s(ptr, buf_len - (ptr - command_line), L" -M %d", ret_dword);
	}

	// Multicast
	if (!reg_get_dword(hkey, L"multicast", &ret_dword)) return 0;
	if (ret_dword != 0)
	{
		ptr += swprintf_s(ptr, buf_len - (ptr - command_line), L" -E");
	}

	// Packet forwarding
	if (!reg_get_dword(hkey, L"packet_forwarding", &ret_dword)) return 0;
	if (ret_dword != 0)
	{
		ptr += swprintf_s(ptr, buf_len - (ptr - command_line), L" -r");
	}

	// header encryption
	if (!reg_get_dword(hkey, L"header_encry", &ret_dword)) return 0;
	if (ret_dword != 0)
	{
		ptr += swprintf_s(ptr, buf_len - (ptr - command_line), L" -H");
	}

	// data compress
	if (!reg_get_dword(hkey, L"data_compress", &ret_dword)) return 0;
	if (ret_dword != 0)
	{
		ptr += swprintf_s(ptr, buf_len - (ptr - command_line), L" -z1");
	}

	// Supernode address
	if (!reg_get_string(hkey, L"supernode_addr", ret_val, 512)) return 0;
	ptr += swprintf_s(ptr, buf_len - (ptr - command_line), L" -l %s", ret_val);

	// Supernode port
	if (!reg_get_dword(hkey, L"supernode_port", &ret_dword)) return 0;
	ptr += swprintf_s(ptr, buf_len - (ptr - command_line), L":%d", ret_dword);

	//custom param
	if (!reg_get_string(hkey, L"custom_param", ret_val, 512)) return 0;
	ptr += swprintf_s(ptr, buf_len - (ptr - command_line), L" %s", ret_val);
	return 1;
}

// auto start exe when system startup
void set_auto_start_service()
{
	HKEY hkey;
	WCHAR ret_val[512];
	//std::string strRegPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

	//open system auto run regedit item
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS)
	{
		//get exe path
		TCHAR str_exe_fulldir[MAX_PATH];
		GetModuleFileName(NULL, str_exe_fulldir, MAX_PATH);

		//check if regedit item exists
		TCHAR str_dir[MAX_PATH] = {};
		DWORD nLength = MAX_PATH;
		
		//already exist
		if (!reg_get_string(hkey, L"Happynet", ret_val, 512) || _tcscmp(str_exe_fulldir, str_dir) != 0)
		{
			//append child Key and set value:"happynet" is exe name
			RegSetValueEx(hkey, _T("Happynet"), 0, REG_SZ, (LPBYTE)str_exe_fulldir, (lstrlen(str_exe_fulldir) + 1)*sizeof(TCHAR));

			//close regedit
			RegCloseKey(hkey);
		}
	}
}


//cancle auto start
void cancel_auto_start_service()
{
	HKEY hkey;
	//std::string strRegPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS)
	{
		RegDeleteValue(hkey, _T("Happynet"));
		RegCloseKey(hkey);
	}
}
