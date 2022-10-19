#ifndef _H_MAINGUI
#define _H_MAINGUI


#define  AUTOIP_TEXT "Auto Get IP Address"
#define IsItemChecked(x,y) (SendDlgItemMessage(x, y, BM_GETCHECK, 0, 0) == BST_CHECKED)

BOOL ValidateOptions(HWND hwndDlg);
VOID UpdateAddressesInfo(HWND hwndDlg);
VOID UpdateServiceStatus(HWND hwndDlg);
//VOID CALLBACK RefreshScreen(HWND hwndDlg, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
VOID ReadOptions(HWND hwndDlg);
VOID SaveOptions(HWND hwndDlg);
VOID HandleCommandEvent(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MainDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AdSettingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

VOID SetupSystemMenu(HWND hwndDlg);
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

#endif
