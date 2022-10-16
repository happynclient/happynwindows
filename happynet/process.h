#ifndef _H_PROCESS
#define _H_PROCESS

#include <windows.h> 

#define PROCESS_STDOUT_BUFSIZE 4096
#define PROCESS_EXIT_CODE	0

HANDLE CreateProcessService(WCHAR* pszCmdLine);
VOID TerminalProcessService(VOID);
VOID GraceStopProcessService(VOID);
DWORD GetProcessServiceStatus(VOID);
VOID GetProcessServiceError(PTSTR lpszFunction);
VOID GetProcessServiceOutput(WCHAR *pszReadBuf);


#endif