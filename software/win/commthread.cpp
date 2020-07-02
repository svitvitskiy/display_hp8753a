#include <Windows.h>

DWORD WINAPI CommThreadFunc(LPVOID lpParameter)
{
	HWND  MainWin = (HWND)lpParameter;
	BOOL  Status;
	DWORD dwEventMask;
	DWORD NoBytesRead;

	HANDLE hComm = CreateFile(L"COM5", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (hComm == INVALID_HANDLE_VALUE) {
		return 0;
	}

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
	timeouts.ReadIntervalTimeout = 5000;
	timeouts.ReadTotalTimeoutConstant = 5000;
	timeouts.ReadTotalTimeoutMultiplier = 100;
	timeouts.WriteTotalTimeoutConstant = 5000;
	timeouts.WriteTotalTimeoutMultiplier = 1000;

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
		Status = WaitCommEvent(hComm, &dwEventMask, NULL);

		if (Status == FALSE) {
			break;
		} else {
			do {
				unsigned char* SerialBuffer = new unsigned char[205];
				Status = ReadFile(hComm, SerialBuffer + 1, 204, &NoBytesRead, NULL);
				SerialBuffer[0] = (unsigned char)NoBytesRead;
				if (NoBytesRead > 0)
					PostMessage(MainWin, WM_USER, (WPARAM)SerialBuffer, 0);
				else
					delete[] SerialBuffer;
			} while (NoBytesRead > 0);
		}
	}
	CloseHandle(hComm);
}