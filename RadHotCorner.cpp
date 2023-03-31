#include "Window.h"
#include "Windowxx.h"
#include <tchar.h>
#include <vector>
#include <string>
#include "Format.h"
#include "memory_utils.h"
#include "reg_utils.h"
#include <shellapi.h>
#include "resource.h"
#include "Logging.h"

extern HINSTANCE g_hInstance;

#define APP_NAME TEXT("Rad Hot Corner")

#define TIMER_CORNER (1)

#define KEYDOWN(k) ((k) & 0x80)

#define WM_TRAY         (WM_USER + 0x0200)

inline BOOL AddTrayIcon(HWND hWnd)
{
    NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
    nid.hWnd = hWnd;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.uCallbackMessage = WM_TRAY;
    wcscpy_s(nid.szTip, APP_NAME);
    //LoadIconMetric(g_hDllInstance, MAKEINTRESOURCE(IDI_MAIN), LIM_SMALL, &(nid.hIcon));
    nid.hIcon = (HICON) GetClassLongPtr(hWnd, GCLP_HICON);
    return Shell_NotifyIcon(NIM_ADD, &nid);
}

inline BOOL DeleteTrayIcon(HWND hWnd)
{
    NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
    nid.hWnd = hWnd;
    nid.uFlags = NIF_MESSAGE;
    nid.uCallbackMessage = WM_TRAY;
    return Shell_NotifyIcon(NIM_DELETE, &nid);
}

struct MonitorInfo
{
    HMONITOR hMonitor;
    RECT rRect;
};

BOOL MonitorEnumProc(
    HMONITOR hMonitor,
    HDC hDC,
    LPRECT pRect,
    LPARAM lParam
)
{
    //GetMonitorInfo(MONITORINFOEX);    // To get device name
    std::vector<MonitorInfo>* monitors = (std::vector<MonitorInfo>*)(void*) lParam;
    monitors->push_back({ hMonitor, *pRect });
    return TRUE;
}

std::vector<MonitorInfo> GetMonitorRects()
{
    std::vector<MonitorInfo> monitors;
    CHECK(LogLevel::ERROR, EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM) (void*) &monitors));
    return monitors;
}

#define WM_MOUSEHOOK    (WM_USER + 372)

/* void Cls::OnMouseHook(UINT uMsg, MSLLHOOKSTRUCT* evt) */
#define HANDLEX_WM_MOUSEHOOK(wParam, lParam, fn) \
    ((fn)((UINT) wParam, (MSLLHOOKSTRUCT*) lParam), 0L)

static HWND s_hWndMouse = NULL;

static LRESULT CALLBACK MouseHookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
    //MSLLHOOKSTRUCT* evt = (MSLLHOOKSTRUCT*) lParam;

    if (wParam == WM_MOUSEMOVE)
        SendMessage(s_hWndMouse, WM_MOUSEHOOK, wParam, lParam);

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

RECT PtExpand(POINT pt, LONG d)
{
    return { pt.x - d, pt.y - d, pt.x + d, pt.y + d };
}

BOOL CreateProcess(HMONITOR hMonitor, LPCTSTR cmd, bool bHidden)
{
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    si.hStdOutput = hMonitor;   // TODO Doesn't appear to be creating the process on the given monitor
    PROCESS_INFORMATION pi = {};
    DWORD Flags = bHidden ? CREATE_NO_WINDOW : 0;
    BOOL ret = CreateProcess(nullptr, const_cast<TCHAR*>(cmd), nullptr, nullptr, FALSE, Flags, nullptr, nullptr, &si, &pi);
    if (ret)
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    return ret;
}

class RootWindow : public Window
{
    friend WindowManager<RootWindow>;
public:
    static ATOM Register() { return WindowManager<RootWindow>::Register(); }
    static RootWindow* Create() { return WindowManager<RootWindow>::Create(); }

    static LPCTSTR ClassName() { return TEXT("RadHotCorner"); }

    static void GetWndClass(WNDCLASS& cs)
    {
        Window::GetWndClass(cs);
        cs.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON_WHITE));
    }

