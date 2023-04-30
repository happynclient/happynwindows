#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <windowsx.h>
#include <tchar.h>

#include "maingui.h"
#include "netadapter.h"
#include "netinterface.h"
#include "registry.h"
#include "process.h"
#include "resource.h"
#include "service.h"
#include "systemsrv.h"
#include "sinstance.h"
#include "tray.h"
#include "utils.h"

#pragma comment(lib, "comctl32.lib")

static WCHAR m_szClassName[] = TEXT("HappynetClient");
static WCHAR m_szHappynVersion[] = TEXT("Happynet Version 1.7.0");
static HICON m_hIcon;
static HICON m_hIconSm;
static HANDLE m_hUpdateMainStatusThread;
static HANDLE m_hMutex = NULL;
static CNetworkAdapter *m_pAdapters = NULL;
static CInstanceChecker m_InstanceChecker{ _T("{H84F50BD-59DF-43F4-A8F9-6C83EDB9CAE5}") };


static DWORD CALLBACK UpdateMainStatusThread(PVOID pvoid)
{
    HWND hwndDlg = (HWND)pvoid;
    while (1) {
        UpdateServiceStatus(hwndDlg);
        UpdateAddressesInfo(hwndDlg);
        Sleep(1000);
    }
    LogEvent(TEXT("thread end here\n"));
    return 0;
}

static VOID SetupSystemMenu(HWND hwndDlg)
{
    HMENU hSysMenu = GetSystemMenu(hwndDlg, FALSE);
    AppendMenu(hSysMenu, MF_SEPARATOR, NULL, NULL);
    AppendMenu(hSysMenu, MF_STRING, IDM_ABOUT, TEXT("About HappynetClient.."));
}


static VOID SyncServiceOutputText(const HWND &hwnd)
{
    TCHAR arrcReadBuf[PROCESS_STDOUT_BUFSIZE] = { 0 };

    if (IsSetSystemService()) {

        GetSystemServiceOutput(arrcReadBuf);
    }
    else {
        GetProcessServiceOutput(arrcReadBuf);
    }

    // get edit control from dialog
    HWND hwnd_output = GetDlgItem(hwnd, IDC_EDT_EDGE_OUTPUT);

    // get new length to determine buffer size
    int out_length = GetWindowTextLength(hwnd_output) + lstrlen(arrcReadBuf) + 1;

    // create buffer to hold current and new text
    WCHAR * buf = (WCHAR *)GlobalAlloc(GPTR, out_length * sizeof(WCHAR));
    if (!buf) return;

    // get existing text from edit control and put into buffer
    GetWindowText(hwnd_output, buf, out_length);

    // append the newText to the buffer
    _tcscat_s(buf, out_length, arrcReadBuf);

    // Set the text in the edit control and scroll the end
    SetWindowText(hwnd_output, buf);
    SendMessage(hwnd_output, WM_VSCROLL, SB_BOTTOM, 0L);

    // free the buffer
    GlobalFree(buf);
}

static BOOL ValidateAdOptions(HWND hwndDlg)
{
    WCHAR arrcAdOptionBuf[MAX_COMMAND_LINE_LEN] = { 0 };
    WCHAR arrcErrString[MAX_COMMAND_LINE_LEN] = { 0 };
    INT nBufLen = MAX_COMMAND_LINE_LEN;
    BOOL bSuccess = TRUE;

    // Key file
    GetDlgItemText(hwndDlg, IDC_EDT_KEYFILE, arrcAdOptionBuf, nBufLen);
    if (IsItemChecked(hwndDlg, IDC_CHK_KEYFILE) && IsEmptyString(arrcAdOptionBuf))
    {
        SetFocus(GetDlgItem(hwndDlg, IDC_EDT_KEYFILE));
        wcscpy_s(arrcErrString, MAX_COMMAND_LINE_LEN, TEXT("Key file is required"));
        bSuccess = FALSE;
    }

    // MTU
    GetDlgItemText(hwndDlg, IDC_EDT_MTU, arrcAdOptionBuf, nBufLen);
    if (IsItemChecked(hwndDlg, IDC_CHK_MTU) && IsEmptyString(arrcAdOptionBuf))
    {
        SetFocus(GetDlgItem(hwndDlg, IDC_EDT_MTU));
        wcscpy_s(arrcErrString, MAX_COMMAND_LINE_LEN, TEXT("MTU is required"));
        bSuccess = FALSE;
    }

    // Local port
    GetDlgItemText(hwndDlg, IDC_EDT_LOCALPORT, arrcAdOptionBuf, nBufLen);
    if (IsItemChecked(hwndDlg, IDC_CHK_LOCALPORT) && !ValidateNumberRange(arrcAdOptionBuf, 1, 65535))
    {
        SetFocus(GetDlgItem(hwndDlg, IDC_EDT_LOCALPORT));
        wcscpy_s(arrcErrString, MAX_COMMAND_LINE_LEN, TEXT("Invalid local port"));
        bSuccess = FALSE;
    }

    // MAC address
    GetDlgItemText(hwndDlg, IDC_EDT_MACADDRESS, arrcAdOptionBuf, nBufLen);
    if (IsItemChecked(hwndDlg, IDC_CHK_MACADDRESS) && !ValidateMacAddress(arrcAdOptionBuf))
    {
        SetFocus(GetDlgItem(hwndDlg, IDC_EDT_MACADDRESS));
        wcscpy_s(arrcErrString, MAX_COMMAND_LINE_LEN, TEXT("Invalid MAC address"));
        bSuccess = FALSE;
    }

    // Finished
    if (!bSuccess)
    {
        MessageBox(hwndDlg, arrcErrString, TEXT("Error"), MB_OK | MB_ICONSTOP);
    }
    return bSuccess;
}

