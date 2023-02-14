/*
Module : sinstance.h
Purpose: Defines the interface for a C++ wrapper class to do instance checking
Created: PJN / 29-07-1998

Copyright (c) 1996 - 2022 by PJ Naughter (Web: www.naughter.com, Email: pjna@naughter.com)

All rights reserved.

Copyright / Usage Details:

You are allowed to include the source code in any product (commercial, shareware, freeware or otherwise) 
when your product is released in binary form. You are allowed to modify the source code in any way you want 
except you cannot modify the copyright details at the top of each module. If you want to distribute source 
code with your application, then you are only allowed to distribute versions released by the author. This is 
to maintain a single distribution point for the source code. 

*/


/////////////////////////// Macros / Defines //////////////////////////////////

#pragma once

#ifndef __SINSTANCE_H__
#define __SINSTANCE_H__

#ifndef CSINGLEINSTANCE_EXT_CLASS
#define CSINGLEINSTANCE_EXT_CLASS
#endif //#ifndef CSINGLEINSTANCE_EXT_CLASS

#ifndef CSINGLEINSTANCE_EXT_API
#define CSINGLEINSTANCE_EXT_API
#endif //#ifndef CSINGLEINSTANCE_EXT_API


/////////////////////////// Includes //////////////////////////////////////////

#ifndef __ATLSYNC_H__
#pragma message("To avoid this message, please put atlsync.h in your pre compiled header (normally stdafx.h)")
#include <atlsync.h>
#endif //#ifndef __ATLSYNC_H__

#ifndef __ATLSTR_H__
#pragma message("To avoid this message, please put atlstr.h in your pre compiled header (normally stdafx.h)")
#include <atlstr.h>
#endif //#ifndef __ATLSTR_H__


/////////////////////////// Classes //////////////////////////////////////////

class CSINGLEINSTANCE_EXT_CLASS CInstanceChecker
{
public:
//Constructors / Destructors
  CInstanceChecker(_In_z_ LPCTSTR pszUniqueName);
  CInstanceChecker(_In_ const CInstanceChecker&) = delete;
  CInstanceChecker(_In_ CInstanceChecker&&) = delete;
  virtual ~CInstanceChecker() = default;

//General functions
  CInstanceChecker& operator=(_In_ const CInstanceChecker&) = delete;
  CInstanceChecker& operator=(_In_ CInstanceChecker&&) = delete;
  _Return_type_success_(return != false) virtual bool TrackFirstInstanceRunning(_In_ HWND hWindowToTrack);
  _Return_type_success_(return != false) virtual bool PreviousInstanceRunning();
  _Return_type_success_(return != false) virtual bool ActivatePreviousInstance(_In_opt_ LPCTSTR lpCmdLine = nullptr, _In_ ULONG_PTR dwCopyDataItemData = 0, _In_ DWORD dwTimeout = 30000, HWND hSender = nullptr);
  _Return_type_success_(return != false) virtual bool QuitPreviousInstance(_In_ int nExitCode = 0);
  _Return_type_success_(return != false) virtual bool FindPreviousHWND(_Out_ HWND& hPrevWnd);

protected:
//Virtual methods
  virtual ATL::CAtlString GetExecuteMutexName();
  _Return_type_success_(return != false) virtual bool CreateExecuteMutexIfNecessary();

//Member variables
  ATL::CAtlString m_sName;
  ATL::CHandle m_MMF;
  ATL::CMutex m_ExecuteMutex;
};

#endif //#ifndef __SINSTANCE_H__
