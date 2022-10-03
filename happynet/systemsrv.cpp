#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#include <Winsock.h>
#include "process.h"
#include "registry.h"
#include "systemsrv.h"
#include "utils.h"

static WCHAR* GetNssmExePath(VOID)
{
    static WCHAR szNssmPath[MAX_PATH] = { 0 };
    WCHAR szInstallPath[MAX_PATH] = { 0 };
    
    // Build path and command line parameters
    if (!GetInstallDirPath(szInstallPath, MAX_PATH))
    {
        LogEvent(TEXT("%s:%d (%s) - Error building executable path.\n"),
            __FILEW__, __LINE__, __FUNCTIONW__);
        return NULL;
    }
    swprintf_s(szNssmPath, MAX_PATH, TEXT("\"%s\\happynssm.exe\""), szInstallPath);
    return szNssmPath;
}

static WCHAR* GetNssmLogPath(VOID)
{
    static WCHAR szNssmLogPath[MAX_PATH] = { 0 };
    WCHAR szAppDataPath[MAX_PATH] = { 0 };

    // Build path and command line parameters
    GetAppDatapath(szAppDataPath);
    swprintf_s(szNssmLogPath, MAX_PATH, L"%s\\happynet.log", szAppDataPath);
    LogEvent(TEXT("%s:%d (%s) - building log path:%s \n"),
        __FILEW__, __LINE__, __FUNCTIONW__, szNssmLogPath);
    return szNssmLogPath;
}


// nssm install <servicename> <program> [<arguments>]
// nssm set <servicename> Description "Happynet is a light VPN software which makes it easy to create virtual networks bypassing intermediate firewalls. Powered by happyn.cn"
VOID RegSystemService(VOID)
{
    WCHAR szInstallPath[MAX_PATH] = { 0 };
    WCHAR szEdgePath[MAX_PATH] = { 0 };
    WCHAR szParamsLine[MAX_COMMAND_LINE_LEN] = { 0 };
    WCHAR szNssmComandLine[MAX_COMMAND_LINE_LEN] = { 0 };

    // Build path and command line parameters
    if (!GetInstallDirPath(szInstallPath, MAX_PATH))
    {
        LogEvent(TEXT("%s:%d (%s) - Error building executable path.\n"),
                    __FILEW__, __LINE__, __FUNCTIONW__);
        return;
    }
    swprintf_s(szEdgePath, MAX_PATH, TEXT("\"%s\\happynedge.exe\""), szInstallPath);
    INT ret = 0;

    GetEdgeParams(TEXT(" "), szParamsLine, MAX_COMMAND_LINE_LEN);
    
    // nssm install <servicename> <program>[<arguments>]
    swprintf_s(szNssmComandLine, MAX_COMMAND_LINE_LEN,
                TEXT("%s install %s %s \"%s\""),
                GetNssmExePath(), SYSTEMSRV_NAME, szEdgePath, szParamsLine);

    WinExecW(szNssmComandLine, SW_HIDE);

    //nssm set <servicename> Description "Happynet ..."
    swprintf_s(szNssmComandLine, MAX_COMMAND_LINE_LEN,
        TEXT("%s set %s Description \"Happynet is a light VPN software which makes it easy to create virtual networks bypassing intermediate firewalls. Powered by happyn.cn\""),
        GetNssmExePath(), SYSTEMSRV_NAME);
    WinExecW(szNssmComandLine, SW_HIDE);

    //nssm set <servicename> AppStdout logpath
    swprintf_s(szNssmComandLine, MAX_COMMAND_LINE_LEN,
        TEXT("%s set %s AppStdout %s"),
        GetNssmExePath(), SYSTEMSRV_NAME, GetNssmLogPath());
    WinExecW(szNssmComandLine, SW_HIDE);

    //nssm set <servicename> AppStderr logpath
    swprintf_s(szNssmComandLine, MAX_COMMAND_LINE_LEN,
        TEXT("%s set %s AppStderr %s"),
        GetNssmExePath(), SYSTEMSRV_NAME, GetNssmLogPath());
    WinExecW(szNssmComandLine, SW_HIDE);
}

// nssm remove <servicename>
VOID UnregSystemService(VOID)
{
    WCHAR nssm_path[MAX_PATH] = { 0 };
    WCHAR szNssmCommandLine[MAX_COMMAND_LINE_LEN] = { 0 };
    swprintf_s(szNssmCommandLine, MAX_COMMAND_LINE_LEN,
                TEXT("%s remove %s confirm"), GetNssmExePath(), SYSTEMSRV_NAME);

    StopSystemService();
    WinExecW(szNssmCommandLine, SW_HIDE);
}

// nssm set <servicename> Start SERVICE_AUTO_START
VOID SetSystemServiceAutoStart(VOID)
{
    WCHAR szNssmCommandLine[MAX_COMMAND_LINE_LEN] = { 0 };
    //nssm set <servicename> Start SERVICE_AUTO_START
    swprintf_s(szNssmCommandLine, MAX_COMMAND_LINE_LEN,
            TEXT("%s set %s Start SERVICE_AUTO_START"),
            GetNssmExePath(), SYSTEMSRV_NAME);
    WinExecW(szNssmCommandLine, SW_HIDE);
    return;
}

