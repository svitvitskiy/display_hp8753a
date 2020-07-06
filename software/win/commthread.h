#pragma once
#include <windows.h>

struct CommThreadArg {
	HWND hWnd;
	HANDLE hComm;

	CommThreadArg(HWND _hWnd, HANDLE _hComm) : hWnd(_hWnd), hComm(_hComm) {
	}
};

DWORD WINAPI CommThreadFunc(LPVOID lpParameter);