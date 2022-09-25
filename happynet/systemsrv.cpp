#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#include <Winsock.h>
#include "registry.h"
#include "systemsrv.h"



void reg_service_system(WCHAR* command_line)
{

}

void unreg_service_system(WCHAR* command_line)
{

}

BOOL start_service_system(WCHAR* command_line)
{
    return TRUE;
}

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
