#ifndef _H_SYSTEMSRV
#define _H_SYSTEMSRV

void reg_service_system(void);
void unreg_service_system(void);
void set_auto_start_service_system(void);
void cancel_auto_start_service_system(void);
void start_service_system(void);
void stop_service_system(void);
void terminal_service_system(void);
DWORD get_service_system_status(void);
void get_service_system_output(void);

#endif