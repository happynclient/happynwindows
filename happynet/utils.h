#ifndef _H_UTILS
#define _H_UTILS

#include <windows.h>

BOOL is_valid_ascii_string(WCHAR *line);
BOOL strip_no_ascii_string(WCHAR *line);

#endif