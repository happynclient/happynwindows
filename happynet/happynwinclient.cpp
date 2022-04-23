#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>

#include "maingui.h"
#include "resource.h"
#include "service.h"
#include "net.h"
#include "registry.h"
#include "process.h"
#include "tray.h"

#pragma comment(lib, "comctl32.lib")

#define is_item_checked(x,y) (SendDlgItemMessage(x, y, BM_GETCHECK, 0, 0) == BST_CHECKED)

WCHAR szClassName[] = L"HappynetClient";
HICON h_icon;
HICON h_icon_sm;
HANDLE h_update_main_status_thread;
HANDLE h_mutex = NULL;

bool string_empty(WCHAR* str)
{
	if (wcslen(str) == 0) return true;
	for (int i = 0; i < (int)wcslen(str); i++)
	{
		if (str[i] != ' ') return false;
	}
	return true;
}

bool validate_options(HWND hwndDlg)
{
	WCHAR tmp_buf[256];
	WCHAR err_str[256];
	int buf_len = 256;
	bool ret_val = true;

	// IP Address
	GetDlgItemText(hwndDlg, IDC_EDT_IPADDRESS, tmp_buf, buf_len);
	if (is_item_checked(hwndDlg, IDC_CHK_IPADDRESS) && !validate_ipv4_address(tmp_buf))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_IPADDRESS));
		wcscpy_s(err_str, 256, L"Invalid IP address");
		ret_val = false;
	}

	// Community
	GetDlgItemText(hwndDlg, IDC_EDT_COMMUNITY, tmp_buf, buf_len);
	if (string_empty(tmp_buf))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_COMMUNITY));
		wcscpy_s(err_str, 256, L"Community is required");
		ret_val = false;
	}

	// Encryption key
	GetDlgItemText(hwndDlg, IDC_EDT_ENCKEY, tmp_buf, buf_len);
	if (is_item_checked(hwndDlg, IDC_CHK_ENCKEY) && string_empty(tmp_buf))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_ENCKEY));
		wcscpy_s(err_str, 256, L"Encryption key is required");
		ret_val = false;
	}

	// Key file
	GetDlgItemText(hwndDlg, IDC_EDT_KEYFILE, tmp_buf, buf_len);
	if (is_item_checked(hwndDlg, IDC_CHK_KEYFILE) && string_empty(tmp_buf))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_KEYFILE));
		wcscpy_s(err_str, 256, L"Key file is required");
		ret_val = false;
	}

	// Supernode port
	GetDlgItemText(hwndDlg, IDC_EDT_SUPERNODEPORT, tmp_buf, buf_len);
	if (!validate_number_range(tmp_buf, 1, 65535))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_SUPERNODEPORT));
		wcscpy_s(err_str, 256, L"Invalid supernode port");
		ret_val = false;
	}

	// MTU
	GetDlgItemText(hwndDlg, IDC_EDT_MTU, tmp_buf, buf_len);
	if (is_item_checked(hwndDlg, IDC_CHK_MTU) && string_empty(tmp_buf))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_MTU));
		wcscpy_s(err_str, 256, L"MTU is required");
		ret_val = false;
	}

	// Local port
	GetDlgItemText(hwndDlg, IDC_EDT_LOCALPORT, tmp_buf, buf_len);
	if (is_item_checked(hwndDlg, IDC_CHK_LOCALPORT) && !validate_number_range(tmp_buf, 1, 65535))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_LOCALPORT));
		wcscpy_s(err_str, 256, L"Invalid local port");
		ret_val = false;
	}

	// MAC address
	GetDlgItemText(hwndDlg, IDC_EDT_MACADDRESS, tmp_buf, buf_len);
	if (is_item_checked(hwndDlg, IDC_CHK_MACADDRESS) && !validate_mac_address(tmp_buf))
	{
		SetFocus(GetDlgItem(hwndDlg, IDC_EDT_MACADDRESS));
		wcscpy_s(err_str, 256, L"Invalid MAC address");
		ret_val = false;
	}

	// Finished
	if (!ret_val)
	{
		MessageBox(hwndDlg, err_str, L"Error", MB_OK | MB_ICONSTOP);
	}
	return ret_val;
}

