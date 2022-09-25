#ifndef _H_SYSTEMSRV
#define _H_SYSTEMSRV

void reg_service_system(WCHAR* command_line);
void unreg_service_system(WCHAR* command_line);
BOOL start_service_system(WCHAR* command_line);
void grace_stop_service_system(void);
void terminal_service_system(void);
DWORD get_service_system_status(void);
void get_service_system_output(WCHAR *read_buf);

#endif