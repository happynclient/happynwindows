#ifndef __WIN_ERROR_CLASS_H___
#define __WIN_ERROR_CLASS_H___
#include <windows.h>
#include <string>

#ifndef tstring
#ifdef _UNICODE
#define tstring			std::wstring
#else
#define tstring			std::string		
#endif
#endif

//////////////////////////////////////////////////////////
//	Desc:
//		Class wrapper for windows errors.  This class
//		allows easy translation of error codes into
//		readable strings however, in order for it to
//		work proper it must be given a correct
//		module handle if the error code is not
//		direct from system.
//////////////////////////////////////////////////////////
class CWinErr {
public:
    CWinErr() {
        m_dwErr = 0;
        return;
    }

    CWinErr& operator = (DWORD dwCode) {
        m_dwErr = dwCode;
        return *this;
    }

    void SetCode(DWORD dwCode) {
        m_dwErr = dwCode;
        return;
    }

    DWORD GetCode() {
        return m_dwErr;
    }

    operator DWORD() {
        return m_dwErr;
    }

    tstring GetFormattedMsg(LPCTSTR lpszModule = NULL)
    {
        DWORD	dwFmtRt = 0;
        DWORD	dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM;
        LPVOID	lpMsgBuf = NULL;
        HMODULE hLookupMod = NULL;
        tstring	sMsg = _T("");

        if (lpszModule != NULL) {
            hLookupMod = ::LoadLibraryEx(lpszModule, NULL, LOAD_LIBRARY_AS_DATAFILE);
            if (hLookupMod) {
                dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
            }
        }

        dwFmtRt = ::FormatMessage(
            dwFlags,
            (LPCVOID)hLookupMod,
            m_dwErr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0,
            NULL);


        if (dwFmtRt != 0)	sMsg = (TCHAR*)lpMsgBuf;
        if (lpMsgBuf)		::LocalFree(lpMsgBuf);
        if (hLookupMod)	::FreeLibrary(hLookupMod);

        return sMsg;
    }

protected:
    DWORD m_dwErr;
};

#endif // __WIN_ERROR_CLASS_H___