protected:
    static void GetCreateWindow(CREATESTRUCT& cs);

    RootWindow();
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
    BOOL OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();
    void OnMouseHook(UINT uMsg, MSLLHOOKSTRUCT* evt);
    void OnTimer(UINT id);
    void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    void OnDisplayChange(UINT bitsPerPixel, UINT cxScreen, UINT cyScreen);

    struct HotCorner
    {
        HMONITOR hMonitor;
        RECT rc;
        std::wstring cmd;
        bool hidden = false;
    };

    HotCorner* FindHotCorner(POINT pt)
    {
        for (HotCorner& hc : m_corners)
            if (PtInRect(&hc.rc, pt))
                return &hc;
        return nullptr;
    }

    void ReadSettings();

    HHOOK m_hMouseHook = NULL;
    UINT m_Delay = 500;
    std::vector<HotCorner> m_corners;
    HotCorner* m_currentcorner = nullptr;
};

void RootWindow::GetCreateWindow(CREATESTRUCT& cs)
{
    Window::GetCreateWindow(cs);
    cs.lpszName = APP_NAME;
    cs.style = WS_OVERLAPPEDWINDOW;
}

RootWindow::RootWindow()
{
    ReadSettings();
}

BOOL RootWindow::OnCreate(const LPCREATESTRUCT lpCreateStruct)
{
    s_hWndMouse = *this;
    CHECK(LogLevel::ERROR, m_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookCallback, NULL, 0));

    CHECK(LogLevel::ERROR, AddTrayIcon(*this));

    return TRUE;
}

void RootWindow::OnDestroy()
{
    CHECK(LogLevel::ERROR, DeleteTrayIcon(*this));

    CHECK(LogLevel::ERROR, UnhookWindowsHookEx(m_hMouseHook));
    m_hMouseHook = NULL;
    PostQuitMessage(0);
}

void RootWindow::OnMouseHook(UINT uMsg, MSLLHOOKSTRUCT* evt)
{
    switch (uMsg)
    {
    case WM_MOUSEMOVE:
    {
        HotCorner* pCorner = (
            !KEYDOWN(GetKeyState(VK_LBUTTON)) &&
            !KEYDOWN(GetKeyState(VK_RBUTTON)) &&
            !KEYDOWN(GetKeyState(VK_MBUTTON)) &&
            !KEYDOWN(GetKeyState(VK_CONTROL)) &&
            !KEYDOWN(GetKeyState(VK_SHIFT)) &&
            !KEYDOWN(GetKeyState(VK_MENU)) &&
            !KEYDOWN(GetKeyState(VK_LWIN)) &&
            !KEYDOWN(GetKeyState(VK_RWIN)) &&
            !KEYDOWN(GetKeyState(VK_APPS)) &&
            (GetCapture() == NULL))
            ? FindHotCorner(evt->pt) : nullptr;
        if (m_currentcorner != pCorner)
        {
            KillTimer(*this, TIMER_CORNER);
            m_currentcorner = pCorner;
            if (m_currentcorner)
                CHECK(LogLevel::ERROR, SetTimer(*this, TIMER_CORNER, m_Delay, nullptr));
        }
    }
    break;
    }
}

void RootWindow::OnTimer(UINT id)
{
    if (id == TIMER_CORNER)
    {
        KillTimer(*this, TIMER_CORNER);
        if (m_currentcorner != nullptr && !m_currentcorner->cmd.empty())
            if (!CreateProcess(m_currentcorner->hMonitor, m_currentcorner->cmd.c_str(), m_currentcorner->hidden))
                MessageBox(*this, TEXT("Error creating process."), APP_NAME, MB_OK | MB_ICONEXCLAMATION);
    }
}

void RootWindow::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case ID_POPUP_EXIT:
        PostMessage(*this, WM_CLOSE, 0, 0);
        break;
    }
}

void RootWindow::OnDisplayChange(UINT bitsPerPixel, UINT cxScreen, UINT cyScreen)
{
    ReadSettings();
}