static VOID ReadAdOptions(HWND hwndDlg)
{
    WCHAR arrcTmpAdOptionBuf[MAX_COMMAND_LINE_LEN];
    DWORD dwBufLen = MAX_COMMAND_LINE_LEN;
    DWORD dwBuf;
    HKEY hkey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Happynet\\Parameters"),
        NULL, KEY_READ, &hkey) != ERROR_SUCCESS)
    {
        MessageBox(hwndDlg, TEXT("The registry key could not be opened."), TEXT("Error"), MB_OK | MB_ICONSTOP);
        return;
    }

    // Local Port
    GetRegDword(hkey, TEXT("local_port"), &dwBuf);
    SetDlgItemInt(hwndDlg, IDC_EDT_LOCALPORT, dwBuf, FALSE);
    SendDlgItemMessage(hwndDlg, IDC_CHK_LOCALPORT, BM_SETCHECK, (dwBuf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);
    EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_LOCALPORT), dwBuf != 0);

    // Key file
    GetRegString(hkey, TEXT("keyfile"), arrcTmpAdOptionBuf, dwBufLen);
    SetDlgItemText(hwndDlg, IDC_EDT_KEYFILE, arrcTmpAdOptionBuf);
    SendDlgItemMessage(hwndDlg, IDC_CHK_KEYFILE, BM_SETCHECK, (IsEmptyString(arrcTmpAdOptionBuf) ? BST_UNCHECKED : BST_CHECKED), 0);
    EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_KEYFILE), !IsEmptyString(arrcTmpAdOptionBuf));
    EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_ENCKEY), IsEmptyString(arrcTmpAdOptionBuf));

    // Adapter
    GetRegString(hkey, TEXT("adapter"), arrcTmpAdOptionBuf, dwBufLen);
    SetDlgItemText(hwndDlg, IDC_COMBO_ADAPTERS, arrcTmpAdOptionBuf);
    SendDlgItemMessage(hwndDlg, IDC_CHK_ADAPTERS, BM_SETCHECK, (IsEmptyString(arrcTmpAdOptionBuf) ? BST_UNCHECKED : BST_CHECKED), 0);
    EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO_ADAPTERS), !IsEmptyString(arrcTmpAdOptionBuf));


    // MAC address
    GetRegString(hkey, TEXT("mac_address"), arrcTmpAdOptionBuf, dwBufLen);
    SetDlgItemText(hwndDlg, IDC_EDT_MACADDRESS, arrcTmpAdOptionBuf);
    SendDlgItemMessage(hwndDlg, IDC_CHK_MACADDRESS, BM_SETCHECK, (IsEmptyString(arrcTmpAdOptionBuf) ? BST_UNCHECKED : BST_CHECKED), 0);
    EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_MACADDRESS), !IsEmptyString(arrcTmpAdOptionBuf));

    // MTU
    GetRegDword(hkey, TEXT("mtu"), &dwBuf);
    SetDlgItemInt(hwndDlg, IDC_EDT_MTU, dwBuf, FALSE);
    SendDlgItemMessage(hwndDlg, IDC_CHK_MTU, BM_SETCHECK, (dwBuf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);
    EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_MTU), dwBuf);

    // Multicast
    GetRegDword(hkey, TEXT("multicast"), &dwBuf);
    SendDlgItemMessage(hwndDlg, IDC_CHK_MULTICAST, BM_SETCHECK, (dwBuf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

    // Packet Forwarding
    GetRegDword(hkey, TEXT("packet_forwarding"), &dwBuf);
    SendDlgItemMessage(hwndDlg, IDC_CHK_PKTFORWARD, BM_SETCHECK, (dwBuf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

    // header_encry
    GetRegDword(hkey, TEXT("header_encry"), &dwBuf);
    SendDlgItemMessage(hwndDlg, IDC_HEADER_ENCRY, BM_SETCHECK, (dwBuf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

    GetRegDword(hkey, TEXT("data_compress"), &dwBuf);
    SendDlgItemMessage(hwndDlg, IDC_CHK_DATA_COMPRESS, BM_SETCHECK, (dwBuf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

    GetRegDword(hkey, TEXT("select_rtt"), &dwBuf);
    SendDlgItemMessage(hwndDlg, IDC_CHK_SELECT_RTT, BM_SETCHECK, (dwBuf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

    //custom param
    GetRegString(hkey, TEXT("custom_param"), arrcTmpAdOptionBuf, dwBufLen);
    SetDlgItemText(hwndDlg, IDC_EDT_CUSTOM_PARAM, arrcTmpAdOptionBuf);
    SendDlgItemMessage(hwndDlg, IDC_CHK_CUSTOM_PARAM, BM_SETCHECK, (IsEmptyString(arrcTmpAdOptionBuf) ? BST_UNCHECKED : BST_CHECKED), 0);
    EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_CUSTOM_PARAM), !IsEmptyString(arrcTmpAdOptionBuf));

    // system_service
    GetRegDword(hkey, TEXT("system_service"), &dwBuf);
    SendDlgItemMessage(hwndDlg, IDC_CHK_SYSTEM_SERVICE, BM_SETCHECK, (dwBuf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

    // auto_start
    GetRegDword(hkey, TEXT("auto_start"), &dwBuf);
    SendDlgItemMessage(hwndDlg, IDC_CHK_AUTO_START, BM_SETCHECK, (dwBuf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

    // auto_tray
    GetRegDword(hkey, TEXT("auto_tray"), &dwBuf);
    SendDlgItemMessage(hwndDlg, IDC_CHK_AUTO_TRAY, BM_SETCHECK, (dwBuf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

    RegCloseKey(hkey);
}


static VOID SaveAdOptions(HWND hwndDlg)
{
    if (!ValidateAdOptions(hwndDlg)) return;
    WCHAR arrcTmpAdOptionBuf[MAX_COMMAND_LINE_LEN];
    DWORD dwBufLen = MAX_COMMAND_LINE_LEN;
    HKEY hkey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Happynet\\Parameters"),
        NULL, KEY_READ | KEY_WRITE, &hkey) != ERROR_SUCCESS)
    {
        MessageBox(hwndDlg, TEXT("The registry key could not be opened."), TEXT("Error"), MB_OK | MB_ICONSTOP);
        return;
    }

    // Key file
    if (IsItemChecked(hwndDlg, IDC_CHK_KEYFILE))
    {
        GetDlgItemText(hwndDlg, IDC_EDT_KEYFILE, arrcTmpAdOptionBuf, dwBufLen);
        SetRegString(hkey, TEXT("keyfile"), arrcTmpAdOptionBuf);
    }
    else
    {
        SetRegString(hkey, TEXT("keyfile"), TEXT(""));
    }


    // TAP Adapter
    if (IsItemChecked(hwndDlg, IDC_CHK_ADAPTERS))
    {
        GetDlgItemText(hwndDlg, IDC_COMBO_ADAPTERS, arrcTmpAdOptionBuf, dwBufLen);
        SetRegString(hkey, TEXT("adapter"), arrcTmpAdOptionBuf);

        // set adapter net interface name to HAPPYNET by uuid
        WCHAR *strtok_buf = NULL, *adapter_id = NULL;
        const WCHAR s[4] = TEXT("_");
        adapter_id = wcstok_s(arrcTmpAdOptionBuf, s, &strtok_buf);
        adapter_id = wcstok_s(NULL, s, &strtok_buf);
        SetNetinterfaceNameById(adapter_id);
    }
    else
    {
        SetRegString(hkey, TEXT("adapter"), TEXT(""));
    }

    // MAC address
    if (IsItemChecked(hwndDlg, IDC_CHK_MACADDRESS))
    {
        GetDlgItemText(hwndDlg, IDC_EDT_MACADDRESS, arrcTmpAdOptionBuf, dwBufLen);
        SetRegString(hkey, TEXT("mac_address"), arrcTmpAdOptionBuf);
    }
    else
    {
        SetRegString(hkey, TEXT("mac_address"), TEXT(""));
    }

    // MTU
    if (IsItemChecked(hwndDlg, IDC_CHK_MTU))
    {
        GetDlgItemText(hwndDlg, IDC_EDT_MTU, arrcTmpAdOptionBuf, dwBufLen);
        SetRegDword(hkey, TEXT("mtu"), (DWORD)_wtoi(arrcTmpAdOptionBuf));
    }
    else
    {
        SetRegDword(hkey, TEXT("mtu"), 0);
    }

    // Local port
    if (IsItemChecked(hwndDlg, IDC_CHK_LOCALPORT))
    {
        GetDlgItemText(hwndDlg, IDC_EDT_LOCALPORT, arrcTmpAdOptionBuf, dwBufLen);
        SetRegDword(hkey, TEXT("local_port"), (DWORD)_wtoi(arrcTmpAdOptionBuf));
    }
    else
    {
        SetRegDword(hkey, TEXT("local_port"), 0);
    }

    // Multicast
    SetRegDword(hkey, TEXT("multicast"), (IsItemChecked(hwndDlg, IDC_CHK_MULTICAST) ? 1 : 0));

    // Packet Forwarding
    SetRegDword(hkey, TEXT("packet_forwarding"), (IsItemChecked(hwndDlg, IDC_CHK_PKTFORWARD) ? 1 : 0));

    // header encry
    SetRegDword(hkey, TEXT("header_encry"), (IsItemChecked(hwndDlg, IDC_HEADER_ENCRY) ? 1 : 0));

    // data compress
    SetRegDword(hkey, TEXT("data_compress"), (IsItemChecked(hwndDlg, IDC_CHK_DATA_COMPRESS) ? 1 : 0));

    // select rtt
    SetRegDword(hkey, TEXT("select_rtt"), (IsItemChecked(hwndDlg, IDC_CHK_SELECT_RTT) ? 1 : 0));

    //custom param
    if (IsItemChecked(hwndDlg, IDC_CHK_CUSTOM_PARAM))
    {
        GetDlgItemText(hwndDlg, IDC_EDT_CUSTOM_PARAM, arrcTmpAdOptionBuf, dwBufLen);
        SetRegString(hkey, TEXT("custom_param"), arrcTmpAdOptionBuf);
    }
    else
    {
        SetRegString(hkey, TEXT("custom_param"), TEXT(""));
    }

    // set system service
    if (IsItemChecked(hwndDlg, IDC_CHK_SYSTEM_SERVICE)) {
        // if running at normal mode
        StopHappynetService();
        RegSystemService();
    }
    else {
        UnregSystemService();
    }
    SetRegDword(hkey, TEXT("system_service"), (IsItemChecked(hwndDlg, IDC_CHK_SYSTEM_SERVICE) ? 1 : 0));

    // auto start
    SetRegDword(hkey, TEXT("auto_start"), (IsItemChecked(hwndDlg, IDC_CHK_AUTO_START) ? 1 : 0));
    if (IsItemChecked(hwndDlg, IDC_CHK_AUTO_START)) {
        SetHappynetServiceAutoStart();
    }
    else {
        UnsetHappynetServiceAutoStart();
    }
    // auto tray
    SetRegDword(hkey, TEXT("auto_tray"), (IsItemChecked(hwndDlg, IDC_CHK_AUTO_TRAY) ? 1 : 0));


    // Finished
    RegCloseKey(hkey);
}


BOOL ValidateOptions(HWND hwndDlg)
{
    WCHAR arrcOptionTmpBuf[MAX_COMMAND_LINE_LEN] = { 0 };
    WCHAR arrcErrString[MAX_COMMAND_LINE_LEN] = { 0 };
    INT nBufLen = MAX_COMMAND_LINE_LEN;
	BOOL bSuccess = TRUE;

	// IP Address
	GetDlgItemText(hwndDlg, IDC_EDT_IPADDRESS, arrcOptionTmpBuf, nBufLen);
	if (IsItemChecked(hwndDlg, IDC_CHK_IPADDRESS) && !ValidateIpv4Address(arrcOptionTmpBuf))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_IPADDRESS));
		wcscpy_s(arrcErrString, MAX_COMMAND_LINE_LEN, TEXT("Invalid IP address"));
		bSuccess = FALSE;
	}

	// Community
	GetDlgItemText(hwndDlg, IDC_EDT_COMMUNITY, arrcOptionTmpBuf, nBufLen);
	if (IsEmptyString(arrcOptionTmpBuf))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_COMMUNITY));
		wcscpy_s(arrcErrString, MAX_COMMAND_LINE_LEN, TEXT("Community is required"));
		bSuccess = FALSE;
	}

	// Encryption key
	GetDlgItemText(hwndDlg, IDC_EDT_ENCKEY, arrcOptionTmpBuf, nBufLen);
	if (IsItemChecked(hwndDlg, IDC_CHK_ENCKEY) && IsEmptyString(arrcOptionTmpBuf))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_ENCKEY));
		wcscpy_s(arrcErrString, MAX_COMMAND_LINE_LEN, TEXT("Encryption key is required"));
		bSuccess = FALSE;
	}

	// Supernode port
	GetDlgItemText(hwndDlg, IDC_EDT_SUPERNODEPORT, arrcOptionTmpBuf, nBufLen);
	if (!ValidateNumberRange(arrcOptionTmpBuf, 1, 65535))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_SUPERNODEPORT));
		wcscpy_s(arrcErrString, MAX_COMMAND_LINE_LEN, TEXT("Invalid supernode port"));
		bSuccess = FALSE;
	}

	// Finished
	if (!bSuccess)
	{
		MessageBox(hwndDlg, arrcErrString, TEXT("Error"), MB_OK | MB_ICONSTOP);
	}
	return bSuccess;
}


