#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#include <Winsock.h>
#include "process.h"
#include "registry.h"
#include "systemsrv.h"
#include "utils.h"

#define SYSTEMSRV_NAME L"HAPPYNET"

static WCHAR* get_nssm_exe_path()
{
    static WCHAR nssm_path[MAX_PATH] = { '\0' };
    WCHAR install_path[MAX_PATH] = { '\0' };
    
    // Build path and command line parameters
    if (!get_install_dir_path(install_path, MAX_PATH))
    {
        log_event(L"%s:%d (%s) - Error building executable path.\n",
            __FILEW__, __LINE__, __FUNCTIONW__);
        return NULL;
    }
    swprintf_s(nssm_path, MAX_PATH, L"\"%s\\happynssm.exe\"", install_path);
    return nssm_path;
}

// nssm install <servicename> <program> [<arguments>]
// nssm set <servicename> Description "Happynet is a light VPN software which makes it easy to create virtual networks bypassing intermediate firewalls. Powered by happyn.cn"
void reg_service_system()
{
    WCHAR install_path[MAX_PATH] = { '\0' };
    WCHAR edge_path[MAX_PATH] = { '\0' };
    WCHAR params_line[MAX_COMMAND_LINE_LEN] = { '\0' };
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = { '\0' };

    // Build path and command line parameters
    if (!get_install_dir_path(install_path, MAX_PATH))
    {
        log_event(L"%s:%d (%s) - Error building executable path.\n",
                    __FILEW__, __LINE__, __FUNCTIONW__);
        return;
    }
    swprintf_s(edge_path, MAX_PATH, L"\"%s\\happynedge.exe\"", install_path);
    int ret = 0;

    get_params_edge(L" ", params_line, MAX_COMMAND_LINE_LEN);
    
    // nssm install <servicename> <program>[<arguments>]
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
                L"%s install %s %s \"%s\"",
                get_nssm_exe_path(), SYSTEMSRV_NAME, edge_path, params_line);

    log_event(L"%s:%d (%s) - building nssm line: %s \n",
                __FILEW__, __LINE__, __FUNCTIONW__,
                nssm_command_line);
    WinExecW(nssm_command_line, SW_HIDE);

    //nssm set <servicename> Description "Happynet ..."
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
        L"%s set %s Description \"Happynet is a light VPN software which makes it easy to create virtual networks bypassing intermediate firewalls. Powered by happyn.cn\"",
        get_nssm_exe_path(), SYSTEMSRV_NAME);
    log_event(L"%s:%d (%s) - building nssm line: %s \n",
        __FILEW__, __LINE__, __FUNCTIONW__,
        nssm_command_line);
    WinExecW(nssm_command_line, SW_HIDE);
}

// nssm remove <servicename>
void unreg_service_system()
{
    WCHAR nssm_path[MAX_PATH] = L"0";
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = { '\0' };
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
                L"%s remove %s confirm", get_nssm_exe_path(), SYSTEMSRV_NAME);

    stop_service_system();
    WinExecW(nssm_command_line, SW_HIDE);
}

// nssm set <servicename> Start SERVICE_AUTO_START
void set_auto_start_service_system()
{
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = { '\0' };
    //nssm set <servicename> Start SERVICE_AUTO_START
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
            L"%s set %s Start SERVICE_AUTO_START",
            get_nssm_exe_path(), SYSTEMSRV_NAME);
    log_event(L"%s:%d (%s) - building nssm line: %s \n",
        __FILEW__, __LINE__, __FUNCTIONW__,
        nssm_command_line);
    WinExecW(nssm_command_line, SW_HIDE);
    return;
}

// nssm set <servicename> Start SERVICE_DEMAND_START
void cancel_auto_start_service_system()
{
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = { '\0' };
    //nssm set <servicename> Start SERVICE_DEMAND_START
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
        L"%s set %s Start SERVICE_DEMAND_START",
        get_nssm_exe_path(), SYSTEMSRV_NAME);
    log_event(L"%s:%d (%s) - building nssm line: %s \n",
        __FILEW__, __LINE__, __FUNCTIONW__,
        nssm_command_line);
    WinExecW(nssm_command_line, SW_HIDE);
    return;
}

// nssm start <servicename>
void start_service_system()
{
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = { '\0' };
    //nssm set <servicename> Start SERVICE_DEMAND_START
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
        L"%s start  %s",
        get_nssm_exe_path(), SYSTEMSRV_NAME);
    log_event(L"%s:%d (%s) - building nssm line: %s \n",
        __FILEW__, __LINE__, __FUNCTIONW__,
        nssm_command_line);
    WinExecW(nssm_command_line, SW_HIDE);
    return;
}

// nssm stop <servicename>
void stop_service_system(void)
{
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = { '\0' };
    //nssm stop <servicename>
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
        L"%s stop  %s",
        get_nssm_exe_path(), SYSTEMSRV_NAME);
    log_event(L"%s:%d (%s) - building nssm line: %s \n",
        __FILEW__, __LINE__, __FUNCTIONW__,
        nssm_command_line);
    WinExecW(nssm_command_line, SW_HIDE);
    return;
}

void terminal_service_system(void)
{

}

// nssm status <servicename>
// result:
// Can't open service!
// SERVICE_STOPPED
// SERVICE_RUNNING
DWORD get_service_system_status(void)
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
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = { '\0' };
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
        L"%s status %s",
        get_nssm_exe_path(), SYSTEMSRV_NAME);
    log_event(L"%s:%d (%s) - building nssm line: %s \n",
        __FILEW__, __LINE__, __FUNCTIONW__,
        nssm_command_line);
    PROCESS_INFORMATION pi;
    if (!CreateProcessW(NULL, nssm_command_line, NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
    {
        return EXCEPTION_BREAKPOINT;
    }

    //立即关闭hWrite
    CloseHandle(hWrite);

    //读取命令行返回值
    DWORD dwread;
    CHAR chbuf[PROCESS_STDOUT_BUFSIZE] = { '\0' };
    BOOL bsuccess = FALSE;
    HANDLE hparent_stdout = GetStdHandle(STD_OUTPUT_HANDLE);

    bsuccess = ReadFile(hRead, chbuf, PROCESS_STDOUT_BUFSIZE, &dwread, NULL);
    if (!bsuccess || dwread == 0) return EXCEPTION_BREAKPOINT;
    log_event(L"%s\n", chbuf);
    CloseHandle(hRead);
    
    //Convert char* string to a wchar_t* string.
    size_t convertedChars = 0;
    size_t newsize = strlen(chbuf) + 1;
    WCHAR read_buf[PROCESS_STDOUT_BUFSIZE] = { '\0' };
    mbstowcs_s(&convertedChars, read_buf, newsize, chbuf, _TRUNCATE);
    //Display the result and indicate the type of string that it is.
    log_event(L"%s\n", read_buf);

    if (wcsstr(read_buf, L"SERVICE_STOPPED")) {
        return PROCESS_EXIT_CODE;
    }

    if (wcsstr(read_buf, L"SERVICE_RUNNING")) {
        return STILL_ACTIVE;
    }
    return EXCEPTION_BREAKPOINT;
}


void get_service_system_output(WCHAR *read_buf)
{

}