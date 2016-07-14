#pragma once

#include <Windows.h>

class D3D12DemoBase;

class Win32Application {
public:
	static int Run (
        D3D12DemoBase * demo,
        HINSTANCE hInstance,
        int nCmdShow
    );

	static HWND GetHwnd () {
        return m_hwnd;
    }

protected:
	static LRESULT CALLBACK WindowProc (
        HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    );

private:
	static HWND m_hwnd;

};
