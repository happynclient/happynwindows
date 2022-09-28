#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#include <Winsock.h>
#include "registry.h"
#include "systemsrv.h"
#include "utils.h"

#define SYSTEMSRV_NAME L"happynet"

// nssm install <servicename> <program> [<arguments>]
void reg_service_system(WCHAR* program, WCHAR* arguments)
{
    WCHAR exe_path[MAX_PATH] = L"0";
    WCHAR command_line[MAX_COMMAND_LINE_LEN] = L"0";    

    // Build path and command line parameters
    if (!get_install_dir_path(exe_path, MAX_PATH))
    {
        log_event(L"%s:%d (%s) - Error building executable path.\n", __FILEW__, __LINE__, __FUNCTIONW__);
        return;
    }
    int ret = 0;

    ret = get_command_line_edge(exe_path, command_line, MAX_COMMAND_LINE_LEN);

    log_event(L"%s:%d (%s) - building command line: %s \n", __FILEW__, __LINE__, __FUNCTIONW__, command_line);

    log_event(L"\n->Start of parent execution.\n");
}

// nssm remove <servicename>
void unreg_service_system(WCHAR* command_line)
{

}

// nssm start <servicename>
void set_service_system_auto_start(WCHAR* command_line)
{
    return;
}

// nssm start <servicename>
BOOL start_service_system(WCHAR* command_line)
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
