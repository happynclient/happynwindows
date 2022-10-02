#include <windows.h>
#include "registry.h"

INT GetRegDword(HKEY hkey, LPWSTR pszValueName, LPDWORD lpdwRetValue)
{
  // Fetch DWORD value from registry
  DWORD buf_size = sizeof(DWORD);
  if (RegQueryValueEx(hkey, pszValueName, NULL, NULL, (LPBYTE)lpdwRetValue, &buf_size) != ERROR_SUCCESS)
  {
    *lpdwRetValue = 0;
    return 0;
  }
  return 1;
}

INT GetRegString(HKEY hkey, LPWSTR pszValueName, LPWSTR pszRetValue, DWORD dwBufSize)
{
  // Fetch string value from registry
  if (RegQueryValueEx(hkey, pszValueName, NULL, NULL, (LPBYTE)pszRetValue, &dwBufSize) != ERROR_SUCCESS)
  {
    return 0;
  }
  return 1;
}

INT SetRegDword(HKEY hkey, LPWSTR pszValueName, DWORD dwValue)
{
  // Set DWORD value in registry
  if (RegSetValueEx(hkey, pszValueName, NULL, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD)) != ERROR_SUCCESS)
  {
    return 0;
  }
  return 1;
}

INT SetRegString(HKEY hkey, LPWSTR pszValueName, LPWSTR pszValueString)
{
  DWORD data_len = (wcslen(pszValueString) + 1) * sizeof(WCHAR);
  // Set string value in registry
  if (RegSetValueEx(hkey, pszValueName, NULL, REG_SZ, (LPBYTE)pszValueString, data_len) != ERROR_SUCCESS)
  {
    return 0;
  }
  return 1;
}

BOOL IsSetSystemService(VOID)
{
    DWORD dword_buf;
    HKEY hkey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Happynet\\Parameters"), NULL, KEY_READ, &hkey) != ERROR_SUCCESS)
    {
        return FALSE;
    }
    // system_service
    GetRegDword(hkey, TEXT("system_service"), &dword_buf);
    RegCloseKey(hkey);

    return dword_buf != 0;
}

BOOL IsSetAutoStart(VOID)
{
    DWORD dword_buf;
    HKEY hkey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Happynet\\Parameters"), NULL, KEY_READ, &hkey) != ERROR_SUCCESS)
    {
        return FALSE;
    }
    // auto_start
    GetRegDword(hkey, TEXT("auto_start"), &dword_buf);
    RegCloseKey(hkey);

    return dword_buf != 0;
}

BOOL IsSetAutoTray(VOID)
{
    DWORD dword_buf;
    HKEY hkey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Happynet\\Parameters"), NULL, KEY_READ, &hkey) != ERROR_SUCCESS)
    {
        return FALSE;
    }
    // auto_tray
    GetRegDword(hkey, TEXT("auto_tray"), &dword_buf);
    RegCloseKey(hkey);

    return dword_buf != 0;
}
