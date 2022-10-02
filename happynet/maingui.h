#ifndef _H_MAINGUI
#define _H_MAINGUI

BOOL string_empty(WCHAR* str);
BOOL validate_options(HWND hwndDlg);
VOID update_addresses(HWND hwndDlg);
VOID update_service_status(HWND hwndDlg);
VOID CALLBACK refresh_screen(HWND hwndDlg, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
VOID read_options(HWND hwndDlg);
VOID save_options(HWND hwndDlg);
VOID handle_command_event(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK dialog_proc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ad_settings_dialog_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

VOID setup_system_menu(HWND hwndDlg);
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

#endif
