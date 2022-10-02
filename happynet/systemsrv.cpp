#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#include <Winsock.h>
#include "process.h"
#include "registry.h"
#include "systemsrv.h"
#include "utils.h"

static WCHAR* get_nssm_exe_path(VOID)
{
    static WCHAR nssm_path[MAX_PATH] = { 0 };
    WCHAR install_path[MAX_PATH] = { 0 };
    
    // Build path and command line parameters
    if (!GetInstallDirPath(install_path, MAX_PATH))
    {
        LogEvent(TEXT("%s:%d (%s) - Error building executable path.\n"),
            __FILEW__, __LINE__, __FUNCTIONW__);
        return NULL;
    }
    swprintf_s(nssm_path, MAX_PATH, TEXT("\"%s\\happynssm.exe\""), install_path);
    return nssm_path;
}

static WCHAR* get_nssm_log_path(VOID)
{
    static WCHAR nssm_log_path[MAX_PATH] = { 0 };
    WCHAR app_datapath[MAX_PATH] = { 0 };

    // Build path and command line parameters
    GetAppDatapath(app_datapath);
    swprintf_s(nssm_log_path, MAX_PATH, L"%s\\happynet.log", app_datapath);
    LogEvent(TEXT("%s:%d (%s) - building log path:%s \n"),
        __FILEW__, __LINE__, __FUNCTIONW__, nssm_log_path);
    return nssm_log_path;
}


// nssm install <servicename> <program> [<arguments>]
// nssm set <servicename> Description "Happynet is a light VPN software which makes it easy to create virtual networks bypassing intermediate firewalls. Powered by happyn.cn"
VOID RegSystemService(VOID)
{
    WCHAR install_path[MAX_PATH] = { 0 };
    WCHAR edge_path[MAX_PATH] = { 0 };
    WCHAR params_line[MAX_COMMAND_LINE_LEN] = { 0 };
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = { 0 };

    // Build path and command line parameters
    if (!GetInstallDirPath(install_path, MAX_PATH))
    {
        LogEvent(TEXT("%s:%d (%s) - Error building executable path.\n"),
                    __FILEW__, __LINE__, __FUNCTIONW__);
        return;
    }
    swprintf_s(edge_path, MAX_PATH, TEXT("\"%s\\happynedge.exe\""), install_path);
    INT ret = 0;

    GetEdgeParams(TEXT(" "), params_line, MAX_COMMAND_LINE_LEN);
    
    // nssm install <servicename> <program>[<arguments>]
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
                TEXT("%s install %s %s \"%s\""),
                get_nssm_exe_path(), SYSTEMSRV_NAME, edge_path, params_line);

    WinExecW(nssm_command_line, SW_HIDE);

    //nssm set <servicename> Description "Happynet ..."
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
        TEXT("%s set %s Description \"Happynet is a light VPN software which makes it easy to create virtual networks bypassing intermediate firewalls. Powered by happyn.cn\""),
        get_nssm_exe_path(), SYSTEMSRV_NAME);
    WinExecW(nssm_command_line, SW_HIDE);

    //nssm set <servicename> AppStdout logpath
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
        TEXT("%s set %s AppStdout %s"),
        get_nssm_exe_path(), SYSTEMSRV_NAME, get_nssm_log_path());
    WinExecW(nssm_command_line, SW_HIDE);

    //nssm set <servicename> AppStderr logpath
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
        TEXT("%s set %s AppStderr %s"),
        get_nssm_exe_path(), SYSTEMSRV_NAME, get_nssm_log_path());
    WinExecW(nssm_command_line, SW_HIDE);
}

// nssm remove <servicename>
VOID UnregSystemService(VOID)
{
    WCHAR nssm_path[MAX_PATH] = { 0 };
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = { 0 };
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
                TEXT("%s remove %s confirm"), get_nssm_exe_path(), SYSTEMSRV_NAME);

    StopSystemService();
    WinExecW(nssm_command_line, SW_HIDE);
}

// nssm set <servicename> Start SERVICE_AUTO_START
VOID SetSystemServiceAutoStart(VOID)
{
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = { 0 };
    //nssm set <servicename> Start SERVICE_AUTO_START
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
            TEXT("%s set %s Start SERVICE_AUTO_START"),
            get_nssm_exe_path(), SYSTEMSRV_NAME);
    WinExecW(nssm_command_line, SW_HIDE);
    return;
}

