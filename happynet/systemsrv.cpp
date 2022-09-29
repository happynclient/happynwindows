#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#include <Winsock.h>
#include "registry.h"
#include "systemsrv.h"
#include "utils.h"

#define SYSTEMSRV_NAME L"HAPPYNET"

static WCHAR* get_nssm_exe_path()
{
    static WCHAR nssm_path[MAX_PATH] = L"0";
    WCHAR install_path[MAX_PATH] = L"0";
    
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
// nssm set <servicename> Description "Happynet a light VPN software which makes it easy to create virtual networks bypassing intermediate firewalls. Powered by happyn.cn"
void reg_service_system()
{
    WCHAR install_path[MAX_PATH] = L"0";
    WCHAR edge_path[MAX_PATH] = L"0";
    WCHAR params_line[MAX_COMMAND_LINE_LEN] = L"0";
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = L"0";

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
        L"%s set %s Description \"Happynet a light VPN software which makes it easy to create virtual networks bypassing intermediate firewalls. Powered by happyn.cn\"",
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
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = L"0";
    swprintf_s(nssm_command_line, MAX_COMMAND_LINE_LEN,
                L"%s remove %s confirm", get_nssm_exe_path(), SYSTEMSRV_NAME);

    terminal_service_system();
    WinExecW(nssm_command_line, SW_HIDE);
}

// nssm set <servicename> Start SERVICE_AUTO_START
void set_auto_start_service_system()
{
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = L"0";
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
    WCHAR nssm_command_line[MAX_COMMAND_LINE_LEN] = L"0";
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
BOOL start_service_system()
{
    return TRUE;
}

// nssm stop <servicename>
void grace_stop_service_system(void)
{
    
    return;
}

void terminal_service_system(void)
{

}


DWORD get_service_system_status(void)
{
    return 0;
}


void get_service_system_output(WCHAR *read_buf)
{

}
