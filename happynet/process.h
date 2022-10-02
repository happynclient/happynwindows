#ifndef _H_PROCESS
#define _H_PROCESS

#include <windows.h> 

#define PROCESS_STDOUT_BUFSIZE 4096
#define PROCESS_EXIT_CODE	0

HANDLE create_service_process(WCHAR* command_line);
VOID terminal_service_process(VOID);
VOID grace_stop_service_process(VOID);
DWORD get_service_process_status(VOID);
VOID get_service_process_error(PTSTR); 
VOID get_service_process_output(WCHAR *readBuf);


#endif