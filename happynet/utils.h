#ifndef _H_UTILS
#define _H_UTILS

#include <windows.h>

#define MAX_COMMAND_LINE_LEN 1024*8

void log_event(WCHAR* format, ...);
BOOL is_valid_ascii_string(WCHAR *line);
BOOL strip_no_ascii_string(WCHAR *line);
int build_exe_path(WCHAR* exe_path, DWORD buf_len);
int build_command_line_edge(WCHAR* exe_path, WCHAR* command_line, DWORD buf_len);

#endif