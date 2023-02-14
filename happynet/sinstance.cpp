/*
Module : sinstance.cpp
Purpose: Defines the implementation for an C++ wrapper class to do instance checking
Created: PJN / 29-07-1998
History: PJN / 25-03-2000 Neville Franks made the following changes. Contact nevf@getsoft.com, www.getsoft.com
                          1. Changed #pragma error to #pragma message. Former wouldn't compile under VC6
                          2. Replaced above #pragma with #include
                          3. Added TrackFirstInstanceRunning(), MakeMMFFilename()
         PJN / 27-03-2000 1. Fixed a potential handle leak where the file handle m_hPrevInstance was not being
                          closed under certain circumstances.
                          Neville Franks made the following changes. Contact nevf@getsoft.com, www.getsoft.com
                          2. Split PreviousInstanceRunning() up into separate functions so we
                          can call it without needing the MainFrame window.
                          3. Changed ActivatePreviousInstance() to return hWnd.
         PJN / 15-05-2000 1. Serialized access to all of the CSingleInstance class to prevent race conditions
                          which can occur when you app is programatically spawned.
         PJN / 17-05-2000 1. Updated sample to show how the code can be used for dialog based apps.
         PJN / 01-01-2001 1. Added a number of asserts to CInstanceChecker::ActivatePreviousInstance
                          2. Now includes copyright message in the source code and documentation.
         PJN / 15-01-2001 1. Now allows multiple calls to PreviousInstanceRunning without ASSERTing
                          2. Removed some unnecessary VERIFY's from ActivatePreviousInstance
                          3. Made the MMF filename used modifiable via a virtual function GetMMFFilename 
                          4. Made the window to track modifiable via a virtual function GetWindowToTrack
                          5. Made destructor virtual since the introduction of other virtual functions in the class
                          6. Removed a number of unnecessary verifies
                          7. Changed the meaning of the return value from TrackFirstInstanceRunning
         PJN / 17-06-2001 1. Moved most of the code from CInstanceChecker::CInstanceChecker to 
                          CInstanceChecker::ActivateChecker. This allows client code to turn on or off the instance
                          checking code easily. Thanks to Anders Rundegren for this addition.
         PJN / 31-08-2001 1. made the name of the mutex which the class uses to serialize access to itself a paramter
                          to the constructor. That way multiple independent apps do not block each other while
                          they are calling into the CSingleInstance class. Thanks to Eugene Shmelyov for spotting
                          this problem.
         PJN / 23-03-2002 1. Provided a QuitPreviousInstance method. Thanks to Jon Bennett for providing this.
         PJN / 30-10-2002 1. The name of the internal memory mapped file is now based on the Mutex name rather than
                          the application name. An example: a client was writing a webcam application and wanted it to 
                          run with multiple configuration for multiple camera support. So the app can run multiple times 
                          as long as a special configuration is given on the command line. But for that configuration 
                          only one instance is allowed. Using the application name for the memory mapped file was tying 
                          the single instance to the app rather than the unique mutex name. Thanks to Frank Fesevur for 
                          this nice update.
         PJN / 06-02-2003 1. Was missing a call to ReleaseLock in CInstanceChecker::ActivatePreviousInstance. Thanks to 
                          Pierrick Ingels for reporting this problem.
         PJN / 09-05-2004 1. Updated the copyright details.
                          2. Extended CInstanceChecker::ActivatePreviousInstance to now allow the command line of the
                          second app to be passed to the original app. By default the parameter is null, meaning that 
                          you get the original behaviour which just activates the previous instance. To respond to this
                          information you should add the following to your mainfrm module:

                          mainfrm.h

                          afx_msg LRESULT OnCopyData(WPARAM, LPARAM);


                          mainfrm.cpp

                          LRESULT CMyFrameWnd::OnCopyData(WPARAM wParam, LPARAM lParam)
                          {
                            COPYDATASTRUCT* pCDS = reinterpret_cast<COPYDATASTRUCT*>(lParam);
                            TCHAR* pszCmdLine = static_cast<TCHAR*>(pCDS->lpData);
                            if (pszCmdLine)
                            {
                              //DO SOMETHING with pszCmdLine here such as call AfxGetApp()->OpenDocumentFile(pszCmdLine);
                            }
                            return TRUE;
                          }

                          Also hook up your onCopyData to the windows message map using

                          ON_MESSAGE(WM_COPYDATA, OnCopyData)


                          Thanks to Ingo H. de Boer for providing this nice update.

                          3. Following a discussion on the Codeproject.com discussion forum for CSingleInstance on what
                          exactly a single instance app means, Daniel Lohmann has produced a simple function called CreateUniqueName
                          which given a number of settings as flags, will produce a name which is unique. You can then use this name
                          in the constructor for CInstanceChecker. The concept of a single instance app is complicated by the concept 
                          of Window stations and desktops as used by NT Services and Windows Terminal Services. In addition you might 
                          want to allow your program to be run once per user.
         PJN / 30-05-2005 1. Fix for a crash where CWnd::GetLastActivePopup can sometimes return a null pointer. Thanks to 
                          Dominik Reichl for reporting this bug.
         PJN / 07-07-2006 1. Updated copyright details.
                          2. Addition of CSINGLEINSTANCE_EXT_CLASS and CSINGLEINSTANCE_EXT_API which allows the class to be easily used
                          in an extension DLL.
                          3. Removed derivation from CObject as it was not really needed.
                          4. Updated the documentation to use the same style as the web site.
                          5. Code now uses newer C++ style casts instead of C style casts.
                          6. Fixed a number of level 4 warnings in the sample app.
                          7. Updated code to compile cleanly using VC 2005.
         PJN / 17-03-2007 1. Updated copyright details.
                          2. Optimized _INSTANCE_DATA constructor code
                          3. Reworked how the method CInstanceChecker::GetMMFFilename creates the name of the memory mapped filename
                          the code requires for sharing. Now the main instance name appears before the hard coded string. This
                          ensures that the CInstanceChecker class works correctly for terminal sessions i.e. kernel objects prefixed
                          with the value "Local\". Thanks to Mathias Berchtold for reporting this issue.
                          4. Updated the sample app code to clean compile on VC 2005
                          5. QuitPreviousInstance now uses GetLastActivePopup API to ensure it posts the WM_QUIT message to the 
                          correct window of the previous instance.
         PJN / 02-02-2008 1. Updated copyright details
                          2. Removed VC 6 style classwizard comments from the sample apps code
                          3. Updated ActivatePreviousInstance method to support Win64 compliant data
                          4. ActivatePreviousInstance now takes a "dwTimeout" parameter which it now uses internally as the timeout when
                          calling SendMessageTimeout instead of SendMessage. The code now uses SendMessageTimeout instead of SendMessage 
                          to ensure we do not hang if the previous instance itself is hung. Thanks to Paul Shore for suggesting this 
                          update.
                          5. Updated the sample apps to clean compile on VC 2008
         PJN / 04-03-2016 1. Updated copyright details.
                          2. Update the sample app project settings to more modern defaults.
                          3. Update the code to clean compile on VC 2010 - 2015
                          4. Added SAL annotations to all the code.
                          5. CInstanceChecker::PreviousInstanceRunning now uses FILE_MAP_READ flag when attempting to open the file 
                          mapping
                          6. The ActivatePreviousInstance method now includes a new HWND hSender parameter.
                          7. The sample app which uses a MFC CFrameWnd now shows some text in the client area to tell end-users
                          what to do to exercise the code.
                          8. After updating the instructions on how to use the class, the code which referenced an internal 
                          _SINSTANCE_DATA global static instance has been refactored back into the main CInstanceChecker class
                          9. CInstanceChecker::TrackFirstInstanceRunning now takes a HWND hWindowToTrack parameter. This parameter
                          now eliminates the need for the GetWindowToTrack method.
                          10. Reworked the internals of the CInstanceChecker class to avoid use of MFC. Instead now all the class uses
                          ATL replacements for equivalent MFC functionality.
                          11. Reworked the internals of the QuitPreviousInstance method.
                          12. Removed the need for the ActivateChecker and GetMMFFilename methods.
                          13. The structure which is put into the memory mapped file now is 8 bytes in size. This is to allow
                          interop between Win32 and Win64 instances of the same application.
                          14. Refactored the code in ActivatePreviousInstance & QuitPreviousInstance which finds the previous HWND into 
                          a new FindPreviousHWND method.
                          15. Reworked the code which serializes access to all instances of CInstanceChecker and in the process removed
                          the need for the mutex which protects the memory mapped file.
         PJN / 30-10-2017 1. Updated copyright details
                          2. Replaced NULL with nullptr throughout the codebase. This means that the code now 
                          requires VC 2010 at a minimum to compile.
                          3. Replaced BOOL throughout the codebase with bool.
                          4. Fixed problems with the code when the _ATL_NO_AUTOMATIC_NAMESPACE define is used. Thanks to Victor Derks 
                          for reporting this issue.
         PJN / 15-07-2018 1. Updated copyright details
                          2. Fixed a number of C++ core guidelines compiler warnings. These changes mean that
                          the code will now only compile on VC 2017 or later.
         PJN / 02-09-2018 1. Fixed a number of compiler warnings when using VS 2017 15.8.2
         PJN / 02-06-2019 1. Updated copyright details.
                          2. Updated the code to clean compile on VC 2019
         PJN / 21-09-2019 1. Fixed a number of compiler warnings when the code is compiled with VS 2019 Preview
         PJN / 17-03-2020 1. Updated copyright details.
                          2. Fixed more Clang-Tidy static code analysis warnings in the code.
         PJN / 09-03-2022 1. Updated copyright details
                          2. Updated the code to use C++ uniform initialization for all variable declarations.

Copyright (c) 1996 - 2022 by PJ Naughter (Web: www.naughter.com, Email: pjna@naughter.com)

All rights reserved.

Copyright / Usage Details:

You are allowed to include the source code in any product (commercial, shareware, freeware or otherwise) 
when your product is released in binary form. You are allowed to modify the source code in any way you want 
except you cannot modify the copyright details at the top of each module. If you want to distribute source 
code with your application, then you are only allowed to distribute versions released by the author. This is 
to maintain a single distribution point for the source code.

*/