VOID UpdateAddressesInfo(HWND hwndDlg)
{
	if (GetHappynetServiceStatus() == STILL_ACTIVE)
	{
		WCHAR ip_address[16];
		WCHAR mac_address[18];
		GetIpMacAddresses(ip_address, mac_address);
		SetDlgItemText(hwndDlg, IDC_EDT_CUR_IP, ip_address);
		SetDlgItemText(hwndDlg, IDC_EDT_CUR_MAC, mac_address);
	}
	else
	{
		SetDlgItemText(hwndDlg, IDC_EDT_CUR_IP, TEXT("0.0.0.0"));
		SetDlgItemText(hwndDlg, IDC_EDT_CUR_MAC, TEXT("00:00:00:00:00:00"));
	}
}


VOID UpdateServiceStatus(HWND hwndDlg)
{
	WaitForSingleObject(m_hMutex, 1000);

	HWND hBtnStart = GetDlgItem(hwndDlg, IDC_BTN_START);
	HWND hBtnStop = GetDlgItem(hwndDlg, IDC_BTN_STOP);
    HWND hBtnMonitor = GetDlgItem(hwndDlg, IDC_BTN_MONITOR);
	HWND hBtnAdSettings = GetDlgItem(hwndDlg, IDC_BTN_AD_SETTINGS);
	DWORD dwServiceStatus = GetHappynetServiceStatus();
	switch (dwServiceStatus)
	{
	case PROCESS_EXIT_CODE:
		SetDlgItemText(hwndDlg, IDC_STC_SRV_STATUS, TEXT("Stopped"));
		EnableWindow(hBtnStart, TRUE);
		EnableWindow(hBtnStop, FALSE);
        EnableWindow(hBtnMonitor, FALSE);
		break;
	case STILL_ACTIVE:
		SetDlgItemText(hwndDlg, IDC_STC_SRV_STATUS, TEXT("Started"));
		EnableWindow(hBtnStart, FALSE);
		EnableWindow(hBtnStop, TRUE);
        EnableWindow(hBtnMonitor, TRUE);
		SyncServiceOutputText(hwndDlg);
		break;
	default:
		SetDlgItemText(hwndDlg, IDC_STC_SRV_STATUS, TEXT("Unknown"));
		EnableWindow(hBtnStart, TRUE);
		EnableWindow(hBtnStop, TRUE);
		break;
	}
	ReleaseMutex(m_hMutex);
}


