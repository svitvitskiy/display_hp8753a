#pragma once
#include <d2d1.h>
#include <list>
#include <string>

template <class T> void SafeRelease(T** ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}


class HP1349Renderer {
public:
	void Init(ID2D1HwndRenderTarget*);
	void Render(unsigned char*);
	void GetMessages(std::list<std::string> &dest);
	void Destroy();
private:
	void SetCondition(unsigned short command);
	void Plot(unsigned short command);
	void Graph(unsigned short command);
	void Text(unsigned short command);
	void InterpretCommand(unsigned short command);	
private:
	int curX = 0;
	int curY = 0;
	int nextX = 0;
	int incX = 1;
	int intensity = 3;
	int lineType = 0;
	int wrSpeed = 3;
	int frameBegin = 0;
	int readPtr = 0;
	int writePtr = 0;
	unsigned char buffer[256];
	bool aligned = false;
	std::list<std::string> messages;

	ID2D1HwndRenderTarget* target;
	ID2D1SolidColorBrush* pDimBrush;
	ID2D1SolidColorBrush* pHalfBrush;
	ID2D1SolidColorBrush* pFullBrush;
	IDWriteTextFormat* pTextFormat;
	ID2D1BitmapRenderTarget* pOffscreenTarget;
};
