#ifndef _H_REGISTRY
#define _H_REGISTRY

int reg_get_dword(HKEY hkey, LPWSTR value_name, LPDWORD ret_dword);
int reg_get_string(HKEY hkey, LPWSTR value_name, LPWSTR ret_str, DWORD buf_size);
int reg_set_dword(HKEY hkey, LPWSTR value_name, DWORD dword_val);
int reg_set_string(HKEY hkey, LPWSTR value_name, LPWSTR str_val);
BOOL is_system_service();
BOOL is_auto_start();
BOOL is_auto_tray();

#endif