#include <windows.h>
#include <windowsx.h>
#include <CommCtrl.h>

#include <list>
#include <vector>

#include "basewin.h"
#include "canvas.h"
#include "commthread.h"

static void EnumerateCOMPorts(std::list<std::wstring>& ports)
{
    WCHAR buf[32];
    for (unsigned int i = 1; i < 9; i++)
    {
        //wsprintf(buf, L"\\\\.\\COM%u", i);
        wsprintf(buf, L"COM%u", i);
        
        HANDLE hPort = CreateFile(buf, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hPort == INVALID_HANDLE_VALUE)
        {
            const DWORD dwError = GetLastError();
            if ((dwError == ERROR_ACCESS_DENIED) || (dwError == ERROR_GEN_FAILURE) || (dwError == ERROR_SHARING_VIOLATION) || (dwError == ERROR_SEM_TIMEOUT))
                ports.push_back(buf);
        }
        else
        {
            ports.push_back(buf);
            CloseHandle(hPort);
        }        
    }
}

class MainWindow : public BaseWindow<MainWindow>
{
    HWND                        m_hwndEdit;
    HWND                        m_hwndPort;
    CanvasWindow                m_canvas;    
    std::string                 m_messages;
    HANDLE                      m_hThread;
    HANDLE                      m_hComm;

    void    CalculateLayout() { }    
    void    Resize(LPARAM lParam, WPARAM wParam);    
    void    OnTimer(LPARAM lParam, WPARAM wParam);
public:

    MainWindow() : m_hwndEdit(NULL), m_hwndPort(NULL), m_hThread(NULL), m_hComm(NULL)
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
    MoveWindow(m_hwndEdit, 0, height - 100, width - 100, 100, TRUE);
    MoveWindow(m_hwndPort, width - 100, height - 100, 100, 100, TRUE);
    MoveWindow(m_canvas.Window(), 0, 0, width, height - 100, TRUE);
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
    m_hwndPort = CreateWindowEx(0, WC_COMBOBOX, TEXT(""),
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, m_hwnd, NULL, (HINSTANCE)GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE),
        NULL);
    m_canvas.Create(NULL, WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE, 0,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        m_hwnd);    
    SendMessage(m_hwndEdit, WM_SETFONT, (WPARAM)GetStockObject(ANSI_VAR_FONT), TRUE);
    SendMessage(m_hwndPort, WM_SETFONT, (WPARAM)GetStockObject(ANSI_VAR_FONT), TRUE);
    SendMessage(m_hwndPort, CB_SETEXTENDEDUI, FALSE, FALSE);
    
    std::list<std::wstring> ports;
    EnumerateCOMPorts(ports);
    std::list<std::wstring>::const_iterator iterator;
    for (iterator = ports.begin(); iterator != ports.end(); ++iterator) {
        const WCHAR* const str = iterator->c_str();
        SendMessage(m_hwndPort, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)str);
    }
    SendMessage(m_hwndPort, CB_SETCURSEL, (WPARAM)(ports.size() - 1), (LPARAM)0);
        
    PostMessage(m_hwnd, WM_COMMAND, MAKEWPARAM(0, CBN_SELCHANGE), (LPARAM)m_hwndPort);
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

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        SetTimer(m_hwnd, 1, 100, (TIMERPROC)NULL);
        return 0;

    case WM_DESTROY:
        KillTimer(m_hwnd, 1);
        if (m_hThread != NULL) {
            CloseHandle(m_hThread);
        }
        if (m_hComm != NULL) {
            CloseHandle(m_hComm);
        }
        PostQuitMessage(0);
        return 0;    

    case WM_SIZE:
        Resize(lParam, wParam);
        return 0;

    case WM_TIMER:
        OnTimer(lParam, wParam);
        return 0;
    case WM_COMMAND:        
        if (HIWORD(wParam) == CBN_SELCHANGE && (HWND)lParam == m_hwndPort) {
            int idx = SendMessage(m_hwndPort, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
            WCHAR  portSelected[32];
            SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT,
                (WPARAM)idx, (LPARAM)portSelected);
            if (m_hComm != NULL) {
                CloseHandle(m_hComm);
            }
            m_hComm = CreateFile(portSelected, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (m_hComm == INVALID_HANDLE_VALUE) {
                m_hComm = NULL;
                return 0;
            }

            CommThreadArg* arg = new CommThreadArg(m_canvas.Window(), m_hComm);                        
            m_hThread = CreateThread(NULL, 0, CommThreadFunc, (LPVOID)arg, 0, NULL);
            if (m_hThread == INVALID_HANDLE_VALUE) {
                m_hThread = NULL;
                return 0;
            }
        }
        return 0;

    case WM_CTLCOLORLISTBOX:
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

    ShowWindow(win.Window(), nCmdShow);
    RECT winRect, clientRect;
    GetWindowRect(win.Window(), &winRect);
    GetClientRect(win.Window(), &clientRect);
    int wDiff = (winRect.right - winRect.left) - (clientRect.right - clientRect.left);
    int hDiff = (winRect.bottom - winRect.top) - (clientRect.bottom - clientRect.top);

    MoveWindow(win.Window(), winRect.left, winRect.top, 800 + wDiff, 900 + hDiff, FALSE);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }    
    return 0;
}