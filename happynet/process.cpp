#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#include <Winsock.h>
#include "registry.h"
#include "process.h"
#include "service.h"
#include "utils.h"

#pragma comment(lib, "ws2_32.lib")


static HANDLE m_hChildStdoutRead = NULL;
static HANDLE m_hChildStdoutWrite = NULL;
static HANDLE m_hInputFile = NULL;
static HANDLE m_hProcess = NULL;
static HANDLE m_hThread = NULL;
static DWORD m_dwProcessId = 0;


static INT SendStopSig(UINT nEdgeManagerPort)
{
	WORD wVersionRequested;
	WSADATA wsaData;
	INT err; wVersionRequested = MAKEWORD( 1, 1 );
	err = WSAStartup( wVersionRequested, &wsaData );
	if (err != 0) { 
		return -1;
	}	
	if (LOBYTE( wsaData.wVersion ) != 1 || HIBYTE( wsaData.wVersion ) != 1) {
		WSACleanup( ); 
		return -1; 
	}
	SOCKET sockClient = socket(AF_INET , SOCK_DGRAM , 0) ;
	SOCKADDR_IN sockServerAddr;
	sockServerAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	sockServerAddr.sin_family = AF_INET;
	sockServerAddr.sin_port = htons(nEdgeManagerPort);
	CHAR send_buf[8] = "stop";
	INT nLen = sizeof(SOCKADDR);
	sendto(sockClient, send_buf, strlen(send_buf), 0, (SOCKADDR*)&sockServerAddr, nLen);
	closesocket(sockClient) ;
	WSACleanup();
	return 0;
}


// Create a child process that uses the previously created pipes for STDIN and STDOUT.
HANDLE CreateProcessService(WCHAR* pszCmdLine)	
{ 
	// Set the bInheritHandle flag so pipe handles are inherited. 
	SECURITY_ATTRIBUTES saAttr;
	//ZeroMemory(&sa_attr, sizeof(STARTUPINFO));
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL; 

	// Create a pipe for the child process's STDOUT. 
	if (!CreatePipe(&m_hChildStdoutRead, &m_hChildStdoutWrite, &saAttr, 0) ) 
		GetProcessServiceError(TEXT("StdoutRd CreatePipe")); 

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if (!SetHandleInformation(m_hChildStdoutRead, HANDLE_FLAG_INHERIT, 0) )
		GetProcessServiceError(TEXT("Stdout SetHandleInformation")); 

	BOOL bSuccess = FALSE; 
	PROCESS_INFORMATION piProcessInfo; 
	// Set up members of the PROCESS_INFORMATION structure. 
	ZeroMemory( &piProcessInfo, sizeof(PROCESS_INFORMATION) );		


	// Set up members of the STARTUPINFO structure. 
	STARTUPINFO siStartInfo;
	ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
	siStartInfo.cb = sizeof(STARTUPINFO); 
	siStartInfo.hStdError = m_hChildStdoutWrite;
	siStartInfo.hStdOutput = m_hChildStdoutWrite;
	siStartInfo.hStdInput = NULL;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES; 

	// Create the child process. 
	bSuccess = CreateProcess(NULL, 
		pszCmdLine,		// command line 
		&saAttr,			// process security attributes 
		NULL,				// primary thread security attributes 
		TRUE,				// handles are inherited 
		//CREATE_NO_WINDOW|CREATE_DEFAULT_ERROR_MODE|CREATE_UNICODE_ENVIRONMENT|CREATE_NEW_CONSOLE,
		CREATE_NO_WINDOW|CREATE_DEFAULT_ERROR_MODE|CREATE_UNICODE_ENVIRONMENT|CREATE_NEW_PROCESS_GROUP,
		NULL,				// use parent's environment 
		NULL,				// use parent's current directory 
		&siStartInfo,		// STARTUPINFO pointer 
		&piProcessInfo);	// receives PROCESS_INFORMATION 

	// If an error occurs, exit the application. 
	if (!bSuccess) {
		GetProcessServiceError(TEXT("create_service_process"));
	} else {
		// Close handles to the child process and its primary thread.
		// Some applications might keep these handles to monitor the status
		// of the child process, for example. 
	}

	m_hProcess = piProcessInfo.hProcess;
	m_dwProcessId = piProcessInfo.dwProcessId;
	m_hThread = piProcessInfo.hThread;
	return piProcessInfo.hProcess;
}

