#include "pch.h"

#include <cassert>
#include <chrono>
#include <cwchar>

#include "Win32Application.hpp"
#include "D3D12DemoBase.hpp"

#include "Windowsx.h"  // using GET_X_LPARAM
					   // using GET_Y_LPARAM


HWND Win32Application::m_hwnd = nullptr;


int Win32Application::Run (
    D3D12DemoBase * demo,
    HINSTANCE hInstance,
    int nCmdShow
) {
    assert(demo);

    //-- Open a new console window and redirect std streams to it:
    {
        // Open a new console window
        AllocConsole();

        //-- Associate std input/output with newly opened console window:
        FILE * file0 = freopen("CONIN$", "r", stdin);
        FILE * file1 = freopen("CONOUT$", "w", stdout);
        FILE * file2 = freopen("CONOUT$", "w", stderr);
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
	long width = static_cast<LONG>(demo->GetWindowWidth());
	long height = static_cast<LONG>(demo->GetWindowHeight());
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
		demo            // Store pointer to the D3D12Demo in the user data slot.
                        // We'll retrieve this later within the WindowProc in order
                        // interact with the D3D12Demo instance.
    );

	// Run setup code common to all demos
	demo->Initialize();

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
        demo->BuildNextFrame();
		demo->PresentNextFrame();

        // End frame timer.
        auto timerEnd = std::chrono::high_resolution_clock::now();
        ++frameCount;

        auto timeDelta = 
            std::chrono::duration<double, std::milli>(timerEnd - timerStart).count();
        fpsTimer += (float)timeDelta;

        //-- Update window title only after so many milliseconds:
        if (fpsTimer > 400.0f) {
			float msPerFrame = fpsTimer / float(frameCount);
            float fps = float(frameCount) / fpsTimer * 1000.0f;
            wchar_t buffer[256];
			swprintf(buffer, _countof(buffer), L"%s - %.1f fps (%.2f ms)",
				demo->GetWindowTitle(), fps, msPerFrame);
            SetWindowText(m_hwnd, buffer);

            // Reset timing info.
            fpsTimer = 0.0f;
            frameCount = 0;
        }
	}

	// Flush command-queue before releasing resources.
	demo->PrepareCleanup();

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
    // Retrieve a pointer to the D3D12DemoBase instance held by the user data field of our 
    // window instance.
	D3D12DemoBase * demo =
		reinterpret_cast<D3D12DemoBase*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message) {
	case WM_CREATE:
	{
		// Save the D3D12DemoBase ptr passed into CreateWindow and store it in the
		// window's user data field so we can retrieve it later.
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA,
			reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));

		return 0;
	}

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			PostQuitMessage(0);
		}
		else if (demo) {
			demo->OnKeyDown(static_cast<UINT8>(wParam));
		}
		return 0;

	case WM_KEYUP:
		if (demo) {
			demo->OnKeyUp(static_cast<UINT8>(wParam));
		}
		return 0;

	case WM_LBUTTONDOWN:
	{
		if (demo) {
			const int xPos = GET_X_LPARAM(lParam);
			const int yPos = GET_Y_LPARAM(lParam);
			demo->UpdateMousePosition(xPos, yPos);
			demo->MouseLButtonDown();
		}

		return 0;
	}

	case WM_LBUTTONUP:
	{
		if (demo) {
			demo->MouseLButtonUp();
		}

		return 0;
	}

	case WM_MOUSEMOVE:
	{
		if (wParam & MK_LBUTTON) {
			if (demo) {
				ScreenPosition mousePos = demo->GetMousePosition();

				const int xPos = GET_X_LPARAM(lParam);
				const int yPos = GET_Y_LPARAM(lParam);
				demo->OnMouseMove(xPos - mousePos.x, yPos - mousePos.y);

				demo->UpdateMousePosition(xPos, yPos);
			}
		}
		return 0;
	}

	case WM_SIZE:
		if (demo) {
			const uint width = LOWORD(lParam);
			const uint height = HIWORD(lParam);
			demo->OnResize(width, height);
		}
		return 0;

	case WM_PAINT:
        ValidateRect(hWnd, NULL);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	// Handle any messages the switch statement above didn't.
	return DefWindowProc(hWnd, message, wParam, lParam);
}
