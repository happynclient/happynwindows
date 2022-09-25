#ifndef _H_SERVICE
#define _H_SERVICE

DWORD get_service_status();
void start_service();
void stop_service();
void set_auto_start_service();
void cancel_auto_start_service();

#endif