/////////////////////////// Includes //////////////////////////////////////////

#include "sinstance.h"


/////////////////////////// Implementation ////////////////////////////////////

//The struct which we put into the MMF
struct CWindowInstance
{
  unsigned __int64 hMainWnd;
};


CInstanceChecker::CInstanceChecker(_In_z_ LPCTSTR pszUniqueName) : m_sName{pszUniqueName}
{
}

#pragma warning(suppress: 26461)
_Return_type_success_(return != false) bool CInstanceChecker::TrackFirstInstanceRunning(_In_ HWND hWindowToTrack)
{
  //Create the mutex to serialize access to all instances of CInstanceChecker if required
  if (!CreateExecuteMutexIfNecessary())
    return false;

  //Serialize access using the execute mutex
  ATL::CMutexLock ExecuteLock{m_ExecuteMutex, true};

  //First create the MMF if required
  constexpr const int nMMFSize{sizeof(CWindowInstance)};
  if (m_MMF == nullptr)
  {
#pragma warning(suppress: 26477)
    ATLASSERT(m_sName.GetLength()); //Validate our parameter

    HANDLE hMMF{CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, nMMFSize, m_sName)};
    if (hMMF == nullptr)
    {
      ATLTRACE(_T("CInstanceChecker::TrackFirstInstanceRunning, Failed in call to CreateFileMapping, Error:%u\n"), GetLastError());
      return false;
    }
    m_MMF.Attach(hMMF);
  }

  //Open the MMF for writing
  auto pInstanceData{static_cast<CWindowInstance*>(MapViewOfFile(m_MMF, FILE_MAP_WRITE, 0, 0, nMMFSize))};
  if (pInstanceData == nullptr)
  {
    ATLTRACE(_T("CInstanceChecker::TrackFirstInstanceRunning, Failed in call to MapViewOfFile, Error:%u\n"), GetLastError());
    m_MMF.Close();
    return false;
  }

  //Update the data in the MMF with the window handle we are to track
