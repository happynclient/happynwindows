#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#include <Winsock.h>
#include "registry.h"
#include "process.h"
#include "service.h"

#pragma comment(lib, "ws2_32.lib")


static HANDLE m_hchild_stdout_read = NULL;
static HANDLE m_hchild_stdout_write = NULL;
static HANDLE m_hinput_file = NULL;
static HANDLE m_hprocess = NULL;
static HANDLE m_hthread = NULL;
static DWORD m_dwprocess_id = 0;


static int send_sig_stop(u_short edge_manager_port)
{
	WORD w_version_requested;
	WSADATA wsa_data;
	int err; w_version_requested = MAKEWORD( 1, 1 );
	err = WSAStartup( w_version_requested, &wsa_data );
	if ( err != 0 ) 
	{ 
		return -1;
	}	
	if ( LOBYTE( wsa_data.wVersion ) != 1 || HIBYTE( wsa_data.wVersion ) != 1 )
	{
		WSACleanup( ); 
		return -1; 
	}
	SOCKET sock_client = socket(AF_INET , SOCK_DGRAM , 0) ;
	SOCKADDR_IN addr_srv ;
	addr_srv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	addr_srv.sin_family = AF_INET;
	addr_srv.sin_port = htons(edge_manager_port);
	char send_buf[8] = "stop";
	int len = sizeof(SOCKADDR);
	sendto(sock_client, send_buf, strlen(send_buf), 0, (SOCKADDR*)&addr_srv, len);
	closesocket(sock_client) ;
	WSACleanup();
	return 0;
}


HANDLE create_service_process(WCHAR* command_line)
	// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{ 

	// Set the bInheritHandle flag so pipe handles are inherited. 
	SECURITY_ATTRIBUTES sa_attr;
	//ZeroMemory(&sa_attr, sizeof(STARTUPINFO));
	sa_attr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	sa_attr.bInheritHandle = TRUE; 
	sa_attr.lpSecurityDescriptor = NULL; 

	// Create a pipe for the child process's STDOUT. 
	if (!CreatePipe(&m_hchild_stdout_read, &m_hchild_stdout_write, &sa_attr, 0) ) 
		get_service_process_error(TEXT("StdoutRd CreatePipe")); 

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if (!SetHandleInformation(m_hchild_stdout_read, HANDLE_FLAG_INHERIT, 0) )
		get_service_process_error(TEXT("Stdout SetHandleInformation")); 

	BOOL bsuccess = FALSE; 
	PROCESS_INFORMATION pi_process_info; 
	// Set up members of the PROCESS_INFORMATION structure. 
	ZeroMemory( &pi_process_info, sizeof(PROCESS_INFORMATION) );		


	// Set up members of the STARTUPINFO structure. 
	STARTUPINFO si_start_info;
	ZeroMemory( &si_start_info, sizeof(STARTUPINFO) );
	si_start_info.cb = sizeof(STARTUPINFO); 
	si_start_info.hStdError = m_hchild_stdout_write;
	si_start_info.hStdOutput = m_hchild_stdout_write;
	si_start_info.hStdInput = NULL;
	si_start_info.dwFlags |= STARTF_USESTDHANDLES; 

	// Create the child process. 
	bsuccess = CreateProcess(NULL, 
		command_line,		// command line 
		&sa_attr,			// process security attributes 
		NULL,				// primary thread security attributes 
		TRUE,				// handles are inherited 
		//CREATE_NO_WINDOW|CREATE_DEFAULT_ERROR_MODE|CREATE_UNICODE_ENVIRONMENT|CREATE_NEW_CONSOLE,
		CREATE_NO_WINDOW|CREATE_DEFAULT_ERROR_MODE|CREATE_UNICODE_ENVIRONMENT|CREATE_NEW_PROCESS_GROUP,
		NULL,				// use parent's environment 
		NULL,				// use parent's current directory 
		&si_start_info,		// STARTUPINFO pointer 
		&pi_process_info);	// receives PROCESS_INFORMATION 

	// If an error occurs, exit the application. 
	if ( ! bsuccess ) {
		get_service_process_error(TEXT("create_service_process"));
	} else {
		// Close handles to the child process and its primary thread.
		// Some applications might keep these handles to monitor the status
		// of the child process, for example. 
	}

	m_hprocess = pi_process_info.hProcess;
	m_dwprocess_id = pi_process_info.dwProcessId;
	m_hthread = pi_process_info.hThread;
	return pi_process_info.hProcess;
}

