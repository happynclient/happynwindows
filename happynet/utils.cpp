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


BOOL IsValidAsciiString(WCHAR *line)
{
	for (INT i = 0; i < lstrlenW(line); i++)
	{
		if (!iswascii(line[i])) {
			return FALSE;
		}
	}
	return TRUE;
}

BOOL StripNoAsciiString(WCHAR *line)
{
	INT i = 0;
	for (i = 0; i < lstrlenW(line); i++)
	{
		if (!iswascii(line[i])) {
			line[i] = L'\0';
			break;
		}
	}
	return i;
}

UINT WinExecW(WCHAR* command_line, UINT command_show)
{
    USES_CONVERSION;
    return WinExec(W2A(command_line), command_show);
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

INT GetInstallDirPath(WCHAR* dir_path, DWORD buf_len)
{
    WCHAR exe_dir_buf[MAX_PATH] = { 0 };
    DWORD exe_dir_buf_len = buf_len * sizeof(WCHAR);
    
    // get happyn exe dir path
    if (!GetModuleFileName(NULL, exe_dir_buf, MAX_PATH))
    {
        return 0;
    }
    PathRemoveFileSpec(exe_dir_buf);
    swprintf_s(dir_path, buf_len, exe_dir_buf);
    return 1;
}


INT GetAppDatapath(WCHAR* datapath)
{
    WCHAR default_app_datapath[MAX_PATH] = { 0 };
    SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, default_app_datapath);
    swprintf_s(datapath, MAX_PATH, TEXT("%s\\happynet"), default_app_datapath);
    return SHCreateDirectoryEx(NULL, datapath, NULL);
}


INT GetEdgeCmdLine(WCHAR* dir_path, WCHAR* command_line, DWORD buf_len)
{
    WCHAR edge_path[MAX_PATH] = { 0 };
    swprintf_s(edge_path, MAX_PATH, TEXT("\"%s\\happynedge.exe\""), dir_path);
    return GetEdgeParams(edge_path, command_line, buf_len);
}


INT GetEdgeParams(WCHAR* edge_path, WCHAR* command_line, DWORD buf_len)
{
    WCHAR ret_val[MAX_COMMAND_LINE_LEN];
    DWORD ret_dword = 0;

    // Use 'ptr' to append to the end of the command line
    WCHAR* ptr = command_line;
    ptr += swprintf_s(command_line, buf_len, TEXT("%s "), edge_path);

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
    if (!GetRegString(hkey, TEXT("community"), ret_val, 512)) return 0;
    ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" -c %s"), ret_val);

    // Encryption key
    if (!GetRegString(hkey, TEXT("enckey"), ret_val, 512)) return 0;
    if (wcslen(ret_val) != 0)
    {
        ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" -k %s"), ret_val);
    }

    // IP address
    if (!GetRegString(hkey, TEXT("ip_address"), ret_val, 512)) return 0;
    if (!GetRegDword(hkey, TEXT("packet_forwarding"), &ret_dword)) return 0;
    if (wcslen(ret_val) != 0 && ret_dword == 0)
    {
        ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" -a %s"), ret_val);
    }

    if (wcslen(ret_val) == 0 && ret_dword == 0)
    {
        ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" -r -a dhcp:0.0.0.0"));
    }

    // Encryption key file
    if (!GetRegString(hkey, TEXT("keyfile"), ret_val, 512)) return 0;
    if (wcslen(ret_val) != 0)
    {
        ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" -K %s"), ret_val);
    }

    // set adapter
    if (!GetRegString(hkey, TEXT("adapter"), ret_val, 512)) return 0;
    if (wcslen(ret_val) != 0)
    {
        CHAR *adapter_id = NULL;
        const CHAR s[2] = "_";
        _bstr_t b(ret_val);
        CHAR *tmpbuf = NULL;
        adapter_id = strtok_s(b, s, &tmpbuf);
        adapter_id = strtok_s(NULL, s, &tmpbuf);

        TCHAR adapter_firendly_name[512] = { 0 };
        if (GetAdapterFriendlyName(adapter_id, adapter_firendly_name, 512) == NOERROR) {
            ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" -d \"%s\""), adapter_firendly_name);
        }
    }

    // Local Port
    if (!GetRegDword(hkey, TEXT("local_port"), &ret_dword)) return 0;
    if (ret_dword != 0)
    {
        ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" -p %d"), ret_dword);
    }


    // MAC address
    if (!GetRegString(hkey, TEXT("mac_address"), ret_val, 512)) return 0;
    if (wcslen(ret_val) != 0)
    {
        ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" -m %s"), ret_val);
    }

    // MTU
    if (!GetRegDword(hkey, TEXT("mtu"), &ret_dword)) return 0;
    if (ret_dword != 0)
    {
        ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" -M %d"), ret_dword);
    }

    // Multicast
    if (!GetRegDword(hkey, TEXT("multicast"), &ret_dword)) return 0;
    if (ret_dword != 0)
    {
        ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" -E"));
    }

    // Packet forwarding
    if (!GetRegDword(hkey, TEXT("packet_forwarding"), &ret_dword)) return 0;
    if (ret_dword != 0)
    {
        ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" -r"));
    }

    // header encryption
    if (!GetRegDword(hkey, TEXT("header_encry"), &ret_dword)) return 0;
    if (ret_dword != 0)
    {
        ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" -H"));
    }

    // data compress
    if (!GetRegDword(hkey, TEXT("data_compress"), &ret_dword)) return 0;
    if (ret_dword != 0)
    {
        ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" -z1"));
    }

    // select rtt
    if (!GetRegDword(hkey, TEXT("select_rtt"), &ret_dword)) return 0;
    if (ret_dword != 0)
    {
        ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" --select-rtt "));
    }

    // Supernode address
    if (!GetRegString(hkey, TEXT("supernode_addr"), ret_val, 512)) return 0;
    ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" -l %s"), ret_val);

    // Supernode port
    if (!GetRegDword(hkey, TEXT("supernode_port"), &ret_dword)) return 0;
    ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(":%d"), ret_dword);

    // device name
    const int info_buf_size = MAX_COMPUTERNAME_LENGTH * 16;
    TCHAR  hostname[info_buf_size];
    DWORD  buf_char_count = info_buf_size;

    // Get and display the name of the computer.
    // TODO: support No-ASCII hostname
    if (GetComputerName(hostname, &buf_char_count))
    {
        HKEY hkey_hostname;
        WCHAR reg_hostname[info_buf_size] = { 0 };
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
                            NULL, KEY_READ, &hkey_hostname) == ERROR_SUCCESS && \
                            (GetRegString(hkey_hostname, TEXT("hostname"), reg_hostname, 512)))
        {
            lstrcpynW(hostname, reg_hostname, lstrlenW(reg_hostname) + 1);
        }
    }

    if (IsValidAsciiString(hostname) || (!IsValidAsciiString(hostname) && StripNoAsciiString(hostname)))
    {
        ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" -I %s"), hostname);
    }
    else
    {
        ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" -I unknown"));
    }

    //custom param
    if (!GetRegString(hkey, TEXT("custom_param"), ret_val, MAX_COMMAND_LINE_LEN)) return 0;
    ptr += swprintf_s(ptr, buf_len - (ptr - command_line), TEXT(" %s"), ret_val);
    return 1;
}
