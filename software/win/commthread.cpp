#include <Windows.h>

#include "commthread.h"

DWORD WINAPI CommThreadFunc(LPVOID lpParameter)
{
	CommThreadArg* arg = (CommThreadArg*)lpParameter;
	HWND  MainWin = arg->hWnd;
	HANDLE hComm = arg->hComm;
	BOOL  Status;
	DWORD dwEventMask;
	DWORD NoBytesRead;	

	DCB dcbSerialParams = { 0 };
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	Status = GetCommState(hComm, &dcbSerialParams);

	if (Status == FALSE) {
		CloseHandle(hComm);
		return 0;
	}

	dcbSerialParams.BaudRate = CBR_115200;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;

	Status = SetCommState(hComm, &dcbSerialParams);

	if (Status == FALSE) {
		CloseHandle(hComm);
		return 0;
	}

	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = 0;
	timeouts.ReadTotalTimeoutConstant = 100;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 100;
	timeouts.WriteTotalTimeoutMultiplier = 0;

	if (SetCommTimeouts(hComm, &timeouts) == FALSE) {
		CloseHandle(hComm);
		return 0;
	}

	Status = SetCommMask(hComm, EV_RXCHAR);

	if (Status == FALSE) {
		CloseHandle(hComm);
		return 0;
	}
	
	while (true) {		
		unsigned char* SerialBuffer = new unsigned char[205];
		Status = ReadFile(hComm, SerialBuffer + 1, 204, &NoBytesRead, NULL);
		if (Status == FALSE) {
			PostMessage(MainWin, WM_USER, (WPARAM)0, (LPARAM)0);
			break;
		}				
		SerialBuffer[0] = (unsigned char)NoBytesRead;
		if (NoBytesRead > 0)
			PostMessage(MainWin, WM_USER, (WPARAM)SerialBuffer, 0);
		else
			delete[] SerialBuffer;
	}
	delete arg;
}