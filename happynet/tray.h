#ifndef _TRAY_H
#define _TRAY_H

#include "resource.h"
#include <shellapi.h>

#define	WM_USER_SHELLICON WM_USER + 100

VOID HideToTray(HWND hWnd);
VOID DestroyTray(HWND hWnd);

#endif