void RootWindow::ReadSettings()
{
    m_corners.clear();

    HKEYPtr hKey;
    if (RegOpenKey(HKEY_CURRENT_USER, TEXT(R"-(SOFTWARE\RadSoft\RadHotCorner)-"), &hKey) == ERROR_SUCCESS)
    {
        m_Delay = RegGetDWORD(-hKey, TEXT("delay"), m_Delay);

        const std::vector<MonitorInfo> monitors = GetMonitorRects();

        int id = 1;
        for (const MonitorInfo& mi : monitors)
        {
            TCHAR value[1024];
            HKEYPtr hKeyChild;
            if (RegOpenKey(-hKey, Format(TEXT(R"-(Monitor%d\topleft)-"), id).c_str(), &hKeyChild) == ERROR_SUCCESS)
            {
                if (RegGetString(-hKeyChild, TEXT("cmd"), value, ARRAYSIZE(value)) == ERROR_SUCCESS)
                    m_corners.push_back({ mi.hMonitor, PtExpand({ mi.rRect.left, mi.rRect.top }, 10), value, RegGetDWORD(-hKeyChild, TEXT("hidden"), 0) != 0 });
            }
            if (RegOpenKey(-hKey, Format(TEXT(R"-(Monitor%d\topright)-"), id).c_str(), &hKeyChild) == ERROR_SUCCESS)
            {
                if (RegGetString(-hKeyChild, TEXT("cmd"), value, ARRAYSIZE(value)) == ERROR_SUCCESS)
                    m_corners.push_back({ mi.hMonitor, PtExpand({ mi.rRect.right, mi.rRect.top }, 10), value, RegGetDWORD(-hKeyChild, TEXT("hidden"), 0) != 0 });
            }
            if (RegOpenKey(-hKey, Format(TEXT(R"-(Monitor%d\bottomleft)-"), id).c_str(), &hKeyChild) == ERROR_SUCCESS)
            {
                if (RegGetString(-hKeyChild, TEXT("cmd"), value, ARRAYSIZE(value)) == ERROR_SUCCESS)
                    m_corners.push_back({ mi.hMonitor, PtExpand({ mi.rRect.left, mi.rRect.bottom }, 10), value, RegGetDWORD(-hKeyChild, TEXT("hidden"), 0) != 0 });
            }
            if (RegOpenKey(-hKey, Format(TEXT(R"-(Monitor%d\bottomright)-"), id).c_str(), &hKeyChild) == ERROR_SUCCESS)
            {
                if (RegGetString(-hKeyChild, TEXT("cmd"), value, ARRAYSIZE(value)) == ERROR_SUCCESS)
                    m_corners.push_back({ mi.hMonitor, PtExpand({ mi.rRect.right, mi.rRect.bottom }, 10), value, RegGetDWORD(-hKeyChild, TEXT("hidden"), 0) != 0 });
            }

            ++id;
        }
    }
}

LRESULT RootWindow::HandleMessage(const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
    static UINT s_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
    if (s_uTaskbarRestart != 0 && uMsg == s_uTaskbarRestart)
    {
        CHECK(LogLevel::WARN, AddTrayIcon(*this));
        return TRUE;
    }

    switch (uMsg)
    {
    case WM_TRAY:
        if (LOWORD(lParam) == WM_RBUTTONUP)
        {
            CHECK(LogLevel::ERROR, SetForegroundWindow(*this));
            POINT pt; // = { GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam) };
            CHECK(LogLevel::ERROR, GetCursorPos(&pt));
            const HMENU hMenuBar = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_POPUP_MENU));
            CHECK(LogLevel::ERROR, TrackPopupMenu(GetSubMenu(hMenuBar, 0), TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, *this, nullptr));
            CHECK(LogLevel::ERROR, DestroyMenu(hMenuBar));
        }
        return TRUE;

        HANDLE_MSG(WM_CREATE, OnCreate);
        HANDLE_MSG(WM_DESTROY, OnDestroy);
        HANDLE_MSG(WM_MOUSEHOOK, OnMouseHook);
        HANDLE_MSG(WM_TIMER, OnTimer);
        HANDLE_MSG(WM_COMMAND, OnCommand);
        HANDLE_MSG(WM_DISPLAYCHANGE, OnDisplayChange);
        HANDLE_DEF(Window::HandleMessage);
    }
}

bool Run(_In_ const LPCTSTR lpCmdLine, _In_ const int nShowCmd)
{
    InitLog(NULL, APP_NAME);

    if (FindWindow(RootWindow::ClassName(), nullptr) != NULL)
    {
        MessageBox(NULL, TEXT("Already running"), APP_NAME, MB_ICONERROR | MB_OK);
        return false;
    }

    CHECK_RET(LogLevel::ERROR, RootWindow::Register() != 0, false, TEXT("Error registering window class"));

    RootWindow* prw = RootWindow::Create();
    CHECK_RET(LogLevel::ERROR, prw != nullptr, false, TEXT("Error creating root window"));

    InitLog(*prw, APP_NAME);

    //ShowWindow(*prw, nShowCmd);
    return true;
}
