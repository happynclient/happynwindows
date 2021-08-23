#ifndef _TRAY_H
#define _TRAY_H

#include "resource.h"
#include <shellapi.h>

#define	WM_USER_SHELLICON WM_USER + 100

void hide_to_tray(HWND hWnd);
void destroy_tray(HWND hWnd);

#endif
