#ifndef _H_SERVICE
#define _H_SERVICE

DWORD get_service_status(VOID);
VOID start_service(VOID);
VOID stop_service(VOID);
VOID set_auto_start_service(VOID);
VOID cancel_auto_start_service(VOID);

#endif
