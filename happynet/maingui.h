#ifndef _H_MAINGUI
#define _H_MAINGUI

bool string_empty(WCHAR* str);
bool validate_options(HWND hwndDlg);
void update_addresses(HWND hwndDlg);
void update_service_status(HWND hwndDlg);
void CALLBACK refresh_screen(HWND hwndDlg, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
void read_options(HWND hwndDlg);
void save_options(HWND hwndDlg);
void handle_command_event(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK dialog_proc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ad_settings_dialog_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void setup_system_menu(HWND hwndDlg);
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

#endif
