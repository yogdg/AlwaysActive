#include <Windows.h>

#include "Shared.h"

///
///
#define WM_TRAY_MESSAGE (WM_USER + 711)
///
///
static const DWORD g_dwDefaultRefreshTimeoutMs = 5 * 1000;
static const UINT_PTR g_uTimerId = 6887;
static const UINT g_uNotificationId = 71187;
static const LPCSTR g_lpRunningTooltipText = "Easy, we have your back ;)";
static const LPCSTR g_lpPausedTooltipText = "You're on your own :(";
///
///
extern const LPCSTR g_szDummyWindowClass;
extern APPLICATION_RESOURCES g_AppResources;
///
///
typedef struct
{
    HWND hWnd;
    UINT_PTR uTimerId;
    UINT uNotificationId;
    BOOL bPaused;
}
DUMMY_WINDOW_DATA, * LPDUMMY_WINDOW_DATA;
///
///
static LRESULT DoWmCreate(HWND hThis);
static LRESULT DoWmDestroy(LPDUMMY_WINDOW_DATA lpThis);
static LRESULT DoWmTimer(LPDUMMY_WINDOW_DATA lpThis, UINT_PTR uTimerId);
static LRESULT DoTrayMessage(LPDUMMY_WINDOW_DATA lpThis, UINT uMsg);
static LRESULT DoWmCommand(LPDUMMY_WINDOW_DATA lpThis, UINT uMenuId);

HWND SetupDummyWindow(HINSTANCE hInstance)
{
    HWND ret;
    WNDCLASSEXA wnd;

    ZeroMemory(&wnd, sizeof(wnd));
    wnd.cbSize = sizeof(wnd);
    wnd.hInstance = hInstance;
    wnd.lpfnWndProc = DummyWindowProc;
    wnd.lpszClassName = g_szDummyWindowClass;

    if (!RegisterClassExA(&wnd))
    {
        ShowFatalError("Couldn't register Window.");
        return NULL;
    }

    if ((ret = CreateWindowExA(
        0
        , g_szDummyWindowClass
        , NULL
        , 0
        , 0, 0, 0, 0
        , NULL
        , NULL
        , hInstance
        , NULL)) == NULL)
    {
        ShowFatalError("Couldn't create Window.");
        UnregisterClassA(g_szDummyWindowClass, hInstance);
        return NULL;
    }

    ShowWindow(ret, SW_HIDE);
    return ret;
}

INT ProcessDummyWindowMessages(HWND hWnd)
{
    MSG msg;
    INT bgm;

    while ((bgm = (INT)GetMessageA(&msg, hWnd, 0, 0)) > 0)
        DispatchMessageA(&msg);

    if (bgm == -1)
    {
        ShowFatalError("Error while popping messages.");
        return -1;
    }

    return (INT)msg.wParam;
}

LRESULT CALLBACK DummyWindowProc(HWND hThis, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPDUMMY_WINDOW_DATA pThis = (LPDUMMY_WINDOW_DATA)GetWindowLongPtrA(hThis, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_CREATE:
        return DoWmCreate(hThis);

    case WM_DESTROY:
        return DoWmDestroy(pThis);

    case WM_TIMER:
        return DoWmTimer(pThis, (UINT_PTR)wParam);

    case WM_TRAY_MESSAGE:
        return DoTrayMessage(pThis, (UINT)(LOWORD(lParam)));

    case WM_COMMAND:
        return DoWmCommand(pThis, (UINT)LOWORD(wParam));

    default:
        return DefWindowProcA(hThis, uMsg, wParam, lParam);
    }
}

