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

DWORD GetHappynetServiceStatus(VOID)
{
	//if( STILL_ACTIVE == dwMark) //running
	//if( PROCESS_EXIT_CODE == dwMark) //stopped
    if (IsSetSystemService()) {
        return GetSystemServiceStatus();
    }
	return GetProcessServiceStatus();
}

VOID StartHappynetService(VOID)
{
    if (IsSetSystemService()) {
        // re reg system service args
        //UnregSystemService();
        RegSystemService();
        StartSystemService();
        return;
    }

    WCHAR dir_path[MAX_PATH] = { 0 };
    WCHAR command_line[MAX_COMMAND_LINE_LEN] = { 0 };
	if (GetHappynetServiceStatus() == STILL_ACTIVE) {
		return;
	}

	// Build path and command line parameters
	if (!GetInstallDirPath(dir_path, MAX_PATH))
	{
		LogEvent(TEXT("%s:%d (%s) - Error building executable path.\n"),
                    __FILEW__, __LINE__, __FUNCTIONW__);
		return;
	}
	INT ret = 0;

	ret = GetEdgeCmdLine(dir_path, command_line, MAX_COMMAND_LINE_LEN);

	LogEvent(TEXT("%s:%d (%s) - building command line: %s \n"),
                __FILEW__, __LINE__, __FUNCTIONW__, command_line);
	LogEvent(TEXT("\n->Start of parent execution.\n"));

	// Create the child process. 
	CreateProcessService(command_line); 
}

VOID StopHappynetService(VOID)
{
    if (IsSetSystemService()) {
        StopSystemService();
        Sleep(1500);
        return;
    }

	GraceStopProcessService();
	Sleep(1500);
	TerminalProcessService();
}

// auto start exe when system startup
VOID SetHappynetServiceAutoStart(VOID)
{
    if (IsSetSystemService()) {
        SetSystemServiceAutoStart();
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
		if (!GetRegString(hkey, L"Happynet", ret_val, 512) || _tcscmp(str_exe_fulldir, str_dir) != 0)
		{
			//append child Key and set value:"happynet" is exe name
			RegSetValueEx(hkey, TEXT("Happynet"), 0, REG_SZ, (LPBYTE)str_exe_fulldir, (lstrlen(str_exe_fulldir)+1)*sizeof(TCHAR));

			//close regedit
			RegCloseKey(hkey);
		}
	}
}


//cancle auto start
VOID UnsetHappynetServiceAutoStart(VOID)
{
    if (IsSetSystemService()) {
        UnsetSystemServiceAutoStart();
    }
    
    HKEY hkey;
	//std::string strRegPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

	if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS)
	{
		RegDeleteValue(hkey, TEXT("Happynet"));
		RegCloseKey(hkey);
	}
}
