#ifndef _H_REGISTRY
#define _H_REGISTRY

INT GetRegDword(HKEY hkey, LPWSTR pszValueName, LPDWORD lpdwRetValue);
INT GetRegString(HKEY hkey, LPWSTR pszValueName, LPWSTR pszRetValue, DWORD dwBufSize);
INT SetRegDword(HKEY hkey, LPWSTR pszValueName, DWORD dwValue);
INT SetRegString(HKEY hkey, LPWSTR pszValueName, LPWSTR pszValueString);
BOOL IsSetSystemService(VOID);
BOOL IsSetAutoStart(VOID);
BOOL IsSetAutoTray(VOID);

#endif