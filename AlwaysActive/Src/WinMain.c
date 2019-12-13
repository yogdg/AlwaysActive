#include <Windows.h>
#include <shlwapi.h>

#include "Shared.h"

///
///
const LPCSTR g_szDummyWindowClass = "DUMMY_WINDOW_ALWAYSACTIVE_APP";
static const DWORD g_dwDefaultRefreshTimeoutMs = 5 * 1000;
///
///
APPLICATION_RESOURCES g_AppResources;

///
///
static BOOL CheckPreviousInstance();
static BOOL InitResources(HINSTANCE hInstance);
static VOID TerminateResources();

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hDummy0_, LPSTR lpCmdLn, INT nShowCmd)
{
    INT ret = -1;
    HWND hWnd;

    UNREFERENCED_PARAMETER(hDummy0_);
    UNREFERENCED_PARAMETER(lpCmdLn);
    UNREFERENCED_PARAMETER(nShowCmd);

    if (!InitResources(hInstance))
        goto ExitLabel;

    if ((hWnd = SetupDummyWindow(hInstance)) == NULL)
        goto TerminationLabel;

    ret = ProcessDummyWindowMessages(hWnd);

    UnregisterClassA(g_szDummyWindowClass, hInstance);

TerminationLabel:
    TerminateResources();
ExitLabel:
    return ret;
}

static BOOL InitResources(HINSTANCE hInstance)
{
    if ((g_AppResources.hHeap = GetProcessHeap()) == NULL)
    {
        MessageBoxA(NULL, "Couldn't retrieve process Heap.", NULL, MB_ICONERROR);
        return FALSE;
    }

    {
        INT timeout;
        INT argc;
        const LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

        if (!argv)
        {
            ShowFatalError("Couldn't retrieve command line arguments.");
            return FALSE;
        }

        if (argc != 2)
            g_AppResources.dwRefreshTimeoutMs = g_dwDefaultRefreshTimeoutMs;
        else
            g_AppResources.dwRefreshTimeoutMs = (DWORD)
            (((timeout = StrToIntW(argv[1])) <= 0) ? g_dwDefaultRefreshTimeoutMs : timeout);

        LocalFree((HLOCAL)argv);
    }

    if ((g_AppResources.hRunningIcon = LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_ICON_RUNNING))) == NULL)
    {
        ShowFatalError("Couldn't retrieve application running icon.");
        return FALSE;
    }

    if ((g_AppResources.hPausedIcon = LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_ICON_PAUSED))) == NULL)
    {
        ShowFatalError("Couldn't retrieve application paused icon.");
        return FALSE;
    }

    if ((g_AppResources.hMenu = LoadMenuA(hInstance, MAKEINTRESOURCEA(IDR_MENU1))) == NULL)
    {
        ShowFatalError("Couldn't retrieve application menu.");
        goto FailureLabel;
    }

    if((g_AppResources.hContainerMenu = CreatePopupMenu())== NULL)
    {
        ShowFatalError("Couldn't create application menu.");
        goto FailureLabel;
    }

    if (!InsertMenuA(g_AppResources.hContainerMenu, 0, MF_POPUP, (UINT_PTR)g_AppResources.hMenu, NULL))
    {
        ShowFatalError("Couldn't setup application menu.");
        goto FailureLabel;
    }

    return TRUE;

FailureLabel:
    TerminateResources();
    return FALSE;
}

static VOID TerminateResources()
{
    if(g_AppResources.hRunningIcon)
        DestroyIcon(g_AppResources.hRunningIcon);

    if (g_AppResources.hPausedIcon)
        DestroyIcon(g_AppResources.hPausedIcon);

    if (g_AppResources.hMenu)
        DestroyMenu(g_AppResources.hMenu);

    if (g_AppResources.hContainerMenu)
        DestroyMenu(g_AppResources.hContainerMenu);

}

VOID ShowFatalError(LPCSTR lpMsg)
{
    const DWORD dwSourceError = GetLastError();
    LPSTR lpBuffer = NULL;

    if (!FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, dwSourceError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpBuffer, 0, NULL))
    {
        CHAR buffer[256];
        StringCchPrintfA(buffer, sizeof(buffer), "Unexpected error (%lu) retrieving message for error %lu.", GetLastError(), dwSourceError);

        MessageBoxA(NULL, buffer, lpMsg, MB_ICONERROR);

        return;
    }

    MessageBoxA(NULL, lpBuffer, lpMsg, MB_ICONERROR);
    LocalFree((HLOCAL)lpBuffer);
}

