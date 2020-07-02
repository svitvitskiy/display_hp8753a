#include <windows.h>
#include <windowsx.h>
#include "hp1349renderer.h"
#include <dwrite.h>

static int fromHex(unsigned char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}

void HP1349Renderer::Init(ID2D1HwndRenderTarget* target) {
	this->target = target;
	HRESULT hr;
	D2D1_SIZE_F size = target->GetSize();
	hr = target->CreateCompatibleRenderTarget(
		size,
		&pOffscreenTarget);
	
	IDWriteFactory* pDWriteFactory;
	hr = pOffscreenTarget->CreateSolidColorBrush(D2D1::ColorF(0.8f, 0.8f, 0.8f), &pDimBrush);
	hr = pOffscreenTarget->CreateSolidColorBrush(D2D1::ColorF(0.5f, 0.5f, 0.5f), &pHalfBrush);
	hr = pOffscreenTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f), &pFullBrush);
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(pDWriteFactory), reinterpret_cast<IUnknown**>(&pDWriteFactory));
	hr = pDWriteFactory->CreateTextFormat(L"Courier New",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		12,
		L"", //locale
		&pTextFormat
	);
	messages.push_back("Waiting for HP8753A");
}

void HP1349Renderer::Destroy() {
	SafeRelease(&pDimBrush);
	SafeRelease(&pHalfBrush);
	SafeRelease(&pFullBrush);
}

void HP1349Renderer::Render(unsigned char* data) {
	unsigned int dataLen = data[0];

	if (dataLen > 204)
		return;
	data++;

	for (int i = 0; i < dataLen; i++) {
		buffer[writePtr++] = data[i];
	}
	
	while (!aligned && readPtr < writePtr) {
		if (buffer[readPtr] == 'F' && buffer[readPtr + 1] == 'F' && buffer[readPtr + 2] == 'F' && buffer[readPtr + 3] == 'F') {
			aligned = true;
			messages.push_back("Connected");
		} else {
			++readPtr;
		}
	}

	while (readPtr + 3 < writePtr) {
		unsigned short command = (fromHex(buffer[readPtr + 0]) << 12) | (fromHex(buffer[readPtr + 1]) << 8) | (fromHex(buffer[readPtr + 2]) << 4) | (fromHex(buffer[readPtr + 3]));
		if (command != 0xFFFF) {
			InterpretCommand(command);
		}
		readPtr += 4;
	}

	for (int i = 0; i < writePtr - readPtr; i++) {
		buffer[i] = buffer[i + readPtr];
	}
	writePtr -= readPtr;
	readPtr = 0;
}

void HP1349Renderer::GetMessages(std::list<std::string> &dest) 
{
	dest.insert(dest.end(), messages.begin(), messages.end());
	messages.clear();
}

void HP1349Renderer::SetCondition(unsigned short command) {
	intensity = (command >> 11) & 0x3;
	lineType = (command >> 7) & 0x7;
	wrSpeed = (command >> 3) & 0x3;	
}

void HP1349Renderer::Plot(unsigned short command) {
	int coord = command & 0x7ff;
	int coordS1 = coord >> 1;
	int xy = (command >> 12) & 0x1;
	int pc = (command >> 11) & 0x1;

	if (frameBegin == 0 && coordS1 == 0 && xy == 0 && pc == 0) {
		frameBegin = 1;
	} else if (frameBegin == 1 && coordS1 == 1005 && xy == 1 && pc == 0) {
		frameBegin = 2;
	} else {
		frameBegin = 0;
	}

	if (xy == 0) {
		nextX = coord;
	} else {
		if (pc == 1 && intensity != 0 && lineType == 0 && wrSpeed == 3) {
			D2D1_SIZE_F size = pOffscreenTarget->GetSize();
			ID2D1SolidColorBrush* brush = intensity == 1 ? pDimBrush : (intensity == 2 ? pHalfBrush : pFullBrush);
			pOffscreenTarget->BeginDraw();
			pOffscreenTarget->DrawLine(
				D2D1::Point2F(int(curX * size.width) >> 11, size.height - (int(curY * size.height) >> 11)),
				D2D1::Point2F(int(nextX * size.width) >> 11, size.height - (int(coord * size.height) >> 11)),
				pFullBrush,
				0.5f
			);
			pOffscreenTarget->EndDraw();
		}
		curX = nextX;
		curY = coord;
	}
}

void HP1349Renderer::Graph(unsigned short command) {	
	int coord = command & 0x7ff;
	int xy = (command >> 12) & 0x1;
	int pc = (command >> 11) & 0x1;

	if (xy == 0) {
		incX = coord;
	}
	else {
		if (pc == 1 && intensity != 0 && lineType == 0 && wrSpeed == 3) {
			D2D1_SIZE_F size = pOffscreenTarget->GetSize();
			ID2D1SolidColorBrush* brush = intensity == 1 ? pDimBrush : (intensity == 2 ? pHalfBrush : pFullBrush);
			pOffscreenTarget->BeginDraw();
			pOffscreenTarget->DrawLine(
				D2D1::Point2F(int(curX * size.width) >> 11, size.height - (int(curY * size.height) >> 11)),
				D2D1::Point2F(int((curX + incX) * size.width) >> 11, size.height - (int(coord * size.height) >> 11)),
				pFullBrush,
				0.5f
			);
			pOffscreenTarget->EndDraw();
		}
		curX += incX;
		curY = coord;
	}
}

void HP1349Renderer::Text(unsigned short command) {
	int character = command & 0x7f;
	
	if (character == 10) {
		curX = 0;
	}
	else if (character == 13) {
		curY += 24;
	}
	else {
		HRESULT hr;
		if (intensity != 0) {
			bool frameStart = false;			
			if (frameBegin == 2 && character == 'C') {
				frameBegin = 3;
			} else if (frameBegin == 3 && character == 'H') {
				frameBegin = 0;
				frameStart = true;
			} else {
				frameBegin = 0;
			}
			if (frameStart) {
				ID2D1Bitmap* pGridBitmap = NULL;
				hr = pOffscreenTarget->GetBitmap(&pGridBitmap);
				if (SUCCEEDED(hr))
				{
					D2D1_SIZE_F size = pGridBitmap->GetSize();
					target->BeginDraw();
					target->DrawBitmap(pGridBitmap, D2D1::RectF(0, 0, size.width, size.height));
					target->EndDraw();
					pGridBitmap->Release();
				}
				pOffscreenTarget->BeginDraw();
				pOffscreenTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));
				pOffscreenTarget->EndDraw();
			}
			D2D1_SIZE_F size = pOffscreenTarget->GetSize();
			ID2D1SolidColorBrush* brush = intensity == 1 ? pDimBrush : (intensity == 2 ? pHalfBrush : pFullBrush);
			WCHAR c = (const char)character;
			int left = int(curX * size.width) >> 11;
			int top = (size.height - (int(curY * size.height) >> 11)) - 12;
			pOffscreenTarget->BeginDraw();
			pOffscreenTarget->DrawText(&c, 1, pTextFormat,
				D2D1::RectF(left, top, left + 24, top + 24),
				brush
			);
			pOffscreenTarget->EndDraw();
		}
		curX += 24;
	}
}

void HP1349Renderer::InterpretCommand(unsigned short command) {
	int type = (command >> 13) & 0x3;

	switch (type) {
	case 0:
		Plot(command);
		break;
	case 1:
		Graph(command);
		break;
	case 2:
		Text(command);
		break;
	case 3:
		SetCondition(command);
		break;
	}
}