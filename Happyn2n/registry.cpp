#include <windows.h>
#include "registry.h"

int reg_get_dword(HKEY hkey, LPWSTR value_name, LPDWORD ret_dword)
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

int reg_get_string(HKEY hkey, LPWSTR value_name, LPWSTR ret_str, DWORD buf_size)
{
  // Fetch string value from registry
  if (RegQueryValueEx(hkey, value_name, NULL, NULL, (LPBYTE)ret_str, &buf_size) != ERROR_SUCCESS)
  {
    return 0;
  }
  return 1;
}

int reg_set_dword(HKEY hkey, LPWSTR value_name, DWORD dword_val)
{
  // Set DWORD value in registry
  if (RegSetValueEx(hkey, value_name, NULL, REG_DWORD, (LPBYTE)&dword_val, sizeof(DWORD)) != ERROR_SUCCESS)
  {
    return 0;
  }
  return 1;
}

int reg_set_string(HKEY hkey, LPWSTR value_name, LPWSTR str_val)
{
  DWORD data_len = (wcslen(str_val) + 1) * sizeof(WCHAR);
  // Set string value in registry
  if (RegSetValueEx(hkey, value_name, NULL, REG_SZ, (LPBYTE)str_val, data_len) != ERROR_SUCCESS)
  {
    return 0;
  }
  return 1;
}