#pragma warning(suppress: 26490)
  pInstanceData->hMainWnd = reinterpret_cast<unsigned __int64>(hWindowToTrack);

  //Close the MMF
  if (!UnmapViewOfFile(pInstanceData))
  {
    ATLTRACE(_T("CInstanceChecker::TrackFirstInstanceRunning, Failed in call to UnmapViewOfFile, Error:%u\n"), GetLastError());
    m_MMF.Close();
    return false;
  }

  return true;
}

_Return_type_success_(return != false) bool CInstanceChecker::PreviousInstanceRunning()
{
  //Validate our parameters
#pragma warning(suppress: 26477)
  ATLASSERT(m_sName.GetLength());

  //Create the mutex to serialize access to all instances of CInstanceChecker if required
  if (!CreateExecuteMutexIfNecessary())
    return false;

  //Serialize access using the execute mutex
  ATL::CMutexLock ExecuteLock{m_ExecuteMutex, true};

  //Try to open the MMF first to see if we have an instance already running
  HANDLE hPrevInstance{OpenFileMapping(FILE_MAP_READ, FALSE, m_sName)};
  const bool bPreviousInstance{hPrevInstance != nullptr};
  if (hPrevInstance)
    CloseHandle(hPrevInstance);

  return bPreviousInstance;
}

