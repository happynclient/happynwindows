#include <windows.h>
#include "registry.h"

INT reg_get_dword(HKEY hkey, LPWSTR value_name, LPDWORD ret_dword)
{
  // Fetch DWORD value from registry
  DWORD buf_size = sizeof(DWORD);
  if (RegQueryValueEx(hkey, value_name, NULL, NULL, (LPBYTE)ret_dword, &buf_size) != ERROR_SUCCESS)
  {
    *ret_dword = 0;
    return 0;
  }
  return 1;
}

INT reg_get_string(HKEY hkey, LPWSTR value_name, LPWSTR ret_str, DWORD buf_size)
{
  // Fetch string value from registry
  if (RegQueryValueEx(hkey, value_name, NULL, NULL, (LPBYTE)ret_str, &buf_size) != ERROR_SUCCESS)
  {
    return 0;
  }
  return 1;
}

INT reg_set_dword(HKEY hkey, LPWSTR value_name, DWORD dword_val)
{
  // Set DWORD value in registry
  if (RegSetValueEx(hkey, value_name, NULL, REG_DWORD, (LPBYTE)&dword_val, sizeof(DWORD)) != ERROR_SUCCESS)
  {
    return 0;
  }
  return 1;
}

INT reg_set_string(HKEY hkey, LPWSTR value_name, LPWSTR str_val)
{
  DWORD data_len = (wcslen(str_val) + 1) * sizeof(WCHAR);
  // Set string value in registry
  if (RegSetValueEx(hkey, value_name, NULL, REG_SZ, (LPBYTE)str_val, data_len) != ERROR_SUCCESS)
  {
    return 0;
  }
  return 1;
}

BOOL is_system_service()
{
    DWORD dword_buf;
    HKEY hkey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Happynet\\Parameters"), NULL, KEY_READ, &hkey) != ERROR_SUCCESS)
    {
        return FALSE;
    }
    // system_service
    reg_get_dword(hkey, TEXT("system_service"), &dword_buf);
    RegCloseKey(hkey);

    return dword_buf != 0;
}

BOOL is_auto_start()
{
    DWORD dword_buf;
    HKEY hkey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Happynet\\Parameters"), NULL, KEY_READ, &hkey) != ERROR_SUCCESS)
    {
        return FALSE;
    }
    // auto_start
    reg_get_dword(hkey, TEXT("auto_start"), &dword_buf);
    RegCloseKey(hkey);

    return dword_buf != 0;
}

BOOL is_auto_tray()
{
    DWORD dword_buf;
    HKEY hkey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Happynet\\Parameters"), NULL, KEY_READ, &hkey) != ERROR_SUCCESS)
    {
        return FALSE;
    }
    // auto_tray
    reg_get_dword(hkey, TEXT("auto_tray"), &dword_buf);
    RegCloseKey(hkey);

    return dword_buf != 0;
}
