#include "canvas.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "Dwrite")

class DPIScale
{
    static float scaleX;
    static float scaleY;

public:
    static void Initialize(ID2D1Factory* pFactory)
    {
        FLOAT dpiX, dpiY;
        pFactory->GetDesktopDpi(&dpiX, &dpiY);
        scaleX = dpiX / 96.0f;
        scaleY = dpiY / 96.0f;
    }

    template <typename T>
    static D2D1_POINT_2F PixelsToDips(T x, T y)
    {
        return D2D1::Point2F(static_cast<float>(x) / scaleX, static_cast<float>(y) / scaleY);
    }
};

float DPIScale::scaleX = 1.0f;
float DPIScale::scaleY = 1.0f;


HRESULT CanvasWindow::CreateGraphicsResources()
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &pRenderTarget);

        if (SUCCEEDED(hr))
        {
            renderer.Init(pRenderTarget);
            CalculateLayout();
        }

    }
    return hr;
}

void CanvasWindow::DiscardGraphicsResources()
{
    renderer.Destroy();
    SafeRelease(&pRenderTarget);
}

void CanvasWindow::Draw(unsigned char* buffer) {
    renderer.Render(buffer);
}

void CanvasWindow::OnPaint()
{
    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {
        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        while (!data.empty()) {
            unsigned char* buffer = data.front();
            Draw(buffer);
            data.pop_front();
            delete[] buffer;
        }

        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
        {
            DiscardGraphicsResources();
        }
        EndPaint(m_hwnd, &ps);
    }
}

void CanvasWindow::Resize(LPARAM lParam, WPARAM wParam)
{
    if (pRenderTarget != NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        pRenderTarget->Resize(size);
        CalculateLayout();
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}

void CanvasWindow::OnTimer()
{
    InvalidateRect(m_hwnd, NULL, FALSE);
}

void CanvasWindow::ProcessBuffer(unsigned char* buffer) {
    data.push_back(buffer);
}

void CanvasWindow::GetMessages(std::list<std::string> &messages) {
    renderer.GetMessages(messages);
}

LRESULT CanvasWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        if (FAILED(D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
        {
            return -1;  // Fail CreateWindowEx.
        }
        DPIScale::Initialize(pFactory);
        SetTimer(m_hwnd, 1, 1, (TIMERPROC)NULL);

        return 0;

    case WM_TIMER:
        OnTimer();
        return 0;

    case WM_DESTROY:
        KillTimer(m_hwnd, 1);
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_SIZE:
        Resize(lParam, wParam);
        return 0;

    case WM_USER:
        ProcessBuffer((unsigned char*)wParam);
        return 0;
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}