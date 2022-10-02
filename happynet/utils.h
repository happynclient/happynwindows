#ifndef _H_UTILS
#define _H_UTILS

#include <windows.h>

#define MAX_COMMAND_LINE_LEN 1024 * 8

LARGE_INTEGER IntToLargeInt(UINT nCount);
VOID LogEvent(WCHAR* format, ...);
BOOL IsValidAsciiString(WCHAR *line);
BOOL StripNoAsciiString(WCHAR *line);
UINT WinExecW(WCHAR* command_line, UINT command_show);
INT GetAppDatapath(WCHAR* datapath);
INT GetInstallDirPath(WCHAR* install_path, DWORD buf_len);
INT GetEdgeCmdLine(WCHAR* happynedge_path, WCHAR* command_line, DWORD buf_len);
INT GetEdgeParams(WCHAR* edge_path, WCHAR* params_line, DWORD buf_len);

#endif