// nssm set <servicename> Start SERVICE_DEMAND_START
VOID UnsetSystemServiceAutoStart(VOID)
{
    WCHAR szNssmCommandLine[MAX_COMMAND_LINE_LEN] = { 0 };
    //nssm set <servicename> Start SERVICE_DEMAND_START
    swprintf_s(szNssmCommandLine, MAX_COMMAND_LINE_LEN,
        TEXT("%s set %s Start SERVICE_DEMAND_START"),
        GetNssmExePath(), SYSTEMSRV_NAME);
    WinExecW(szNssmCommandLine, SW_HIDE);
    return;
}

// nssm start <servicename>
VOID StartSystemService(VOID)
{
    WCHAR szNssmCommandLine[MAX_COMMAND_LINE_LEN] = { 0 };
    //nssm set <servicename> Start SERVICE_DEMAND_START
    swprintf_s(szNssmCommandLine, MAX_COMMAND_LINE_LEN,
        TEXT("%s start  %s"),
        GetNssmExePath(), SYSTEMSRV_NAME);
    WinExecW(szNssmCommandLine, SW_HIDE);
    return;
}

// nssm stop <servicename>
VOID StopSystemService(VOID)
{
    WCHAR szNssmCommandLine[MAX_COMMAND_LINE_LEN] = { 0 };
    //nssm stop <servicename>
    swprintf_s(szNssmCommandLine, MAX_COMMAND_LINE_LEN,
        TEXT("%s stop  %s"),
        GetNssmExePath(), SYSTEMSRV_NAME);
    WinExecW(szNssmCommandLine, SW_HIDE);
    return;
}

// nssm status <servicename>
// result:
// Can't open service!
// SERVICE_STOPPED
// SERVICE_RUNNING
DWORD GetSystemServiceStatus(VOID)
{
    //create pipe
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    HANDLE hRead, hWrite;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
    {
        return EXCEPTION_BREAKPOINT;
    }

    //set thread output to pipe
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    GetStartupInfo(&si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdError = hWrite;
    si.hStdOutput = hWrite;

    //start commandline
    WCHAR szNssmCommandLine[MAX_COMMAND_LINE_LEN] = { 0 };
    swprintf_s(szNssmCommandLine, MAX_COMMAND_LINE_LEN,
        TEXT("%s status %s"),
        GetNssmExePath(), SYSTEMSRV_NAME);
    PROCESS_INFORMATION pi;
    if (!CreateProcessW(NULL, szNssmCommandLine, NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
    {
        return EXCEPTION_BREAKPOINT;
    }

    //close hWrite
    CloseHandle(hWrite);

    DWORD dwRead;
    CHAR arrcStdoutBuf[PROCESS_STDOUT_BUFSIZE] = { 0 };
    BOOL bSuccess = FALSE;
    HANDLE hParentStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    bSuccess = ReadFile(hRead, arrcStdoutBuf, PROCESS_STDOUT_BUFSIZE, &dwRead, NULL);
    if (!bSuccess || dwRead == 0) return EXCEPTION_BREAKPOINT;
    LogEvent(TEXT("%s\n"), arrcStdoutBuf);
    CloseHandle(hRead);
    
    //Convert char* string to a wchar_t* string.
    UINT nConvertedChars = 0;
    UINT nNewsize = strlen(arrcStdoutBuf) + 1;
    WCHAR arrcReadBuf[PROCESS_STDOUT_BUFSIZE] = { 0 };
    mbstowcs_s(&nConvertedChars, arrcReadBuf, nNewsize, arrcStdoutBuf, _TRUNCATE);
    //Display the result and indicate the type of string that it is.
    LogEvent(TEXT("%s\n"), arrcReadBuf);

    if (wcsstr(arrcReadBuf, TEXT("SERVICE_STOPPED"))) {
        return PROCESS_EXIT_CODE;
    }

    if (wcsstr(arrcReadBuf, TEXT("SERVICE_RUNNING"))) {
        return STILL_ACTIVE;
    }
    return EXCEPTION_BREAKPOINT;
}

VOID GetSystemServiceOutput(WCHAR *szReadBuf)
{
    CHAR chbuf[PROCESS_STDOUT_BUFSIZE] = { 0 };
    BOOL bSuccess = FALSE;
    HANDLE hParentStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    OVERLAPPED ol = { 0 };

    HANDLE  hFile = CreateFileW(
        GetNssmLogPath(), // file to open
        GENERIC_READ,          // open for reading
        FILE_SHARE_READ | FILE_SHARE_WRITE,       // share for reading
        NULL,                  // default security
        OPEN_EXISTING,         // existing file only
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, // normal file
        NULL);
    
    DWORD dwOffset = SetFilePointer(hFile, 0 - PROCESS_STDOUT_BUFSIZE - 1, NULL, FILE_END);
    if (dwOffset == INVALID_SET_FILE_POINTER) {
        LogEvent(TEXT("Terminal failure: unable to set file pointer.\n"));
        return;
    }
    ol.Offset = dwOffset;
    bSuccess = ReadFileEx(hFile, chbuf, PROCESS_STDOUT_BUFSIZE, &ol, NULL);
    if (!bSuccess) return;
    //Convert char* string to a wchar_t* string.
    UINT convertedChars = 0;
    mbstowcs_s(&convertedChars, szReadBuf, PROCESS_STDOUT_BUFSIZE, chbuf, _TRUNCATE);
    //Display the result and indicate the type of string that it is.
    LogEvent(TEXT("%s\n"), szReadBuf);
}

VOID TerminalSystemService(VOID)
{
    return;
}