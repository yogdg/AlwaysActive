#ifndef ALWAYSACTIVE_SHARED_H__
#define ALWAYSACTIVE_SHARED_H__

#include <Windows.h>
#include <strsafe.h>

#include "../resource.h"

typedef struct tagAPPLICATION_RESOURCES
{
    HANDLE hHeap;
    DWORD dwRefreshTimeoutMs;
    HICON hRunningIcon;
    HICON hPausedIcon;
    HMENU hMenu;
    HMENU hContainerMenu;
}
APPLICATION_RESOURCES;

LRESULT CALLBACK DummyWindowProc(HWND hThis, UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID ShowFatalError(LPCSTR lpMsg);
HWND SetupDummyWindow(HINSTANCE hInstance);
INT ProcessDummyWindowMessages(HWND hWnd);

#endif /*ALWAYSACTIVE_SHARED_H__*/
