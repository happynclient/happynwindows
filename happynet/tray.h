#ifndef _TRAY_H
#define _TRAY_H

#include "resource.h"
#include <shellapi.h>

#define	WM_USER_SHELLICON WM_USER + 100

VOID hide_to_tray(HWND hWnd);
VOID destroy_tray(HWND hWnd);

#endif
