#ifndef _H_UTILS
#define _H_UTILS

#include <windows.h>

#define MAX_COMMAND_LINE_LEN 1024 * 8

LARGE_INTEGER IntToLargeInt(UINT nCount);
VOID LogEvent(WCHAR* format, ...);
BOOL IsValidAsciiString(WCHAR *pszLine);
BOOL StripNoAsciiString(WCHAR *pszLine);
UINT WinExecW(WCHAR* pszCommandLine, UINT nCommandShow);
INT GetAppDatapath(WCHAR* pszDatapath);
INT GetInstallDirPath(WCHAR* pszDirPath, DWORD dwBufLen);
INT GetEdgeCmdLine(WCHAR* pszDirPath, WCHAR* pszCommandLine, DWORD dwBufLen);
INT GetEdgeParams(WCHAR* pszEdgePath, WCHAR* pszCommandLine, DWORD dwCommandBufLen);

#endif