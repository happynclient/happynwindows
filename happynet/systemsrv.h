#ifndef _H_SYSTEMSRV
#define _H_SYSTEMSRV

#define SYSTEMSRV_NAME TEXT("Happynet")

VOID RegSystemService(VOID);
VOID UnregSystemService(VOID);
VOID SetSystemServiceAutoStart(VOID);
VOID UnsetSystemServiceAutoStart(VOID);
VOID StartSystemService(VOID);
VOID StopSystemService(VOID);
VOID TerminalSystemService(VOID);
VOID GetSystemServiceOutput(WCHAR *read_buf);
DWORD GetSystemServiceStatus(VOID);

#endif