VOID  ReadOptions(HWND hwndDlg)
{
	WCHAR tmp_buf[MAX_COMMAND_LINE_LEN];
	DWORD buf_len = MAX_COMMAND_LINE_LEN;
	DWORD dword_buf;
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Happynet\\Parameters"),
                        NULL, KEY_READ, &hkey) != ERROR_SUCCESS)
	{
		MessageBox(hwndDlg, TEXT("The registry key could not be opened."),
                    TEXT("Error"), MB_OK | MB_ICONSTOP);
		return;
	}
	// Community
	GetRegString(hkey, TEXT("community"), tmp_buf, buf_len);
	SetDlgItemText(hwndDlg, IDC_EDT_COMMUNITY, tmp_buf);

	// Encryption key
	GetRegString(hkey, TEXT("enckey"), tmp_buf, buf_len);
	SetDlgItemText(hwndDlg, IDC_EDT_ENCKEY, tmp_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_ENCKEY, BM_SETCHECK, (IsEmptyString(tmp_buf) ? BST_UNCHECKED : BST_CHECKED), 0);
	EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_ENCKEY), !IsEmptyString(tmp_buf));
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_KEYFILE), IsEmptyString(tmp_buf));

	// IP address
	GetRegString(hkey, TEXT("ip_address"), tmp_buf, buf_len);
	SetDlgItemText(hwndDlg, IDC_EDT_IPADDRESS, tmp_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_IPADDRESS, BM_SETCHECK, (IsEmptyString(tmp_buf) ? BST_UNCHECKED : BST_CHECKED), 0);
	EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_IPADDRESS), !IsEmptyString(tmp_buf));
    if (IsEmptyString(tmp_buf)) {
        SetDlgItemText(hwndDlg, IDC_EDT_IPADDRESS, TEXT(AUTOIP_TEXT));
    }

	// Supernode address
	GetRegString(hkey, TEXT("supernode_addr"), tmp_buf, buf_len);
	SetDlgItemText(hwndDlg, IDC_EDT_SUPERNODEADDR, tmp_buf);

	// Supernode port
	GetRegDword(hkey, TEXT("supernode_port"), &dword_buf);
	SetDlgItemInt(hwndDlg, IDC_EDT_SUPERNODEPORT, dword_buf, FALSE);
	RegCloseKey(hkey);
}