ATL::CAtlString CInstanceChecker::GetExecuteMutexName()
{
  ATL::CAtlString sExecuteMutexName{m_sName};
  sExecuteMutexName += _T("_CInstanceChecker_XMUTEX");
  return sExecuteMutexName;
}

_Return_type_success_(return != false) bool CInstanceChecker::CreateExecuteMutexIfNecessary()
{
  if (m_ExecuteMutex == nullptr)
  {
    if (!m_ExecuteMutex.Create(nullptr, FALSE, GetExecuteMutexName()))
    {
      ATLTRACE(_T("CInstanceChecker::CreateExecuteMutexIfNecessary, Failed in call to create execute mutex, Error:%u\n"), GetLastError());
      return false;
    }
  }

  return true;
}

_Return_type_success_(return != false) bool CInstanceChecker::FindPreviousHWND(_Out_ HWND& hPrevWnd)
{
  //Validate our parameters
#pragma warning(suppress: 26477)
  ATLASSERT(m_sName.GetLength());

  //Create the mutex to serialize access to all instances of CInstanceChecker if required
  if (!CreateExecuteMutexIfNecessary())
    return false;

  //Serialize access using the execute mutex
  ATL::CMutexLock ExecuteLock{m_ExecuteMutex, true};

  //Try to open the MMF for reading
  HANDLE hInstance{OpenFileMapping(FILE_MAP_READ, FALSE, m_sName)};
  if (hInstance == nullptr)
  {
    ATLTRACE(_T("CInstanceChecker::FindPreviousHWND, Failed in call to OpenFileMapping, Error:%u\n"), GetLastError());
    return false;
  }
  ATL::CHandle hPrevInstance{hInstance};

  //Open up the MMF to get the data
  constexpr const int nMMFSize{sizeof(CWindowInstance)};
  auto pInstanceData{static_cast<CWindowInstance*>(MapViewOfFile(hPrevInstance, FILE_MAP_READ, 0, 0, nMMFSize))};
  if (pInstanceData == nullptr)
  {
    ATLTRACE(_T("CInstanceChecker::FindPreviousHWND, Failed in call to MapViewOfFile, Error:%u\n"), GetLastError());
    return false;
  }

#pragma warning(suppress: 26490)
  hPrevWnd = reinterpret_cast<HWND>(pInstanceData->hMainWnd);

  //Unmap the MMF we were using
  if (!UnmapViewOfFile(pInstanceData))
  {
    ATLTRACE(_T("CInstanceChecker::FindPreviousHWND, Failed in call to UnmapViewOfFile, Error:%u\n"), GetLastError());
  }

  return true;
}

