#ifndef _H_PROCESS
#define _H_PROCESS

#include <windows.h> 

#define PROCESS_STDOUT_BUFSIZE 4096

#define PROCESS_EXIT_CODE	0

HANDLE create_service_process(WCHAR* command_line);
void terminal_service_process(void);
void grace_stop_service_process(void);
DWORD get_service_process_status(void);
void get_service_process_error(PTSTR); 
void get_service_process_output(WCHAR *readBuf);


#endif