BOOL SaveOptions(HWND hwndDlg)
{
    if (!ValidateOptions(hwndDlg)) {
        return FALSE;
    }
	WCHAR tmp_buf[MAX_COMMAND_LINE_LEN];
	DWORD buf_len = MAX_COMMAND_LINE_LEN;
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Happynet\\Parameters"),
                        NULL, KEY_READ | KEY_WRITE, &hkey) != ERROR_SUCCESS)
	{
		MessageBox(hwndDlg, TEXT("The registry key could not be opened."), TEXT("Error"), MB_OK | MB_ICONSTOP);
		return FALSE;
	}
	// Community
	GetDlgItemText(hwndDlg, IDC_EDT_COMMUNITY, tmp_buf, buf_len);
    WCHAR *strip_tmp_buf = StripString(tmp_buf);
	SetRegString(hkey, TEXT("community"), strip_tmp_buf);

	// Encryption key
	if (IsItemChecked(hwndDlg, IDC_CHK_ENCKEY))
	{
		GetDlgItemText(hwndDlg, IDC_EDT_ENCKEY, tmp_buf, buf_len);
		SetRegString(hkey, TEXT("enckey"), tmp_buf);
	}
	else
	{
		SetRegString(hkey, TEXT("enckey"), TEXT(""));
	}

	// IP Address
	if (IsItemChecked(hwndDlg, IDC_CHK_IPADDRESS))
	{
		GetDlgItemText(hwndDlg, IDC_EDT_IPADDRESS, tmp_buf, buf_len);
		SetRegString(hkey, TEXT("ip_address"), tmp_buf);
	}
	else
	{
		SetRegString(hkey, TEXT("ip_address"), TEXT(""));
	}

	// Supernode address
	GetDlgItemText(hwndDlg, IDC_EDT_SUPERNODEADDR, tmp_buf, buf_len);
	SetRegString(hkey, TEXT("supernode_addr"), tmp_buf);

	// Supernode port
	GetDlgItemText(hwndDlg, IDC_EDT_SUPERNODEPORT, tmp_buf, buf_len);
	SetRegDword(hkey, TEXT("supernode_port"), (DWORD)_wtoi(tmp_buf));


	// Finished
	RegCloseKey(hkey);

    return TRUE;
}