#pragma warning(suppress: 26461)
_Return_type_success_(return != false) bool CInstanceChecker::ActivatePreviousInstance(_In_opt_ LPCTSTR lpCmdLine, _In_ ULONG_PTR dwCopyDataItemData, _In_ DWORD dwTimeout, HWND hSender)
{
  //Create the mutex to serialize access to all instances of CInstanceChecker if required
  if (!CreateExecuteMutexIfNecessary())
    return false;

  //Serialize access using the execute mutex
  ATL::CMutexLock ExecuteLock{m_ExecuteMutex, true};

  //First find the previous HWND
  HWND hPrevWnd{nullptr};
  if (!FindPreviousHWND(hPrevWnd))
    return false;
  if (hPrevWnd == nullptr)
  {
    ATLTRACE(_T("CInstanceChecker::ActivatePreviousInstance, HWND in memory mapping is null\n"));
    return false;
  }

  //activate, restore the focus and and bring to the foreground the previous instance's HWND
  HWND hWndChild{GetLastActivePopup(hPrevWnd)};
  if (IsIconic(hPrevWnd))
    ShowWindow(hPrevWnd, SW_RESTORE);
  if (hWndChild != nullptr)
    SetForegroundWindow(hWndChild);

  //Send the specified command line to the previous instance using SendMessageTimeout & WM_COPYDATA if required
  LRESULT lResult{0};
  if (lpCmdLine != nullptr)
  {
    COPYDATASTRUCT cds{};
    cds.dwData = dwCopyDataItemData;
#pragma warning(suppress: 26472)
    const auto dwCmdLength{static_cast<DWORD>(_tcslen(lpCmdLine) + 1)};
    cds.cbData = dwCmdLength * sizeof(TCHAR);
    ATL::CAtlString sCmdLine{lpCmdLine};
    LPTSTR pszCmdLine{sCmdLine.GetBuffer()};
    cds.lpData = pszCmdLine;

    //Send the message to the previous instance. Use SendMessageTimeout instead of SendMessage to ensure we 
    //do not hang if the previous instance itself is hung
    DWORD_PTR dwResult{0};
#pragma warning(suppress: 26490)
    lResult = SendMessageTimeout(hPrevWnd, WM_COPYDATA, reinterpret_cast<WPARAM>(hSender), reinterpret_cast<LPARAM>(&cds),
                                 SMTO_ABORTIFHUNG, dwTimeout, &dwResult);
    if (lResult == 0)
    {
      ATLTRACE(_T("CInstanceChecker::ActivatePreviousInstance, SendMessageTimeout call failed, Error:%u\n"), GetLastError());
    }
    sCmdLine.ReleaseBuffer();
  }

  return (lResult != 0);
}

_Return_type_success_(return != false) bool CInstanceChecker::QuitPreviousInstance(_In_ int nExitCode)
{
  //Create the mutex to serialize access to all instances of CInstanceChecker if required
  if (!CreateExecuteMutexIfNecessary())
    return false;

  //Serialize access using the execute mutex
  ATL::CMutexLock ExecuteLock{m_ExecuteMutex, true};

  //First find the previous HWND
  HWND hPrevWnd{nullptr};
  if (!FindPreviousHWND(hPrevWnd))
    return false;

  if (hPrevWnd == nullptr)
  {
    ATLTRACE(_T("CInstanceChecker::QuitPreviousInstance, HWND in memory mapping is null\n"));
    return false;
  }

  //Ask it to exit
  HWND hChildWnd{GetLastActivePopup(hPrevWnd)};
  PostMessage(hChildWnd, WM_QUIT, nExitCode, 0);

  return true;
}