// nssm set <servicename> Start SERVICE_DEMAND_START
VOID UnsetSystemServiceAutoStart(VOID)
{
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = { 0 };
    //nssm set <servicename> Start SERVICE_DEMAND_START
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
        TEXT("%s set %s Start SERVICE_DEMAND_START"),
        get_nssm_exe_path(), SYSTEMSRV_NAME);
    WinExecW(nssm_command_line, SW_HIDE);
    return;
}

// nssm start <servicename>
VOID StartSystemService(VOID)
{
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = { 0 };
    //nssm set <servicename> Start SERVICE_DEMAND_START
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
        TEXT("%s start  %s"),
        get_nssm_exe_path(), SYSTEMSRV_NAME);
    WinExecW(nssm_command_line, SW_HIDE);
    return;
}

// nssm stop <servicename>
VOID StopSystemService(VOID)
{
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = { 0 };
    //nssm stop <servicename>
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
        TEXT("%s stop  %s"),
        get_nssm_exe_path(), SYSTEMSRV_NAME);
    WinExecW(nssm_command_line, SW_HIDE);
    return;
}

// nssm status <servicename>
// result:
// Can't open service!
// SERVICE_STOPPED
// SERVICE_RUNNING
DWORD GetSystemServiceStatus(VOID)
{
    //创建匿名管道
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    HANDLE hRead, hWrite;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
    {
        return EXCEPTION_BREAKPOINT;
    }

    //设置命令行进程启动信息(以隐藏方式启动命令并定位其输出到hWrite)
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    GetStartupInfo(&si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdError = hWrite;
    si.hStdOutput = hWrite;

    //启动命令行
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = { 0 };
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
        TEXT("%s status %s"),
        get_nssm_exe_path(), SYSTEMSRV_NAME);
    PROCESS_INFORMATION pi;
    if (!CreateProcessW(NULL, nssm_command_line, NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
    {
        return EXCEPTION_BREAKPOINT;
    }

    //立即关闭hWrite
    CloseHandle(hWrite);

    //读取命令行返回值
    DWORD dwread;
    CHAR chbuf[PROCESS_STDOUT_BUFSIZE] = { 0 };
    BOOL bsuccess = FALSE;
    HANDLE hparent_stdout = GetStdHandle(STD_OUTPUT_HANDLE);

    bsuccess = ReadFile(hRead, chbuf, PROCESS_STDOUT_BUFSIZE, &dwread, NULL);
    if (!bsuccess || dwread == 0) return EXCEPTION_BREAKPOINT;
    LogEvent(TEXT("%s\n"), chbuf);
    CloseHandle(hRead);
    
    //Convert char* string to a wchar_t* string.
    UINT convertedChars = 0;
    UINT newsize = strlen(chbuf) + 1;
    WCHAR read_buf[PROCESS_STDOUT_BUFSIZE] = { 0 };
    mbstowcs_s(&convertedChars, read_buf, newsize, chbuf, _TRUNCATE);
    //Display the result and indicate the type of string that it is.
    LogEvent(TEXT("%s\n"), read_buf);

    if (wcsstr(read_buf, TEXT("SERVICE_STOPPED"))) {
        return PROCESS_EXIT_CODE;
    }

    if (wcsstr(read_buf, TEXT("SERVICE_RUNNING"))) {
        return STILL_ACTIVE;
    }
    return EXCEPTION_BREAKPOINT;
}

VOID GetSystemServiceOutput(WCHAR *szReadBuf)
{
    CHAR chbuf[PROCESS_STDOUT_BUFSIZE] = { 0 };
    BOOL bsuccess = FALSE;
    HANDLE hparent_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    OVERLAPPED ol = { 0 };

    HANDLE  hFile = CreateFileW(
        get_nssm_log_path(), // file to open
        GENERIC_READ,          // open for reading
        FILE_SHARE_READ | FILE_SHARE_WRITE,       // share for reading
        NULL,                  // default security
        OPEN_EXISTING,         // existing file only
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, // normal file
        NULL);
    
    DWORD offset = SetFilePointer(hFile, 0-PROCESS_STDOUT_BUFSIZE-1, NULL, FILE_END);
    if (offset == INVALID_SET_FILE_POINTER) {
        LogEvent(TEXT("Terminal failure: unable to set file pointer.\n"));
        return;
    }
    ol.Offset = offset;
    bsuccess = ReadFileEx(hFile, chbuf, PROCESS_STDOUT_BUFSIZE, &ol, NULL);
    if (!bsuccess) return;
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