VOID HandleCommandEvent(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (HIWORD(wParam != BN_CLICKED)) return;
	static HINSTANCE hInstance;
    HINSTANCE hInstShellExec;

	switch (LOWORD(wParam))
	{
	case WM_CREATE:
		hInstance = ((LPCREATESTRUCT)lParam)->hInstance;
		break;

	case IDC_BTN_AD_SETTINGS:
		DialogBox(hInstance, MAKEINTRESOURCE(IDD_AD_SETTINGS), hwndDlg, AdSettingsDialogProc);
		break;
	case IDC_BTN_START:
        if (SaveOptions(hwndDlg)) {
            StartHappynetService();
            UpdateServiceStatus(hwndDlg);
            UpdateAddressesInfo(hwndDlg);
        }
		break;
	case IDC_BTN_STOP:
		StopHappynetService();
		UpdateServiceStatus(hwndDlg);
		UpdateAddressesInfo(hwndDlg);
        SyncServiceOutputText(hwndDlg);
		break;

    case IDC_BTN_MONITOR:        
        // Build path and command line parameters
        hInstShellExec = ShellExecute(NULL, TEXT("open"), TEXT("happynmonitor.exe"), NULL, NULL, SW_SHOW);
        if (hInstShellExec <= (HINSTANCE)32)
        {
            WinExec("happynmonitor.exe", SW_SHOW);
        }
        break;
	/*
	case IDC_BTN_SAVE:
		save_options(hwndDlg);
		MessageBox(NULL, TEXT("已成功保存"), TEXT("保存当前设置"), MB_OK| MB_ICONINFORMATION);
		break;
	*/

	case IDC_BTN_EXIT:
		StopHappynetService();
		EndDialog(hwndDlg, NULL);
		break;

	case IDC_CHK_IPADDRESS:
		{
			bool checked = IsItemChecked(hwndDlg, IDC_CHK_IPADDRESS);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_IPADDRESS), checked);
            if (!checked) {
                SetDlgItemText(hwndDlg, IDC_EDT_IPADDRESS, TEXT(AUTOIP_TEXT));
            }
			// SendDlgItemMessage(hwndDlg, IDC_CHK_PKTFORWARD, BM_SETCHECK, (checked ? BST_UNCHECKED : BST_CHECKED), 0);
			break;
		}
	case IDC_CHK_ENCKEY:
		{
			bool checked = IsItemChecked(hwndDlg, IDC_CHK_ENCKEY);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_ENCKEY), checked);
			//EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_KEYFILE), !checked);
			break;
		}
	}
}