void update_addresses(HWND hwndDlg)
{
	if (get_service_status() == STILL_ACTIVE)
	{
		WCHAR ip_address[16];
		WCHAR mac_address[18];
		get_addresses(ip_address, mac_address);
		SetDlgItemText(hwndDlg, IDC_EDT_CUR_IP, ip_address);
		SetDlgItemText(hwndDlg, IDC_EDT_CUR_MAC, mac_address);
	}
	else
	{
		SetDlgItemText(hwndDlg, IDC_EDT_CUR_IP, L"0.0.0.0");
		SetDlgItemText(hwndDlg, IDC_EDT_CUR_MAC, L"00:00:00:00:00:00");
	}
}

void sync_service_output_text( const HWND &hwnd, TCHAR *newText )
{
	// get edit control from dialog
	HWND hwnd_output = GetDlgItem( hwnd, IDC_EDT_EDGE_OUTPUT );

	// get new length to determine buffer size
	int out_length = GetWindowTextLength( hwnd_output ) + lstrlen(newText) + 1;

	// create buffer to hold current and new text
	WCHAR * buf = ( WCHAR * ) GlobalAlloc( GPTR, out_length * sizeof(WCHAR) );
	if (!buf) return;

	// get existing text from edit control and put into buffer
	GetWindowText(hwnd_output, buf, out_length);

	// append the newText to the buffer
	_tcscat_s(buf, out_length, newText);

	// Set the text in the edit control and scroll the end
	SetWindowText(hwnd_output, buf);
	SendMessage(hwnd_output, WM_VSCROLL, SB_BOTTOM, 0L);

	// free the buffer
	GlobalFree( buf );
}

void update_service_status(HWND hwndDlg)
{
	WaitForSingleObject(h_mutex, 1000);

	HWND btn_start = GetDlgItem(hwndDlg, IDC_BTN_START);
	HWND btn_stop = GetDlgItem(hwndDlg, IDC_BTN_STOP);
	HWND btn_ad_settings = GetDlgItem(hwndDlg, IDC_BTN_AD_SETTINGS);
	WCHAR read_buf[BUFSIZE] = {'\0'};
	DWORD service_status = get_service_status();
	switch (service_status)
	{
	case PROCESS_EXIT_CODE:
		SetDlgItemText(hwndDlg, IDC_STC_SRV_STATUS, L"Stopped");
		EnableWindow(btn_start, TRUE);
		EnableWindow(btn_stop, FALSE);
		break;
	case STILL_ACTIVE:
		SetDlgItemText(hwndDlg, IDC_STC_SRV_STATUS, L"Started");
		EnableWindow(btn_start, FALSE);
		EnableWindow(btn_stop, TRUE);
		get_service_process_output(read_buf);
		sync_service_output_text(hwndDlg, read_buf);
		break;
	default:
		SetDlgItemText(hwndDlg, IDC_STC_SRV_STATUS, L"Unknown");
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
		update_service_status(hwndDlg);
		update_addresses(hwndDlg);
		Sleep(500);
	}
	log_event(L"thread end here\n");
	return 0;
}


