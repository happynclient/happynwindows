#ifndef _H_UTILS
#define _H_UTILS

#include <windows.h>

#define MAX_COMMAND_LINE_LEN 1024*8

void log_event(WCHAR* format, ...);
BOOL is_valid_ascii_string(WCHAR *line);
BOOL strip_no_ascii_string(WCHAR *line);

#endif