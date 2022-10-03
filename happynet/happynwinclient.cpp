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
#include "tray.h"
#include "utils.h"

#pragma comment(lib, "comctl32.lib")

WCHAR szClassName[] = TEXT("HappynetClient");
HICON h_icon;
HICON h_icon_sm;
HANDLE h_update_main_status_thread;
HANDLE h_mutex = NULL;
CNetworkAdapter *m_pAdapters = NULL;


BOOL ValidateOptions(HWND hwndDlg)
{
	WCHAR tmp_buf[MAX_COMMAND_LINE_LEN];
	WCHAR err_str[MAX_COMMAND_LINE_LEN];
	int buf_len = MAX_COMMAND_LINE_LEN;
	bool ret_val = true;

	// IP Address
	GetDlgItemText(hwndDlg, IDC_EDT_IPADDRESS, tmp_buf, buf_len);
	if (IsItemChecked(hwndDlg, IDC_CHK_IPADDRESS) && !ValidateIpv4Address(tmp_buf))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_IPADDRESS));
		wcscpy_s(err_str, MAX_COMMAND_LINE_LEN, L"Invalid IP address");
		ret_val = false;
	}

	// Community
	GetDlgItemText(hwndDlg, IDC_EDT_COMMUNITY, tmp_buf, buf_len);
	if (IsEmptyString(tmp_buf))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_COMMUNITY));
		wcscpy_s(err_str, MAX_COMMAND_LINE_LEN, L"Community is required");
		ret_val = false;
	}

	// Encryption key
	GetDlgItemText(hwndDlg, IDC_EDT_ENCKEY, tmp_buf, buf_len);
	if (IsItemChecked(hwndDlg, IDC_CHK_ENCKEY) && IsEmptyString(tmp_buf))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_ENCKEY));
		wcscpy_s(err_str, MAX_COMMAND_LINE_LEN, L"Encryption key is required");
		ret_val = false;
	}

	// Supernode port
	GetDlgItemText(hwndDlg, IDC_EDT_SUPERNODEPORT, tmp_buf, buf_len);
	if (!ValidateNumberRange(tmp_buf, 1, 65535))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_SUPERNODEPORT));
		wcscpy_s(err_str, MAX_COMMAND_LINE_LEN, L"Invalid supernode port");
		ret_val = false;
	}

	// Finished
	if (!ret_val)
	{
		MessageBox(hwndDlg, err_str, TEXT("Error"), MB_OK | MB_ICONSTOP);
	}
	return ret_val;
}


BOOL validate_ad_options(HWND hwndDlg)
{
	WCHAR tmp_buf[MAX_COMMAND_LINE_LEN];
	WCHAR err_str[MAX_COMMAND_LINE_LEN];
	INT buf_len = MAX_COMMAND_LINE_LEN;
	BOOL ret_val = TRUE;


	// Key file
	GetDlgItemText(hwndDlg, IDC_EDT_KEYFILE, tmp_buf, buf_len);
	if (IsItemChecked(hwndDlg, IDC_CHK_KEYFILE) && IsEmptyString(tmp_buf))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_KEYFILE));
		wcscpy_s(err_str, MAX_COMMAND_LINE_LEN, TEXT("Key file is required"));
		ret_val = FALSE;
	}

	// MTU
	GetDlgItemText(hwndDlg, IDC_EDT_MTU, tmp_buf, buf_len);
	if (IsItemChecked(hwndDlg, IDC_CHK_MTU) && IsEmptyString(tmp_buf))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_MTU));
		wcscpy_s(err_str, MAX_COMMAND_LINE_LEN, TEXT("MTU is required"));
		ret_val = FALSE;
	}

	// Local port
	GetDlgItemText(hwndDlg, IDC_EDT_LOCALPORT, tmp_buf, buf_len);
	if (IsItemChecked(hwndDlg, IDC_CHK_LOCALPORT) && !ValidateNumberRange(tmp_buf, 1, 65535))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_LOCALPORT));
		wcscpy_s(err_str, MAX_COMMAND_LINE_LEN, TEXT("Invalid local port"));
		ret_val = FALSE;
	}

	// MAC address
	GetDlgItemText(hwndDlg, IDC_EDT_MACADDRESS, tmp_buf, buf_len);
	if (IsItemChecked(hwndDlg, IDC_CHK_MACADDRESS) && !ValidateMacAddress(tmp_buf))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_MACADDRESS));
		wcscpy_s(err_str, MAX_COMMAND_LINE_LEN, TEXT("Invalid MAC address"));
		ret_val = FALSE;
	}

	// Finished
	if (!ret_val)
	{
		MessageBox(hwndDlg, err_str, TEXT("Error"), MB_OK | MB_ICONSTOP);
	}
	return ret_val;
}


