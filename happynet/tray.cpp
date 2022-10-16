#include "tray.h"

VOID HideToTray(HWND hWnd)
{
	NOTIFYICONDATA nid;
	nid.cbSize = (DWORD)sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = IDI_ICON32;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_USER_SHELLICON;
	nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON32));
	wcscpy_s(nid.szTip,  TEXT("happynet"));
	Shell_NotifyIcon(NIM_ADD, &nid);
}


VOID DestroyTray(HWND hWnd)
{
	NOTIFYICONDATA nid;
	nid.cbSize = (DWORD)sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = IDI_ICON32;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_USER_SHELLICON;
	nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON32));
	wcscpy_s(nid.szTip, TEXT("happynet"));
	//delete tray icon show
	Shell_NotifyIcon(NIM_DELETE, &nid);
}