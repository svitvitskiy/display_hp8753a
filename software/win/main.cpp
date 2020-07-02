#include <windows.h>
#include <windowsx.h>
#include <list>
#include <vector>
#include "basewin.h"
#include "canvas.h"

class MainWindow : public BaseWindow<MainWindow>
{
    HWND                        m_hwndEdit;
    CanvasWindow                m_canvas;    
    std::string                 m_messages;

    void    CalculateLayout() { }    
    void    Resize(LPARAM lParam, WPARAM wParam);    
    void    OnTimer(LPARAM lParam, WPARAM wParam);
public:

    MainWindow() : m_hwndEdit(NULL)
    {
    }
    void    CreateLayout();
    CanvasWindow& Canvas() { return m_canvas; }
    PCWSTR  ClassName() const { return L"HP8753A"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

void MainWindow::Resize(LPARAM lParam, WPARAM wParam)
{
    WORD height = HIWORD(lParam);
    WORD width = LOWORD(lParam);
    MoveWindow(m_hwndEdit, 0, height - 50, width, 50, TRUE);
    MoveWindow(m_canvas.Window(), 0, 0, width, height - 50, TRUE);
}

void MainWindow::CreateLayout() {
    m_hwndEdit = CreateWindowEx(
        0, L"EDIT",
        NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL |
        ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        0, 0, 0, 0,
        m_hwnd,
        (HMENU)42,
        (HINSTANCE)GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE),
        NULL);
    m_canvas.Create(NULL, WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE, 0,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        m_hwnd);    
    SendMessage(m_hwndEdit, WM_SETFONT, (WPARAM)GetStockObject(ANSI_VAR_FONT), TRUE);
}

void MainWindow::OnTimer(LPARAM lParam, WPARAM wParam) {
    std::list<std::string> messages;
    m_canvas.GetMessages(messages); 
    if (!messages.empty()) {
        m_messages.clear();
        for (std::list<std::string>::const_iterator i = messages.begin(); i != messages.end(); ++i)
            m_messages += *i + "\r\n";
        //SendMessage(m_hwndEdit, WM_SETTEXT, 0, (LPARAM)m_messages.c_str());
        int index = GetWindowTextLength(m_hwndEdit);
        //SetFocus(hEdit); // set focus
        SendMessageA(m_hwndEdit, EM_SETSEL, (WPARAM)index, (LPARAM)index); // set selection - end of text
        SendMessageA(m_hwndEdit, EM_REPLACESEL, 0, (LPARAM)m_messages.c_str()); // append!        
    }
}

DWORD WINAPI CommThreadFunc(LPVOID lpParameter);

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        SetTimer(m_hwnd, 1, 100, (TIMERPROC)NULL);
        return 0;

    case WM_DESTROY:
        KillTimer(m_hwnd, 1);
        PostQuitMessage(0);
        return 0;    

    case WM_SIZE:
        Resize(lParam, wParam);
        return 0;

    case WM_TIMER:
        OnTimer(lParam, wParam);
        return 0;

    case WM_CTLCOLORSTATIC:
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, GetSysColor(COLOR_INACTIVECAPTIONTEXT));
        SetBkColor(hdcStatic, GetSysColor(COLOR_INACTIVECAPTION));
        return (INT_PTR)GetSysColorBrush(COLOR_INACTIVECAPTION);
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    MainWindow win;

    if (!win.Create(L"HP8753A/B", WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MINIMIZEBOX ^ WS_MAXIMIZEBOX))
    {
        return 0;
    }
    win.CreateLayout();    

    HANDLE hThread = CreateThread(NULL, 0, CommThreadFunc, (LPVOID)win.Canvas().Window(), 0, NULL);
    if (hThread == NULL) {
        return 1;
    }

    ShowWindow(win.Window(), nCmdShow);
    RECT winRect, clientRect;
    GetWindowRect(win.Window(), &winRect);
    GetClientRect(win.Window(), &clientRect);
    int wDiff = (winRect.right - winRect.left) - (clientRect.right - clientRect.left);
    int hDiff = (winRect.bottom - winRect.top) - (clientRect.bottom - clientRect.top);

    MoveWindow(win.Window(), winRect.left, winRect.top, 800 + wDiff, 850 + hDiff, FALSE);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    CloseHandle(hThread);
    return 0;
}