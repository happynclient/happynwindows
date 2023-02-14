#ifndef _H_SERVICE
#define _H_SERVICE

DWORD GetHappynetServiceStatus(VOID);
VOID StartHappynetService(VOID);
VOID StopHappynetService(VOID);
VOID SetHappynetServiceAutoStart(VOID);
VOID UnsetHappynetServiceAutoStart(VOID);

#endif