VOID GraceStopProcessService(VOID)
{
	DWORD dwEdgeManagerPort = 0;

	HKEY hkey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Happynet\\Parameters"),
                        NULL, KEY_READ, &hkey) != ERROR_SUCCESS) {
		dwEdgeManagerPort = 0;
	} else {
		GetRegDword(hkey, L"local_port", &dwEdgeManagerPort);
	}

	if(dwEdgeManagerPort == 0) {
		dwEdgeManagerPort = 5644;
	}
	if (m_hProcess != NULL && m_dwProcessId) {
		u_short edge_manager_port = (u_short)(dwEdgeManagerPort);
		//CreateThread(NULL, 0, send_sig_stop, &edge_manager_port, 0, NULL);
		if (SendStopSig(edge_manager_port) != 0) {
			LogEvent(L"%s:%d (%s) - send sig to stop edge socket error.\n", __FILEW__, __LINE__, __FUNCTIONW__);
		}
	} else {
		LogEvent(L"%s:%d (%s) - Failed to stop or had been stopped.\n", __FILEW__, __LINE__, __FUNCTIONW__);
	}
	return;
}

VOID TerminalProcessService(VOID)
{
	if (m_hProcess != NULL && m_dwProcessId) {
		//HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, m_dwprocess_id);
		if(TerminateProcess(m_hProcess, PROCESS_EXIT_CODE)) {
			// 500 ms timeout; use INFINITE for no timeout
			const DWORD result = WaitForSingleObject(m_hProcess, INFINITE);
			if (result == WAIT_OBJECT_0) {
				// Success
				CloseHandle(m_hProcess);
				CloseHandle(m_hThread);
				m_hThread = NULL;
				m_hProcess = NULL;
				m_dwProcessId = 0;
				m_hChildStdoutRead = NULL;
				m_hChildStdoutWrite = NULL;
			}
			else {
				// Timed out or an error occurred
				LogEvent(L"%s:%d (%s) - Failed to WatiForSingleObject.\n", __FILEW__, __LINE__, __FUNCTIONW__);
			}
		} else {
			LogEvent(L"%s:%d (%s) - Process had been stopped.\n", __FILEW__, __LINE__, __FUNCTIONW__);
		}
	}
}


DWORD GetProcessServiceStatus(VOID)
{
	//if( STILL_ACTIVE == dwMark) //running
	//if( PROCESS_EXIT_CODE == dwMark) //stopped

	DWORD dwMark = PROCESS_EXIT_CODE;
	if (m_hProcess != NULL) {
		//HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, m_dwprocess_id);
		GetExitCodeProcess(m_hProcess, &dwMark);
	} 

	if (dwMark > PROCESS_EXIT_CODE && dwMark != STILL_ACTIVE){
		dwMark = PROCESS_EXIT_CODE;
	}
	return dwMark;
}


// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT. 
// Stop when there is no more data. 
void GetProcessServiceOutput(WCHAR *pszReadBuf) 
{ 
	if (!m_hChildStdoutRead) {
		return;
	}
	DWORD dwRead; 
	CHAR chbuf[PROCESS_STDOUT_BUFSIZE] = { 0 }; 
	BOOL bsuccess = FALSE;
	HANDLE hparent_stdout = GetStdHandle(STD_OUTPUT_HANDLE);

	bsuccess = ReadFile( m_hChildStdoutRead, chbuf, PROCESS_STDOUT_BUFSIZE-1, &dwRead, NULL);
	if( ! bsuccess || dwRead == 0 ) return; 
	//Convert char* string to a wchar_t* string.
	UINT convertedChars = 0;
	UINT newsize = strlen(chbuf) + 1;
    if (newsize > PROCESS_STDOUT_BUFSIZE) {
        newsize = PROCESS_STDOUT_BUFSIZE;
    }
	mbstowcs_s(&convertedChars, pszReadBuf, newsize, chbuf, _TRUNCATE);
	//Display the result and indicate the type of string that it is.
	LogEvent(TEXT("%s\n"), pszReadBuf); 
} 


VOID GetProcessServiceError(PTSTR lpszFunction) 

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
