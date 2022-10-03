#include <atlstr.h>
#include <comdef.h>
#include <stdio.h>
#include <ShlObj.h>
#include <windows.h>
#include <WinError.h>

#include "atlbase.h"
#include "atlstr.h"

#include "netinterface.h"
#include "service.h"
#include "registry.h"
#include "process.h"
#include "utils.h"

LARGE_INTEGER IntToLargeInt(UINT nCount) {
    LARGE_INTEGER li;
    li.QuadPart = nCount;
    return li;
}


BOOL IsValidAsciiString(WCHAR *pszLine)
{
	for (INT i = 0; i < lstrlenW(pszLine); i++)
	{
		if (!iswascii(pszLine[i])) {
			return FALSE;
		}
	}
	return TRUE;
}

BOOL StripNoAsciiString(WCHAR *pszLine)
{
	INT i = 0;
	for (i = 0; i < lstrlenW(pszLine); i++)
	{
		if (!iswascii(pszLine[i])) {
			pszLine[i] = L'\0';
			break;
		}
	}
	return i;
}

UINT WinExecW(WCHAR* pszCommandLine, UINT nCommandShow)
{
    USES_CONVERSION;
    return WinExec(W2A(pszCommandLine), nCommandShow);
}

VOID LogEvent(WCHAR* format, ...)
{
    INT n = 0;
    va_list arg;

    // Construct the message
    va_start(arg, format);
    INT size = _vscwprintf(format, arg) + 1;
    WCHAR *message = new WCHAR[size];
    n = _vsnwprintf_s(message, size, size, format, arg);
    va_end(arg);

    // Check success
    if (n < 0 || n >= size) return;

    // Log the message
    OutputDebugStringW(message);
    delete[] message;
}

INT GetInstallDirPath(WCHAR* pszDirPath, DWORD dwBufLen)
{
    WCHAR arrcExeDirBuf[MAX_PATH] = { 0 };
    DWORD nExeDirBufLen = dwBufLen * sizeof(WCHAR);
    
    // get happyn exe dir path
    if (!GetModuleFileName(NULL, arrcExeDirBuf, MAX_PATH))
    {
        return 0;
    }
    PathRemoveFileSpec(arrcExeDirBuf);
    swprintf_s(pszDirPath, dwBufLen, arrcExeDirBuf);
    return 1;
}


INT GetAppDatapath(WCHAR* pszDatapath)
{
    WCHAR pszDefaultAppDatapath[MAX_PATH] = { 0 };
    SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, pszDefaultAppDatapath);
    swprintf_s(pszDatapath, MAX_PATH, TEXT("%s\\happynet"), pszDefaultAppDatapath);
    return SHCreateDirectoryEx(NULL, pszDatapath, NULL);
}


INT GetEdgeCmdLine(WCHAR* pszDirPath, WCHAR* pszCommandLine, DWORD dwBufLen)
{
    WCHAR edge_path[MAX_PATH] = { 0 };
    swprintf_s(edge_path, MAX_PATH, TEXT("\"%s\\happynedge.exe\""), pszDirPath);
    return GetEdgeParams(edge_path, pszCommandLine, dwBufLen);
}


