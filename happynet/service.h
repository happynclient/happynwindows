#ifndef _H_SERVICE
#define _H_SERVICE

#define MAX_COMMAND_LINE_LEN 1024*8

DWORD get_service_status();
void start_service();
void stop_service();
void set_auto_start_service();
void cancel_auto_start_service();

void log_event(WCHAR* format, ...);
int build_exe_path(WCHAR* exe_path, DWORD buf_len);
int build_command_line_edge(WCHAR* exe_path, WCHAR* command_line, DWORD buf_len);


#endif
