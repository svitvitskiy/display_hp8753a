#pragma once
#include <windows.h>
#include <windowsx.h>
#include <d2d1.h>
#include <list>

#include "hp1349renderer.h"

#include "basewin.h"

class CanvasWindow : public BaseWindow<CanvasWindow>
{
    ID2D1Factory*             pFactory;
    ID2D1HwndRenderTarget*    pRenderTarget;
    std::list<unsigned char*> data;
    HP1349Renderer            renderer;    

    void    CalculateLayout() { }
    HRESULT CreateGraphicsResources();
    void    DiscardGraphicsResources();
    void    OnPaint();
    void    Resize(LPARAM lParam, WPARAM wParam);
    void    OnTimer();
    void    Draw(unsigned char* buffer);
    void    ProcessBuffer(unsigned char*);
public:

    CanvasWindow() : pFactory(NULL), pRenderTarget(NULL)
    {
    }

    void GetMessages(std::list<std::string>&);
    PCWSTR  ClassName() const { return L"HP8753A_CANVAS"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};