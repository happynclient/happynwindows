#ifndef _H_SYSTEMSRV
#define _H_SYSTEMSRV

#define SYSTEMSRV_NAME TEXT("Happynet")

VOID reg_service_system(VOID);
VOID unreg_service_system(VOID);
VOID set_auto_start_service_system(VOID);
VOID cancel_auto_start_service_system(VOID);
VOID start_service_system(VOID);
VOID stop_service_system(VOID);
VOID terminal_service_system(VOID);
VOID get_service_system_output(WCHAR *read_buf);
DWORD get_service_system_status(VOID);

#endif