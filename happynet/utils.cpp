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