void grace_stop_service_process(void)
{
	DWORD dword_edge_manager_port = 0;

	HKEY hkey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Happyn2n\\Parameters", NULL, KEY_READ, &hkey) != ERROR_SUCCESS)
	{
		dword_edge_manager_port = 0;
	} else {
		reg_get_dword(hkey, L"local_port", &dword_edge_manager_port);
	}

	if(dword_edge_manager_port == 0) {
		dword_edge_manager_port = 5644;
	}
	if (m_hprocess != NULL && m_dwprocess_id)
	{
		u_short edge_manager_port = (u_short)(dword_edge_manager_port);
		//CreateThread(NULL, 0, send_sig_stop, &edge_manager_port, 0, NULL);
		if (send_sig_stop(edge_manager_port) != 0) {
			log_event(L"%s:%d (%s) - send sig to stop edge socket error.\n", __FILEW__, __LINE__, __FUNCTIONW__);
		}
	} else {
		log_event(L"%s:%d (%s) - Failed to stop or had been stopped.\n", __FILEW__, __LINE__, __FUNCTIONW__);
	}
	return;
}

void terminal_service_process(void)
{
	if (m_hprocess != NULL && m_dwprocess_id)
	{
		//HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, m_dwprocess_id);
		if(TerminateProcess(m_hprocess, PROCESS_EXIT_CODE)) {
			// 500 ms timeout; use INFINITE for no timeout
			const DWORD result = WaitForSingleObject(m_hprocess, 500);
			if (result == WAIT_OBJECT_0) {
				// Success
				CloseHandle(m_hprocess);
				CloseHandle(m_hthread);
				m_hthread = NULL;
				m_hprocess = NULL;
				m_dwprocess_id = 0;
				m_hchild_stdout_read = NULL;
				m_hchild_stdout_write = NULL;
			}
			else {
				// Timed out or an error occurred
				log_event(L"%s:%d (%s) - Failed to WatiForSingleObject.\n", __FILEW__, __LINE__, __FUNCTIONW__);
			}
		} else {
			log_event(L"%s:%d (%s) - Failed to stop or had been stopped.\n", __FILEW__, __LINE__, __FUNCTIONW__);
		}
	}
}


DWORD get_service_process_status(void)
{
	//if( STILL_ACTIVE == dwmark) //running
	//if( PROCESS_EXIT_CODE == dwmark) //stopped

	DWORD dwmark = PROCESS_EXIT_CODE;
	if (m_hprocess != NULL) {
		//HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, m_dwprocess_id);
		GetExitCodeProcess(m_hprocess, &dwmark);
	} 

	if (dwmark > PROCESS_EXIT_CODE && dwmark != STILL_ACTIVE){
		dwmark = PROCESS_EXIT_CODE;
	}
	return dwmark;
}


// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT. 
// Stop when there is no more data. 
void get_service_process_output(WCHAR *read_buf) 
{ 
	if (!m_hchild_stdout_read) {
		return;
	}
	DWORD dwread; 
	CHAR chbuf[BUFSIZE] = {'\0'}; 
	BOOL bsuccess = FALSE;
	HANDLE hparent_stdout = GetStdHandle(STD_OUTPUT_HANDLE);

	bsuccess = ReadFile( m_hchild_stdout_read, chbuf, BUFSIZE, &dwread, NULL);
	if( ! bsuccess || dwread == 0 ) return; 
	//Convert char* string to a wchar_t* string.
	size_t convertedChars = 0;
	size_t newsize = strlen(chbuf) + 1;
	mbstowcs_s(&convertedChars, read_buf, newsize, chbuf, _TRUNCATE);
	//Display the result and indicate the type of string that it is.
	log_event(L"%s\n", read_buf); 
} 


void get_service_process_error(PTSTR lpszFunction) 

	// Format a readable error message, display a message box, 
	// and exit from the application.
{ 
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError(); 

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
		(lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
	StringCchPrintf((LPTSTR)lpDisplayBuf, 
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"), 
		lpszFunction, dw, lpMsgBuf); 
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}