INT GetEdgeParams(WCHAR* pszEdgePath, WCHAR* pszCommandLine, DWORD dwCommandBufLen)
{
    WCHAR arrcRetVal[MAX_COMMAND_LINE_LEN];
    DWORD dwRetVal = 0;

    // Use 'ptr' to append to the end of the command line
    WCHAR* ptr = pszCommandLine;
    ptr += swprintf_s(pszCommandLine, dwCommandBufLen, TEXT("%s "), pszEdgePath);

    // Open registry key
    HKEY hkey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Happynet\\Parameters"),
                        NULL, KEY_READ, &hkey) != ERROR_SUCCESS)
    {
        LogEvent(TEXT("%s:%d (%s) - Error opening registry key.\n"),
                    __FILEW__, __LINE__, __FUNCTIONW__);
        return 0;
    }

    // Community
    if (!GetRegString(hkey, TEXT("community"), arrcRetVal, 512)) return 0;
    ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" -c %s"), arrcRetVal);

    // Encryption key
    if (!GetRegString(hkey, TEXT("enckey"), arrcRetVal, 512)) return 0;
    if (wcslen(arrcRetVal) != 0)
    {
        ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" -k %s"), arrcRetVal);
    }

    // IP address
    if (!GetRegString(hkey, TEXT("ip_address"), arrcRetVal, 512)) return 0;
    if (!GetRegDword(hkey, TEXT("packet_forwarding"), &dwRetVal)) return 0;
    if (wcslen(arrcRetVal) != 0 && dwRetVal == 0)
    {
        ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" -a %s"), arrcRetVal);
    }

    if (wcslen(arrcRetVal) == 0 && dwRetVal == 0)
    {
        ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" -r -a dhcp:0.0.0.0"));
    }

    // Encryption key file
    if (!GetRegString(hkey, TEXT("keyfile"), arrcRetVal, 512)) return 0;
    if (wcslen(arrcRetVal) != 0)
    {
        ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" -K %s"), arrcRetVal);
    }

    // set adapter
    if (!GetRegString(hkey, TEXT("adapter"), arrcRetVal, 512)) return 0;
    if (wcslen(arrcRetVal) != 0)
    {
        CHAR *pAdapterId = NULL;
        const CHAR s[2] = "_";
        _bstr_t b(arrcRetVal);
        CHAR *pTmpBuf = NULL;
        pAdapterId = strtok_s(b, s, &pTmpBuf);
        pAdapterId = strtok_s(NULL, s, &pTmpBuf);

        TCHAR adapter_firendly_name[512] = { 0 };
        if (GetAdapterFriendlyName(pAdapterId, adapter_firendly_name, 512) == NOERROR) {
            ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" -d \"%s\""), adapter_firendly_name);
        }
    }

    // Local Port
    if (!GetRegDword(hkey, TEXT("local_port"), &dwRetVal)) return 0;
    if (dwRetVal != 0)
    {
        ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" -p %d"), dwRetVal);
    }


    // MAC address
    if (!GetRegString(hkey, TEXT("mac_address"), arrcRetVal, 512)) return 0;
    if (wcslen(arrcRetVal) != 0)
    {
        ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" -m %s"), arrcRetVal);
    }

    // MTU
    if (!GetRegDword(hkey, TEXT("mtu"), &dwRetVal)) return 0;
    if (dwRetVal != 0)
    {
        ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" -M %d"), dwRetVal);
    }

    // Multicast
    if (!GetRegDword(hkey, TEXT("multicast"), &dwRetVal)) return 0;
    if (dwRetVal != 0)
    {
        ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" -E"));
    }

    // Packet forwarding
    if (!GetRegDword(hkey, TEXT("packet_forwarding"), &dwRetVal)) return 0;
    if (dwRetVal != 0)
    {
        ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" -r"));
    }

    // header encryption
    if (!GetRegDword(hkey, TEXT("header_encry"), &dwRetVal)) return 0;
    if (dwRetVal != 0)
    {
        ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" -H"));
    }

    // data compress
    if (!GetRegDword(hkey, TEXT("data_compress"), &dwRetVal)) return 0;
    if (dwRetVal != 0)
    {
        ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" -z1"));
    }

    // select rtt
    if (!GetRegDword(hkey, TEXT("select_rtt"), &dwRetVal)) return 0;
    if (dwRetVal != 0)
    {
        ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" --select-rtt "));
    }

    // Supernode address
    if (!GetRegString(hkey, TEXT("supernode_addr"), arrcRetVal, 512)) return 0;
    ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" -l %s"), arrcRetVal);

    // Supernode port
    if (!GetRegDword(hkey, TEXT("supernode_port"), &dwRetVal)) return 0;
    ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(":%d"), dwRetVal);

    // device name
    const int nInfoBufSize = MAX_COMPUTERNAME_LENGTH * 16;
    TCHAR  arrcHostName[nInfoBufSize];
    DWORD  dwBufCount = nInfoBufSize;

    // Get and display the name of the computer.
    // TODO: support No-ASCII hostname
    if (GetComputerName(arrcHostName, &dwBufCount))
    {
        HKEY hkeyHostName;
        WCHAR arrcRegHostname[nInfoBufSize] = { 0 };
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
                            NULL, KEY_READ, &hkeyHostName) == ERROR_SUCCESS && \
                            (GetRegString(hkeyHostName, TEXT("hostname"), arrcRegHostname, 512)))
        {
            lstrcpynW(arrcHostName, arrcRegHostname, lstrlenW(arrcRegHostname) + 1);
        }
    }

    if (IsValidAsciiString(arrcHostName) || (!IsValidAsciiString(arrcHostName) && StripNoAsciiString(arrcHostName)))
    {
        ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" -I %s"), arrcHostName);
    }
    else
    {
        ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" -I unknown"));
    }

    //custom param
    if (!GetRegString(hkey, TEXT("custom_param"), arrcRetVal, MAX_COMMAND_LINE_LEN)) return 0;
    ptr += swprintf_s(ptr, dwCommandBufLen - (ptr - pszCommandLine), TEXT(" %s"), arrcRetVal);
    return 1;
}
