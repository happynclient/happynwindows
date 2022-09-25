#include <stdio.h>
#include "utils.h"

BOOL is_valid_ascii_string(WCHAR *line)
{
	for (int i = 0; i < lstrlenW(line); i++)
	{
		if (!iswascii(line[i])) {
			return FALSE;
		}
	}
	return TRUE;
}

BOOL strip_no_ascii_string(WCHAR *line)
{
	int i = 0;
	for (i = 0; i < lstrlenW(line); i++)
	{
		if (!iswascii(line[i])) {
			line[i] = L'\0';
			break;
		}
	}
	return i;
}

void log_event(WCHAR* format, ...)
{
    //WCHAR message[4096] = {'\0'};
    int n = 0;
    va_list arg;

    // Construct the message
    va_start(arg, format);
    int size = _vscwprintf(format, arg) + 1;
    wchar_t *message = new wchar_t[size];
    n = _vsnwprintf_s(message, size, size, format, arg);
    va_end(arg);

    // Check success
    if (n < 0 || n >= size) return;

    // Log the message
    OutputDebugStringW(message);
    delete[] message;
}