INT_PTR CALLBACK MainDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
            m_InstanceChecker.TrackFirstInstanceRunning(hwndDlg);

			SetupSystemMenu(hwndDlg);
			LOGFONT lfont;
			HWND hwndIp, hwndMac;
			hwndIp = GetDlgItem(hwndDlg, IDC_EDT_CUR_IP);
			hwndMac = GetDlgItem(hwndDlg, IDC_EDT_CUR_MAC);
			HFONT hfont = (HFONT)SendMessage(hwndIp, WM_GETFONT, 0, 0);
			GetObject(hfont, sizeof(lfont), &lfont);
			lfont.lfWeight = FW_BOLD;
			hfont = CreateFontIndirect(&lfont);
			SendMessage(hwndIp, WM_SETFONT, (WPARAM)hfont, 0);
			SendMessage(hwndMac, WM_SETFONT, (WPARAM)hfont, 0);
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
			SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

			m_hMutex = CreateMutex(NULL, FALSE, NULL);

			m_hUpdateMainStatusThread = CreateThread(NULL, 0, UpdateMainStatusThread, hwndDlg, 0, NULL);
			UpdateServiceStatus(hwndDlg);
			UpdateAddressesInfo(hwndDlg);
			ReadOptions(hwndDlg);

			if (IsSetAutoStart()) {
				HWND hbtn_start = GetDlgItem(hwndDlg, IDC_BTN_START);
				SendMessage(hbtn_start, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(0, 0));
				SendMessage(hbtn_start, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(0, 0));
			}

			if (IsSetAutoTray()) {
				PostMessage(hwndDlg, WM_SYSCOMMAND, SC_MINIMIZE, 0);
				ShowWindow(hwndDlg, SW_HIDE);
			}
			break;
		}

	case WM_COMMAND:
		{
			HandleCommandEvent(hwndDlg, uMsg, wParam, lParam);
			break;
		}

	case WM_SYSCOMMAND:
		{
			if (wParam == IDM_ABOUT)
			{
				MessageBox(hwndDlg, m_szHappynVersion, TEXT("About HappynetClient"),
                                MB_OK | MB_ICONINFORMATION);
				break;
			}
			return FALSE;
			break;
		}

	case WM_SIZE:
		{
			if (wParam == SIZE_MINIMIZED && IsSetAutoTray()) {
				HideToTray(hwndDlg);
				ShowWindow(hwndDlg, SW_HIDE);
				break;
			}
		}

	case WM_USER_SHELLICON:
		{
			if (lParam == WM_LBUTTONDOWN) {
				SetForegroundWindow(hwndDlg);
				ShowWindow(hwndDlg, SW_SHOWNORMAL);
			}
			break;
		}

	case WM_CLOSE:
		{

            if (GetHappynetServiceStatus() == STILL_ACTIVE) {
                INT nRet = MessageBox(HWND_DESKTOP, TEXT("关闭程序后会终止网络服务，您确定退出吗?"), TEXT("终止服务"), MB_YESNO | MB_ICONWARNING);
                if (nRet == IDYES) {
                        DestroyWindow(hwndDlg);
                }
            }
            else {
                DestroyWindow(hwndDlg);
            }
			break;
		}

	case WM_DESTROY:
		{
			StopHappynetService();

			// stop thread
			CloseHandle(m_hMutex);
			CloseHandle(m_hUpdateMainStatusThread);

			DestroyTray(hwndDlg);
			PostQuitMessage(0);
			break;
		}

	default:
		return FALSE;
	}
	return TRUE;
}