static LRESULT DoWmCreate(HWND hThis)
{
    LPDUMMY_WINDOW_DATA lpThis;

    if ((lpThis = (LPDUMMY_WINDOW_DATA)HeapAlloc(
        g_AppResources.hHeap
        , HEAP_ZERO_MEMORY
        , sizeof(DUMMY_WINDOW_DATA))) == NULL)
    {
        ShowFatalError("Couldn't allocate memory for the Window.");
        return -1;
    }

    SetLastError(0);
    if (!SetWindowLongPtrA(hThis, GWLP_USERDATA, (LONG_PTR)lpThis) && GetLastError())
    {
        ShowFatalError("Couldn't link memory to Window.");
        HeapFree(g_AppResources.hHeap, 0, (LPVOID)lpThis);
        return -1;
    }

    lpThis->hWnd = hThis;

    if ((lpThis->uTimerId = SetTimer(lpThis->hWnd, g_uTimerId, g_AppResources.dwRefreshTimeoutMs, NULL)) == 0)
    {
        ShowFatalError("Coudln't create the timer.");
        return -1;
    }

    {
        NOTIFYICONDATAA ni;
        ZeroMemory(&ni, sizeof(ni));

        ni.cbSize = sizeof(ni);
        ni.hWnd = lpThis->hWnd;
        lpThis->uNotificationId = ni.uID = g_uNotificationId;
        ni.uCallbackMessage = WM_TRAY_MESSAGE;
        ni.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        ni.hIcon = g_AppResources.hRunningIcon;
        StringCchCopyA(ni.szTip, sizeof(ni.szTip), g_lpRunningTooltipText);

        if (!Shell_NotifyIconA(NIM_ADD, &ni))
        {
            lpThis->uNotificationId = 0;
            ShowFatalError("Couldn't add notification icon.");
            return -1;
        }
    }

    return 0;
}

static LRESULT DoWmDestroy(LPDUMMY_WINDOW_DATA lpThis)
{
    if (!lpThis)
        return 0;

    if (lpThis->uTimerId)
        KillTimer(lpThis->hWnd, lpThis->uTimerId);

    if (lpThis->uNotificationId)
    {
        NOTIFYICONDATAA ni;
        ZeroMemory(&ni, sizeof(ni));

        ni.cbSize = sizeof(ni);
        ni.hWnd = lpThis->hWnd;
        ni.uID = lpThis->uNotificationId;

        Shell_NotifyIconA(NIM_DELETE, &ni);
    }

    // In the event of SetWindowLongPtrA failing for whatever reason, the safest thing
    // to do is not to free the memory, and have the OS to free the memory by us
    SetLastError(0);
    if (!SetWindowLongPtrA(lpThis->hWnd, GWLP_USERDATA, (LONG_PTR)NULL) && GetLastError())
    {
        ShowFatalError("Couldn't unlink memory from Window");
        return -1;
    }
    HeapFree(g_AppResources.hHeap, 0, (LPVOID)lpThis);

    PostQuitMessage(0);
    return 0;
}

static LRESULT DoWmTimer(LPDUMMY_WINDOW_DATA lpThis, UINT_PTR uTimerId)
{
    if (uTimerId != lpThis->uTimerId)
        return !0;

    if (lpThis->bPaused)
        return 0;

    if (!SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED))
    {
        ShowFatalError("Couldn't update session status.");
        SendMessageA(lpThis->hWnd, WM_CLOSE, 0, 0);
    }

    return 0;
}

static LRESULT DoTrayMessage(LPDUMMY_WINDOW_DATA lpThis, UINT uMsg)
{
    if (uMsg == WM_CONTEXTMENU || uMsg == WM_RBUTTONUP)
    {
        POINT pt;
        GetCursorPos(&pt);
        TrackPopupMenu(g_AppResources.hMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, lpThis->hWnd, NULL);
    }

    return 0;
}

static LRESULT DoWmCommand(LPDUMMY_WINDOW_DATA lpThis, UINT uMenuId)
{
    if (uMenuId == ID_EXIT)
    {
        DestroyWindow(lpThis->hWnd);
        return 0;
    }

    if (uMenuId == ID_TOGGLE)
    {
        LPCSTR lpMenuOption = lpThis->bPaused ? "Pause" : "Resume";
        LPCSTR lpTooltipTxt = lpThis->bPaused ? g_lpRunningTooltipText : g_lpPausedTooltipText;
        HICON hIcon = lpThis->bPaused ? g_AppResources.hRunningIcon : g_AppResources.hPausedIcon;

        ModifyMenuA(g_AppResources.hMenu, ID_TOGGLE, MF_BYCOMMAND | MF_STRING, ID_TOGGLE, lpMenuOption);

        {
            NOTIFYICONDATAA ni;
            ZeroMemory(&ni, sizeof(ni));

            ni.cbSize = sizeof(ni);
            ni.hWnd = lpThis->hWnd;
            ni.uID = g_uNotificationId;
            ni.uCallbackMessage = WM_TRAY_MESSAGE;
            ni.uFlags = NIF_ICON | NIF_TIP;
            ni.hIcon = hIcon;
            StringCchCopyA(ni.szTip, sizeof(ni.szTip), lpTooltipTxt);

            Shell_NotifyIconA(NIM_MODIFY, &ni);
        }

        lpThis->bPaused = !lpThis->bPaused;
    }

    return !0;
}
