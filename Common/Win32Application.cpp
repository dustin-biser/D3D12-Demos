/*
 * Win32Application.cpp
 */

#include "pch.h"

#include "Win32Application.hpp"
#include <cassert>
#include <chrono>
#include <cwchar>


HWND Win32Application::m_hwnd = nullptr;


int Win32Application::Run (
    D3D12DemoBase * demo,
    HINSTANCE hInstance,
    int nCmdShow
) {
    assert(demo);
	// Parse the command line parameters
	int argc;
	LPWSTR * argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	demo->ParseCommandLineArgs(argv, argc);
	LocalFree(argv);


    //-- Use the following code to open a new console window and redirect stdout to it:
    {
        // Open a new console window
        AllocConsole();

        //-- Associate std input/output with newly opened console window:
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }


	//-- Initialize the window class:
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = L"DXSampleClass";
	RegisterClassEx(&windowClass);

    //-- Center window:
    RECT windowRect;
    GetClientRect(GetDesktopWindow(), &windowRect);
    long width = static_cast<LONG>(demo->GetWidth());
    long height = static_cast<LONG>(demo->GetHeight());
    windowRect.left = (windowRect.right / 2) - (width / 2);
    windowRect.top = (windowRect.bottom / 2) - (height / 2);

	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// Create the window and store a handle to it.
	m_hwnd = CreateWindow (
		windowClass.lpszClassName,
		demo->GetWindowTitle(),
		WS_OVERLAPPEDWINDOW,
		windowRect.left,
		windowRect.top,
        width,
        height,
		nullptr,		// We have no parent window.
		nullptr,		// We aren't using menus.
		hInstance,
		demo         // Store pointer to DXSample in the user data slot.
                        // We'll retrieve this later within the WindowProc in order
                        // interact with the DXSample instance.
    );

	// Initialize the sample. OnInit is defined in each child-implementation of DXSample.
	demo->OnInit();

	ShowWindow(m_hwnd, nCmdShow);

    //-- Timing information:
    static uint32 frameCount(0);
    static float fpsTimer(0.0f);

	// Main sample loop.
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
        // Start frame timer.
        auto timerStart = std::chrono::high_resolution_clock::now();
        // Process any messages in the queue.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // Translate virtual-key codes into character messages.
            TranslateMessage(&msg);

            // Dispatches a message to a the registered window procedure.
            DispatchMessage(&msg);
        }
        demo->OnUpdate();
        demo->OnRender();
        // End frame timer.
        auto timerEnd = std::chrono::high_resolution_clock::now();
        frameCount++;

        auto timeDelta = 
            std::chrono::duration<double, std::milli>(timerEnd - timerStart).count();
        fpsTimer += (float)timeDelta;

        //-- Update window title only after so many milliseconds:
        if (fpsTimer > 400.0f) {
            float fps = float(frameCount) / fpsTimer * 1000.0f;
            wchar_t buffer[256];
            swprintf(buffer, _countof(buffer), L"%s - %.1f fps", demo->GetWindowTitle(), fps);
            SetWindowText(m_hwnd, buffer);

            // Reset timing info.
            fpsTimer = 0.0f;
            frameCount = 0;
        }
	}

	demo->OnDestroy();

	// Return this part of the WM_QUIT message to Windows.
	return static_cast<char>(msg.wParam);
}

// Main message handler for the sample.
LRESULT CALLBACK Win32Application::WindowProc (
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    // Retrieve a pointer to the DXSample instance held by the user data field of our 
    // window instance.
	D3D12DemoBase * pSample = reinterpret_cast<D3D12DemoBase*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message)
	{
	case WM_CREATE:
		{
			// Save the DXSample* passed in to CreateWindow and store it in the window's
            // user data field so we can retrieve it later.
			LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
			SetWindowLongPtr(hWnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
		}
		return 0;

	case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        }
		else if (pSample)
		{
			pSample->OnKeyDown(static_cast<UINT8>(wParam));
		}
		return 0;

	case WM_KEYUP:
		if (pSample)
		{
			pSample->OnKeyUp(static_cast<UINT8>(wParam));
		}
		return 0;

	case WM_PAINT:
        ValidateRect(hWnd, NULL);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	// Handle any messages the switch statement didn't.
	return DefWindowProc(hWnd, message, wParam, lParam);
}