void read_options(HWND hwndDlg)
{
	WCHAR tmp_buf[256];
	DWORD buf_len = 256;
	DWORD dword_buf;
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Happynet\\Parameters", NULL, KEY_READ, &hkey) != ERROR_SUCCESS)
	{
		MessageBox(hwndDlg, L"The registry key could not be opened.", L"Error", MB_OK | MB_ICONSTOP);
		return;
	}
	// Community
	reg_get_string(hkey, L"community", tmp_buf, buf_len);
	SetDlgItemText(hwndDlg, IDC_EDT_COMMUNITY, tmp_buf);

	// Encryption key
	reg_get_string(hkey, L"enckey", tmp_buf, buf_len);
	SetDlgItemText(hwndDlg, IDC_EDT_ENCKEY, tmp_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_ENCKEY, BM_SETCHECK, (string_empty(tmp_buf) ? BST_UNCHECKED : BST_CHECKED), 0);
	EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_ENCKEY), !string_empty(tmp_buf));
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_KEYFILE), string_empty(tmp_buf));

	// IP address
	reg_get_string(hkey, L"ip_address", tmp_buf, buf_len);
	SetDlgItemText(hwndDlg, IDC_EDT_IPADDRESS, tmp_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_IPADDRESS, BM_SETCHECK, (string_empty(tmp_buf) ? BST_UNCHECKED : BST_CHECKED), 0);
	EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_IPADDRESS), !string_empty(tmp_buf));

	// Key file
	reg_get_string(hkey, L"keyfile", tmp_buf, buf_len);
	SetDlgItemText(hwndDlg, IDC_EDT_KEYFILE, tmp_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_KEYFILE, BM_SETCHECK, (string_empty(tmp_buf) ? BST_UNCHECKED : BST_CHECKED), 0);
	EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_KEYFILE), !string_empty(tmp_buf));
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_ENCKEY), string_empty(tmp_buf));

	// Local Port
	reg_get_dword(hkey, L"local_port", &dword_buf);
	SetDlgItemInt(hwndDlg, IDC_EDT_LOCALPORT, dword_buf, FALSE);
	SendDlgItemMessage(hwndDlg, IDC_CHK_LOCALPORT, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);
	EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_LOCALPORT), !string_empty(tmp_buf));

	// MAC address
	reg_get_string(hkey, L"mac_address", tmp_buf, buf_len);
	SetDlgItemText(hwndDlg, IDC_EDT_MACADDRESS, tmp_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_MACADDRESS, BM_SETCHECK, (string_empty(tmp_buf) ? BST_UNCHECKED : BST_CHECKED), 0);
	EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_MACADDRESS), !string_empty(tmp_buf));

	// MTU
	reg_get_dword(hkey, L"mtu", &dword_buf);
	SetDlgItemInt(hwndDlg, IDC_EDT_MTU, dword_buf, FALSE);
	SendDlgItemMessage(hwndDlg, IDC_CHK_MTU, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);
	EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_MTU), dword_buf);

	// Multicast
	reg_get_dword(hkey, L"multicast", &dword_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_MULTICAST, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

	// Packet Forwarding
	reg_get_dword(hkey, L"packet_forwarding", &dword_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_PKTFORWARD, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

	// header_encry
	reg_get_dword(hkey, L"header_encry", &dword_buf);
	SendDlgItemMessage(hwndDlg, IDC_HEADER_ENCRY, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

	reg_get_dword(hkey, L"data_compress", &dword_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_DATA_COMPRESS, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

	// Supernode address
	reg_get_string(hkey, L"supernode_addr", tmp_buf, buf_len);
	SetDlgItemText(hwndDlg, IDC_EDT_SUPERNODEADDR, tmp_buf);

	// Supernode port
	reg_get_dword(hkey, L"supernode_port", &dword_buf);
	SetDlgItemInt(hwndDlg, IDC_EDT_SUPERNODEPORT, dword_buf, FALSE);

	//custom param
	reg_get_string(hkey, L"custom_param", tmp_buf, buf_len);
	SetDlgItemText(hwndDlg, IDC_EDT_CUSTOM_PARAM, tmp_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_CUSTOM_PARAM, BM_SETCHECK, (string_empty(tmp_buf) ? BST_UNCHECKED : BST_CHECKED), 0);
	EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_CUSTOM_PARAM), !string_empty(tmp_buf));

	// auto_start
	reg_get_dword(hkey, L"auto_start", &dword_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_AUTO_START, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);

	// auto_tray
	reg_get_dword(hkey, L"auto_tray", &dword_buf);
	SendDlgItemMessage(hwndDlg, IDC_CHK_AUTO_TRAY, BM_SETCHECK, (dword_buf == 0 ? BST_UNCHECKED : BST_CHECKED), 0);
}

void save_options(HWND hwndDlg)
{
	if (!validate_options(hwndDlg)) return;
	WCHAR tmp_buf[256];
	DWORD buf_len = 256;
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Happynet\\Parameters", NULL, KEY_READ | KEY_WRITE, &hkey) != ERROR_SUCCESS)
	{
		MessageBox(hwndDlg, L"The registry key could not be opened.", L"Error", MB_OK | MB_ICONSTOP);
		return;
	}
	// Community
	GetDlgItemText(hwndDlg, IDC_EDT_COMMUNITY, tmp_buf, buf_len);
	reg_set_string(hkey, L"community", tmp_buf);

	// Encryption key
	if (is_item_checked(hwndDlg, IDC_CHK_ENCKEY))
	{
		GetDlgItemText(hwndDlg, IDC_EDT_ENCKEY, tmp_buf, buf_len);
		reg_set_string(hkey, L"enckey", tmp_buf);
	}
	else
	{
		reg_set_string(hkey, L"enckey", L"");
	}

	// IP Address
	if (is_item_checked(hwndDlg, IDC_CHK_IPADDRESS))
	{
		GetDlgItemText(hwndDlg, IDC_EDT_IPADDRESS, tmp_buf, buf_len);
		reg_set_string(hkey, L"ip_address", tmp_buf);
	}
	else
	{
		reg_set_string(hkey, L"ip_address", L"");
	}


	// Key file
	if (is_item_checked(hwndDlg, IDC_CHK_KEYFILE))
	{
		GetDlgItemText(hwndDlg, IDC_EDT_KEYFILE, tmp_buf, buf_len);
		reg_set_string(hkey, L"keyfile", tmp_buf);
	}
	else
	{
		reg_set_string(hkey, L"keyfile", L"");
	}

	// Local port
	if (is_item_checked(hwndDlg, IDC_CHK_LOCALPORT))
	{
		GetDlgItemText(hwndDlg, IDC_EDT_LOCALPORT, tmp_buf, buf_len);
		reg_set_dword(hkey, L"local_port", (DWORD)_wtoi(tmp_buf));
	}
	else
	{
		reg_set_dword(hkey, L"local_port", 0);
	}

	// MAC address
	if (is_item_checked(hwndDlg, IDC_CHK_MACADDRESS))
	{
		GetDlgItemText(hwndDlg, IDC_EDT_MACADDRESS, tmp_buf, buf_len);
		reg_set_string(hkey, L"mac_address", tmp_buf);
	}
	else
	{
		reg_set_string(hkey, L"mac_address", L"");
	}

	// MTU
	if (is_item_checked(hwndDlg, IDC_CHK_MTU))
	{
		GetDlgItemText(hwndDlg, IDC_EDT_MTU, tmp_buf, buf_len);
		reg_set_dword(hkey, L"mtu", (DWORD)_wtoi(tmp_buf));
	}
	else
	{
		reg_set_dword(hkey, L"mtu", 0);
	}

	// Multicast
	reg_set_dword(hkey, L"multicast", (is_item_checked(hwndDlg, IDC_CHK_MULTICAST) ? 1 : 0));

	// Packet Forwarding
	reg_set_dword(hkey, L"packet_forwarding", (is_item_checked(hwndDlg, IDC_CHK_PKTFORWARD) ? 1 : 0));

	// header encry
	reg_set_dword(hkey, L"header_encry", (is_item_checked(hwndDlg, IDC_HEADER_ENCRY) ? 1 : 0));

	// data compress
	reg_set_dword(hkey, L"data_compress", (is_item_checked(hwndDlg, IDC_CHK_DATA_COMPRESS) ? 1 : 0));

	// Supernode address
	GetDlgItemText(hwndDlg, IDC_EDT_SUPERNODEADDR, tmp_buf, buf_len);
	reg_set_string(hkey, L"supernode_addr", tmp_buf);

	// Supernode port
	GetDlgItemText(hwndDlg, IDC_EDT_SUPERNODEPORT, tmp_buf, buf_len);
	reg_set_dword(hkey, L"supernode_port", (DWORD)_wtoi(tmp_buf));

	//custom param
	if (is_item_checked(hwndDlg, IDC_CHK_CUSTOM_PARAM))
	{
		GetDlgItemText(hwndDlg, IDC_EDT_CUSTOM_PARAM, tmp_buf, buf_len);
		reg_set_string(hkey, L"custom_param", tmp_buf);
	}
	else
	{
		reg_set_string(hkey, L"custom_param", L"");
	}

	// auto start
	reg_set_dword(hkey, L"auto_start", (is_item_checked(hwndDlg, IDC_CHK_AUTO_START) ? 1 : 0));
	if (is_item_checked(hwndDlg, IDC_CHK_AUTO_START)) {
		set_auto_start_service();
	} else {
		cancel_auto_start_service();
	}
	// auto tray
	reg_set_dword(hkey, L"auto_tray", (is_item_checked(hwndDlg, IDC_CHK_AUTO_TRAY) ? 1 : 0));


	// Finished
	RegCloseKey(hkey);
}

void handle_command_event(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (HIWORD(wParam != BN_CLICKED)) return;
	static HINSTANCE hInstance;
	switch (LOWORD(wParam))
	{
	case WM_CREATE:
		hInstance = ((LPCREATESTRUCT)lParam)->hInstance;
		break;

	case IDC_BTN_AD_SETTINGS:
		//DialogBox(hInstance, TEXT("Adwanced Settings"), hwndDlg, ad_settings_dialog_proc);
		DialogBox(hInstance, MAKEINTRESOURCE(IDD_AD_SETTINGS), hwndDlg, ad_settings_dialog_proc);
		break;
	case IDC_BTN_START:
		save_options(hwndDlg);
		start_service();
		update_service_status(hwndDlg);
		update_addresses(hwndDlg);
		break;
	case IDC_BTN_STOP:
		stop_service();
		update_service_status(hwndDlg);
		update_addresses(hwndDlg);
		break;
	case IDC_BTN_SAVE:
		save_options(hwndDlg);
		MessageBox(NULL, TEXT("已成功保存"), TEXT("保存当前设置"), MB_OK| MB_ICONINFORMATION);
		break;

	case IDC_BTN_EXIT:
		stop_service();
		EndDialog(hwndDlg, NULL);
		break;
	case IDC_CHK_IPADDRESS:
		{
			bool checked = is_item_checked(hwndDlg, IDC_CHK_IPADDRESS);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_IPADDRESS), checked);
			// SendDlgItemMessage(hwndDlg, IDC_CHK_PKTFORWARD, BM_SETCHECK, (checked ? BST_UNCHECKED : BST_CHECKED), 0);
			break;
		}
	case IDC_CHK_ENCKEY:
		{
			bool checked = is_item_checked(hwndDlg, IDC_CHK_ENCKEY);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_ENCKEY), checked);
			EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_KEYFILE), !checked);
			break;
		}
	case IDC_CHK_KEYFILE:
		{
			bool checked = is_item_checked(hwndDlg, IDC_CHK_KEYFILE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_KEYFILE), checked);
			EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_ENCKEY), !checked);
			break;
		}
	case IDC_CHK_MTU:
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_MTU), is_item_checked(hwndDlg, IDC_CHK_MTU));
		break;
	case IDC_CHK_CUSTOM_PARAM:
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_CUSTOM_PARAM), is_item_checked(hwndDlg, IDC_CHK_CUSTOM_PARAM));
		break;
	case IDC_CHK_LOCALPORT:
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_LOCALPORT), is_item_checked(hwndDlg, IDC_CHK_LOCALPORT));
		break;
	case IDC_CHK_MACADDRESS:
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDT_MACADDRESS), is_item_checked(hwndDlg, IDC_CHK_MACADDRESS));
		break;
	}
}