INT_PTR CALLBACK AdSettingsDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{		
        SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);
		ReadAdOptions(hwndDlg);
		break;
	}

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_CHK_KEYFILE:
		{
			BOOL bChecked = IsItemChecked(hwndDlg, IDC_CHK_KEYFILE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_KEYFILE), bChecked);
			//EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_ENCKEY), !checked);
			break;
		}

        case IDC_CHK_ADAPTERS:
        {
            HWND hwndCombo = GetDlgItem(hwndDlg, IDC_COMBO_ADAPTERS);
            BOOL bChecked = IsItemChecked(hwndDlg, IDC_CHK_ADAPTERS);
            EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO_ADAPTERS), bChecked);

            if(!bChecked) {
                ComboBox_SetText(hwndCombo, TEXT("Auto Detect"));
                break;
            }

            // set adapters info
            DWORD	dwErr = 0;
            ULONG	ulNeeded = 0;
            UINT	nCount = 0;           

            dwErr = EnumNetworkAdapters(m_pAdapters, 0, &ulNeeded);
            if (dwErr == ERROR_INSUFFICIENT_BUFFER) {
                nCount = ulNeeded / sizeof(CNetworkAdapter);
                m_pAdapters = new CNetworkAdapter[ulNeeded / sizeof(CNetworkAdapter)];
                dwErr = EnumNetworkAdapters(m_pAdapters, ulNeeded, &ulNeeded);
                if (!m_pAdapters) {
                    // not found adapters
                    break;
                }
            }
            else {
                // not found adapters
                break;
            }

            // set to IDC_COMBO_ADAPTERS            
            SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);
            for (UINT nDisplay = 0; nDisplay < nCount; nDisplay++) {
                CNetworkAdapter* pAdapt = &m_pAdapters[nDisplay];   

                if (pAdapt->GetAdapterDescription().rfind(TEXT("TAP"), 0) != 0) {
                    continue;
                }
                else {
                    TCHAR szAdapterName[512] = { 0 };                    
                    swprintf_s(szAdapterName, TEXT("%s_%s"), pAdapt->GetAdapterDescription().c_str(), pAdapt->GetAdapterName().c_str());
                    SendMessage(hwndCombo, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)szAdapterName);
                }
            }            
            SendMessage(hwndCombo, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
            break;
        }
		case IDC_CHK_MTU:
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_MTU), IsItemChecked(hwndDlg, IDC_CHK_MTU));
			break;
		case IDC_CHK_CUSTOM_PARAM:
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_CUSTOM_PARAM), IsItemChecked(hwndDlg, IDC_CHK_CUSTOM_PARAM));            
			break;
		case IDC_CHK_LOCALPORT:
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_LOCALPORT), IsItemChecked(hwndDlg, IDC_CHK_LOCALPORT));
			break;
		case IDC_CHK_MACADDRESS:
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_MACADDRESS), IsItemChecked(hwndDlg, IDC_CHK_MACADDRESS));
			break;
		
		case IDOK:
			SaveAdOptions(hwndDlg);
			EndDialog(hwndDlg, 0);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			return TRUE;
		}

		break;
	}

	default:
		return FALSE;
	}
	return TRUE;
}


INT WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Initialize
	INITCOMMONCONTROLSEX icx;
	icx.dwSize = sizeof(INITCOMMONCONTROLSEX);
	InitCommonControlsEx(&icx);

	m_hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON32), IMAGE_ICON, 32, 32, 0);
	m_hIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON32), IMAGE_ICON, 16, 16, 0);

    //Check for the previous instance as soon as possible
    if (m_InstanceChecker.PreviousInstanceRunning())
    {
        m_InstanceChecker.ActivatePreviousInstance();
        return FALSE;
    }
    // Run GUI window
	INT_PTR res = DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDialogProc);

	return 0;
}
