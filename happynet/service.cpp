#include <comdef.h>
#include <stdio.h>
#include <windows.h>
#include "atlbase.h"
#include "atlstr.h"

#include "netinterface.h"
#include "service.h"
#include "registry.h"
#include "process.h"
#include "systemsrv.h"
#include "utils.h"

HANDLE m_pid;

DWORD get_service_status(VOID)
{
	//if( STILL_ACTIVE == dwMark) //running
	//if( PROCESS_EXIT_CODE == dwMark) //stopped
    if (is_system_service()) {
        return get_service_system_status();
    }
	return get_service_process_status();
}

VOID start_service(VOID)
{
    if (is_system_service()) {
        start_service_system();
        return;
    }

    WCHAR dir_path[MAX_PATH] = { 0 };
    WCHAR command_line[MAX_COMMAND_LINE_LEN] = { 0 };
	if (get_service_status() == STILL_ACTIVE) {
		return;
	}

	// Build path and command line parameters
	if (!get_install_dir_path(dir_path, MAX_PATH))
	{
		log_event(TEXT("%s:%d (%s) - Error building executable path.\n"),
                    __FILEW__, __LINE__, __FUNCTIONW__);
		return;
	}
	INT ret = 0;

	ret = get_command_line_edge(dir_path, command_line, MAX_COMMAND_LINE_LEN);

	log_event(TEXT("%s:%d (%s) - building command line: %s \n"),
                __FILEW__, __LINE__, __FUNCTIONW__, command_line);
	log_event(TEXT("\n->Start of parent execution.\n"));

	// Create the child process. 
	create_service_process(command_line); 
}

VOID stop_service(VOID)
{
    if (is_system_service()) {
        stop_service_system();
        return;
    }

	grace_stop_service_process();
	Sleep(1500);
	terminal_service_process();
}

// auto start exe when system startup
VOID set_auto_start_service(VOID)
{
    if (is_system_service()) {
        set_auto_start_service_system();
    }
    
    HKEY hkey;
	WCHAR ret_val[512];
	//std::string strRegPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

	//open system auto run regedit item
	if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS)
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
			RegSetValueEx(hkey, TEXT("Happynet"), 0, REG_SZ, (LPBYTE)str_exe_fulldir, (lstrlen(str_exe_fulldir) + 1)*sizeof(TCHAR));

			//close regedit
			RegCloseKey(hkey);
		}
	}
}


//cancle auto start
VOID cancel_auto_start_service(VOID)
{
    if (is_system_service()) {
        cancel_auto_start_service_system();
    }
    
    HKEY hkey;
	//std::string strRegPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

	if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS)
	{
		RegDeleteValue(hkey, TEXT("Happynet"));
		RegCloseKey(hkey);
	}
}