void setup_system_menu(HWND hwndDlg)
{
	HMENU sys_menu = GetSystemMenu(hwndDlg, FALSE);
	AppendMenu(sys_menu, MF_SEPARATOR, NULL, NULL);
	AppendMenu(sys_menu, MF_STRING, IDM_ABOUT, L"About HappynetClient..");
}

INT_PTR CALLBACK dialog_proc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			update_service_status(hwndDlg);
			update_addresses(hwndDlg);
			read_options(hwndDlg);

			if (is_item_checked(hwndDlg, IDC_CHK_AUTO_START)) {
				HWND hbtn_start = GetDlgItem(hwndDlg, IDC_BTN_START);
				SendMessage(hbtn_start, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(0, 0));
				SendMessage(hbtn_start, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(0, 0));
			}

			if (is_item_checked(hwndDlg, IDC_CHK_AUTO_TRAY)) {
				PostMessage(hwndDlg, WM_SYSCOMMAND, SC_MINIMIZE, 0);
				ShowWindow(hwndDlg, SW_HIDE);
			}
			break;
		}

	case WM_COMMAND:
		{
			handle_command_event(hwndDlg, uMsg, wParam, lParam);
			break;
		}

	case WM_SYSCOMMAND:
		{
			if (wParam == IDM_ABOUT)
			{
				MessageBox(hwndDlg, L"Happynet Version 0.7", L"About HappynetClient", MB_OK | MB_ICONINFORMATION);
				break;
			}
			return FALSE;
			break;
		}

	case WM_SIZE:
		{
			if (wParam == SIZE_MINIMIZED && is_item_checked(hwndDlg, IDC_CHK_AUTO_TRAY)) {
				hide_to_tray(hwndDlg);
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
			stop_service();

			// stop thread
			CloseHandle(h_mutex);
			CloseHandle(h_update_main_status_thread);

			destroy_tray(hwndDlg);
			PostQuitMessage(0);
			break;
		}

	default:
		return FALSE;
	}
	return TRUE;
}

INT_PTR CALLBACK ad_settings_dialog_proc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{		
		return FALSE;
	}

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDOK:
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



int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Initialize
	INITCOMMONCONTROLSEX icx;
	icx.dwSize = sizeof(INITCOMMONCONTROLSEX);
	InitCommonControlsEx(&icx);

	h_icon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON32), IMAGE_ICON, 32, 32, 0);
	h_icon_sm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON32), IMAGE_ICON, 16, 16, 0);

	// Run GUI window
	INT_PTR res = DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, dialog_proc);

	return 0;
}
