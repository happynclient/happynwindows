#include <comdef.h>
#include <stdio.h>
#include <windows.h>
#include "atlbase.h"
#include "atlstr.h"

#include "netinterface.h"
#include "service.h"
#include "registry.h"
#include "process.h"
#include "utils.h"

HANDLE pid;

DWORD get_service_status()
{
	//if( STILL_ACTIVE == dwMark) //running
	//if( PROCESS_EXIT_CODE == dwMark) //stopped
	return get_service_process_status();
}

void start_service()
{
    WCHAR exe_path[MAX_PATH];
    WCHAR command_line[MAX_COMMAND_LINE_LEN];    
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

	ret = build_command_line_edge(exe_path, command_line, MAX_COMMAND_LINE_LEN);

	log_event(L"%s:%d (%s) - building command line: %s \n", __FILEW__, __LINE__, __FUNCTIONW__, command_line);

	log_event(L"\n->Start of parent execution.\n");


	// Create the child process. 
	create_service_process(command_line); 
}

void stop_service()
{
	grace_stop_service_process();
	Sleep(1500);
	terminal_service_process();
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