VOID UpdateAddressesInfo(HWND hwndDlg)
{
	if (GetServiceStatus() == STILL_ACTIVE)
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

VOID sync_service_output_text(const HWND &hwnd)
{
    TCHAR read_buf[PROCESS_STDOUT_BUFSIZE] = { 0 };

    if (IsSetSystemService()) {

        GetSystemServiceOutput(read_buf);
    }
    else {
        GetProcessServiceOutput(read_buf);
    }

	// get edit control from dialog
	HWND hwnd_output = GetDlgItem( hwnd, IDC_EDT_EDGE_OUTPUT );

	// get new length to determine buffer size
	int out_length = GetWindowTextLength( hwnd_output ) + lstrlen(read_buf) + 1;

	// create buffer to hold current and new text
	WCHAR * buf = ( WCHAR * ) GlobalAlloc( GPTR, out_length * sizeof(WCHAR) );
	if (!buf) return;

	// get existing text from edit control and put into buffer
	GetWindowText(hwnd_output, buf, out_length);

	// append the newText to the buffer
	_tcscat_s(buf, out_length, read_buf);

	// Set the text in the edit control and scroll the end
	SetWindowText(hwnd_output, buf);
	SendMessage(hwnd_output, WM_VSCROLL, SB_BOTTOM, 0L);

	// free the buffer
	GlobalFree( buf );
}

VOID UpdateServiceStatus(HWND hwndDlg)
{
	WaitForSingleObject(h_mutex, 1000);

	HWND btn_start = GetDlgItem(hwndDlg, IDC_BTN_START);
	HWND btn_stop = GetDlgItem(hwndDlg, IDC_BTN_STOP);
    HWND btn_monitor = GetDlgItem(hwndDlg, IDC_BTN_MONITOR);
	HWND btn_ad_settings = GetDlgItem(hwndDlg, IDC_BTN_AD_SETTINGS);
	DWORD service_status = GetServiceStatus();
	switch (service_status)
	{
	case PROCESS_EXIT_CODE:
		SetDlgItemText(hwndDlg, IDC_STC_SRV_STATUS, TEXT("Stopped"));
		EnableWindow(btn_start, TRUE);
		EnableWindow(btn_stop, FALSE);
        EnableWindow(btn_monitor, FALSE);
		break;
	case STILL_ACTIVE:
		SetDlgItemText(hwndDlg, IDC_STC_SRV_STATUS, TEXT("Started"));
		EnableWindow(btn_start, FALSE);
		EnableWindow(btn_stop, TRUE);
        EnableWindow(btn_monitor, TRUE);
		sync_service_output_text(hwndDlg);
		break;
	default:
		SetDlgItemText(hwndDlg, IDC_STC_SRV_STATUS, TEXT("Unknown"));
		EnableWindow(btn_start, TRUE);
		EnableWindow(btn_stop, TRUE);
		break;
	}
	ReleaseMutex(h_mutex);
}


DWORD CALLBACK update_main_status_thread(PVOID pvoid)
{
	HWND hwndDlg = (HWND) pvoid;
	while (1) {
		UpdateServiceStatus(hwndDlg);
		UpdateAddressesInfo(hwndDlg);
		Sleep(1000);
	}
	LogEvent(TEXT("thread end here\n"));
	return 0;
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

	// Supernode address
	GetRegString(hkey, TEXT("supernode_addr"), tmp_buf, buf_len);
	SetDlgItemText(hwndDlg, IDC_EDT_SUPERNODEADDR, tmp_buf);

	// Supernode port
	GetRegDword(hkey, TEXT("supernode_port"), &dword_buf);
	SetDlgItemInt(hwndDlg, IDC_EDT_SUPERNODEPORT, dword_buf, FALSE);
	RegCloseKey(hkey);
}

VOID  read_ad_options(HWND hwndDlg)
{
	WCHAR tmp_buf[MAX_COMMAND_LINE_LEN];
	DWORD buf_len = MAX_COMMAND_LINE_LEN;
	DWORD dword_buf;
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Happynet\\Parameters"),
                        NULL, KEY_READ, &hkey) != ERROR_SUCCESS)
	{
		MessageBox(hwndDlg, TEXT("The registry key could not be opened."), TEXT("Error"), MB_OK | MB_ICONSTOP);
		return;
	}

	// Local Port
	GetRegDword(hkey, TEXT("local_port"), &dword_buf);
	SetDlgItemInt(hwndDlg, IDC_EDT_LOCALPORT, dword_buf, FALSE);
	SendDlgItemMessage(hwndDlg, IDC_CHK_LOCALPORT, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);
	EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_LOCALPORT), dword_buf != 0);

	// Key file
	GetRegString(hkey, TEXT("keyfile"), tmp_buf, buf_len);
	SetDlgItemText(hwndDlg, IDC_EDT_KEYFILE, tmp_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_KEYFILE, BM_SETCHECK, (IsEmptyString(tmp_buf) ? BST_UNCHECKED : BST_CHECKED), 0);
	EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_KEYFILE), !IsEmptyString(tmp_buf));
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_ENCKEY), IsEmptyString(tmp_buf));

    // Adapter
    GetRegString(hkey, TEXT("adapter"), tmp_buf, buf_len);
    SetDlgItemText(hwndDlg, IDC_COMBO_ADAPTERS, tmp_buf);
    SendDlgItemMessage(hwndDlg, IDC_CHK_ADAPTERS, BM_SETCHECK, (IsEmptyString(tmp_buf) ? BST_UNCHECKED : BST_CHECKED), 0);
    EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO_ADAPTERS), !IsEmptyString(tmp_buf));


	// MAC address
	GetRegString(hkey, TEXT("mac_address"), tmp_buf, buf_len);
	SetDlgItemText(hwndDlg, IDC_EDT_MACADDRESS, tmp_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_MACADDRESS, BM_SETCHECK, (IsEmptyString(tmp_buf) ? BST_UNCHECKED : BST_CHECKED), 0);
	EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_MACADDRESS), !IsEmptyString(tmp_buf));

	// MTU
	GetRegDword(hkey, TEXT("mtu"), &dword_buf);
	SetDlgItemInt(hwndDlg, IDC_EDT_MTU, dword_buf, FALSE);
	SendDlgItemMessage(hwndDlg, IDC_CHK_MTU, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);
	EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_MTU), dword_buf);

	// Multicast
	GetRegDword(hkey, TEXT("multicast"), &dword_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_MULTICAST, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

	// Packet Forwarding
	GetRegDword(hkey, TEXT("packet_forwarding"), &dword_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_PKTFORWARD, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

	// header_encry
	GetRegDword(hkey, TEXT("header_encry"), &dword_buf);
	SendDlgItemMessage(hwndDlg, IDC_HEADER_ENCRY, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

	GetRegDword(hkey, TEXT("data_compress"), &dword_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_DATA_COMPRESS, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

	GetRegDword(hkey, TEXT("select_rtt"), &dword_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_SELECT_RTT, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

	//custom param
	GetRegString(hkey, TEXT("custom_param"), tmp_buf, buf_len);
	SetDlgItemText(hwndDlg, IDC_EDT_CUSTOM_PARAM, tmp_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_CUSTOM_PARAM, BM_SETCHECK, (IsEmptyString(tmp_buf) ? BST_UNCHECKED : BST_CHECKED), 0);
	EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_CUSTOM_PARAM), !IsEmptyString(tmp_buf));

    // system_service
    GetRegDword(hkey, TEXT("system_service"), &dword_buf);
    SendDlgItemMessage(hwndDlg, IDC_CHK_SYSTEM_SERVICE, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

	// auto_start
	GetRegDword(hkey, TEXT("auto_start"), &dword_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_AUTO_START, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

	// auto_tray
	GetRegDword(hkey, TEXT("auto_tray"), &dword_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_AUTO_TRAY, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

	RegCloseKey(hkey);
}


VOID SaveOptions(HWND hwndDlg)
{
	if (!ValidateOptions(hwndDlg)) return;
	WCHAR tmp_buf[MAX_COMMAND_LINE_LEN];
	DWORD buf_len = MAX_COMMAND_LINE_LEN;
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Happynet\\Parameters"),
                        NULL, KEY_READ | KEY_WRITE, &hkey) != ERROR_SUCCESS)
	{
		MessageBox(hwndDlg, TEXT("The registry key could not be opened."), TEXT("Error"), MB_OK | MB_ICONSTOP);
		return;
	}
	// Community
	GetDlgItemText(hwndDlg, IDC_EDT_COMMUNITY, tmp_buf, buf_len);
	SetRegString(hkey, TEXT("community"), tmp_buf);

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
}

VOID save_ad_options(HWND hwndDlg)
{
	if (!validate_ad_options(hwndDlg)) return;
	WCHAR tmp_buf[MAX_COMMAND_LINE_LEN];
	DWORD buf_len = MAX_COMMAND_LINE_LEN;
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
		GetDlgItemText(hwndDlg, IDC_EDT_KEYFILE, tmp_buf, buf_len);
		SetRegString(hkey, TEXT("keyfile"), tmp_buf);
	}
	else
	{
		SetRegString(hkey, TEXT("keyfile"), TEXT(""));
	}


    // TAP Adapter
    if (IsItemChecked(hwndDlg, IDC_CHK_ADAPTERS))
    {
        GetDlgItemText(hwndDlg, IDC_COMBO_ADAPTERS, tmp_buf, buf_len);
        SetRegString(hkey, TEXT("adapter"), tmp_buf);

        // set adapter net interface name to HAPPYNET by uuid
        WCHAR *strtok_buf = NULL, *adapter_id = NULL;
        const WCHAR s[4] = TEXT("_");
        adapter_id = wcstok_s(tmp_buf, s, &strtok_buf);
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
		GetDlgItemText(hwndDlg, IDC_EDT_MACADDRESS, tmp_buf, buf_len);
		SetRegString(hkey, TEXT("mac_address"), tmp_buf);
	}
	else
	{
		SetRegString(hkey, TEXT("mac_address"), TEXT(""));
	}

	// MTU
	if (IsItemChecked(hwndDlg, IDC_CHK_MTU))
	{
		GetDlgItemText(hwndDlg, IDC_EDT_MTU, tmp_buf, buf_len);
		SetRegDword(hkey, TEXT("mtu"), (DWORD)_wtoi(tmp_buf));
	}
	else
	{
		SetRegDword(hkey, TEXT("mtu"), 0);
	}

	// Local port
	if (IsItemChecked(hwndDlg, IDC_CHK_LOCALPORT))
	{
		GetDlgItemText(hwndDlg, IDC_EDT_LOCALPORT, tmp_buf, buf_len);
		SetRegDword(hkey, TEXT("local_port"), (DWORD)_wtoi(tmp_buf));
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
		GetDlgItemText(hwndDlg, IDC_EDT_CUSTOM_PARAM, tmp_buf, buf_len);
		SetRegString(hkey, TEXT("custom_param"), tmp_buf);
	}
	else
	{
		SetRegString(hkey, TEXT("custom_param"), TEXT(""));
	}

    // set system service
    SetRegDword(hkey, TEXT("system_service"), (IsItemChecked(hwndDlg, IDC_CHK_SYSTEM_SERVICE) ? 1 : 0));
    if (IsItemChecked(hwndDlg, IDC_CHK_SYSTEM_SERVICE)) {
        RegSystemService();
    }
    else {
        UnregSystemService();
    }

	// auto start
	SetRegDword(hkey, TEXT("auto_start"), (IsItemChecked(hwndDlg, IDC_CHK_AUTO_START) ? 1 : 0));
	if (IsItemChecked(hwndDlg, IDC_CHK_AUTO_START)) {
        SetServiceAutoStart();
	}
	else {
        UnsetServiceAutoStart();
	}
	// auto tray
	SetRegDword(hkey, TEXT("auto_tray"), (IsItemChecked(hwndDlg, IDC_CHK_AUTO_TRAY) ? 1 : 0));


	// Finished
	RegCloseKey(hkey);
}



VOID HandleCommandEvent(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (HIWORD(wParam != BN_CLICKED)) return;
	static HINSTANCE hInstance;
	switch (LOWORD(wParam))
	{
	case WM_CREATE:
		hInstance = ((LPCREATESTRUCT)lParam)->hInstance;
		break;

	case IDC_BTN_AD_SETTINGS:
		DialogBox(hInstance, MAKEINTRESOURCE(IDD_AD_SETTINGS), hwndDlg, AdSettingsDialogProc);
		break;
	case IDC_BTN_START:
		SaveOptions(hwndDlg);
		StartService();
		UpdateServiceStatus(hwndDlg);
		UpdateAddressesInfo(hwndDlg);
		break;
	case IDC_BTN_STOP:
		StopService();
		UpdateServiceStatus(hwndDlg);
		UpdateAddressesInfo(hwndDlg);
        sync_service_output_text(hwndDlg);
		break;

    case IDC_BTN_MONITOR:
        WinExec("happynmonitor.exe", SW_SHOW);
        break;
	/*
	case IDC_BTN_SAVE:
		save_options(hwndDlg);
		MessageBox(NULL, TEXT("已成功保存"), TEXT("保存当前设置"), MB_OK| MB_ICONINFORMATION);
		break;
	*/

	case IDC_BTN_EXIT:
		StopService();
		EndDialog(hwndDlg, NULL);
		break;



	case IDC_CHK_IPADDRESS:
		{
			bool checked = IsItemChecked(hwndDlg, IDC_CHK_IPADDRESS);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_IPADDRESS), checked);
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

VOID setup_system_menu(HWND hwndDlg)
{
	HMENU sys_menu = GetSystemMenu(hwndDlg, FALSE);
	AppendMenu(sys_menu, MF_SEPARATOR, NULL, NULL);
	AppendMenu(sys_menu, MF_STRING, IDM_ABOUT, TEXT("About HappynetClient.."));
}

INT_PTR CALLBACK MainDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			setup_system_menu(hwndDlg);
			LOGFONT lfont;
			HWND hwnd_ip, hwnd_mac;
			hwnd_ip = GetDlgItem(hwndDlg, IDC_EDT_CUR_IP);
			hwnd_mac = GetDlgItem(hwndDlg, IDC_EDT_CUR_MAC);
			HFONT hfont = (HFONT)SendMessage(hwnd_ip, WM_GETFONT, 0, 0);
			GetObject(hfont, sizeof(lfont), &lfont);
			lfont.lfWeight = FW_BOLD;
			hfont = CreateFontIndirect(&lfont);
			SendMessage(hwnd_ip, WM_SETFONT, (WPARAM)hfont, 0);
			SendMessage(hwnd_mac, WM_SETFONT, (WPARAM)hfont, 0);
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)h_icon);
			SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)h_icon_sm);

			h_mutex = CreateMutex(NULL, FALSE, NULL);

			h_update_main_status_thread = CreateThread(NULL, 0, update_main_status_thread, hwndDlg, 0, NULL);
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
				MessageBox(hwndDlg, TEXT("Happynet Version 1.0.6"), TEXT("About HappynetClient"),
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

			DestroyWindow(hwndDlg);
			break;
		}

	case WM_DESTROY:
		{
			StopService();

			// stop thread
			CloseHandle(h_mutex);
			CloseHandle(h_update_main_status_thread);

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
		read_ad_options(hwndDlg);
		break;
	}

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_CHK_KEYFILE:
		{
			bool checked = IsItemChecked(hwndDlg, IDC_CHK_KEYFILE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_KEYFILE), checked);
			//EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_ENCKEY), !checked);
			break;
		}

        case IDC_CHK_ADAPTERS:
        {
            HWND hwndCombo = GetDlgItem(hwndDlg, IDC_COMBO_ADAPTERS);
            bool checked = IsItemChecked(hwndDlg, IDC_CHK_ADAPTERS);
            EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO_ADAPTERS), checked);

            if(!checked) {
                ComboBox_SetText(hwndCombo, _T("Auto Detect"));
                break;
            }

            // set adapters info
            DWORD	dwErr = 0;
            ULONG	ulNeeded = 0;
            UINT	m_nCount = 0;           

            dwErr = EnumNetworkAdapters(m_pAdapters, 0, &ulNeeded);
            if (dwErr == ERROR_INSUFFICIENT_BUFFER) {
                m_nCount = ulNeeded / sizeof(CNetworkAdapter);
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
            for (UINT m_nDisplay = 0; m_nDisplay < m_nCount; m_nDisplay++) {
                CNetworkAdapter* pAdapt = &m_pAdapters[m_nDisplay];   

                if (pAdapt->GetAdapterDescription().rfind(TEXT("TAP"), 0) != 0) {
                    continue;
                }
                else {
                    TCHAR adapter_name[512] = { 0 };                    
                    swprintf_s(adapter_name, TEXT("%s_%s"), pAdapt->GetAdapterDescription().c_str(), pAdapt->GetAdapterName().c_str());
                    SendMessage(hwndCombo, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)adapter_name);
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
			save_ad_options(hwndDlg);
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

	h_icon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON32), IMAGE_ICON, 32, 32, 0);
	h_icon_sm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON32), IMAGE_ICON, 16, 16, 0);

	// Run GUI window
	INT_PTR res = DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDialogProc);

	return 0;
}
