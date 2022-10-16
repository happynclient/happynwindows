#ifndef _H_SERVICE
#define _H_SERVICE

DWORD GetServiceStatus(VOID);
VOID StartService(VOID);
VOID StopService(VOID);
VOID SetServiceAutoStart(VOID);
VOID UnsetServiceAutoStart(VOID);

#endif
