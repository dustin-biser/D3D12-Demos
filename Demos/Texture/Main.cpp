#include "pch.h"
#include "TextureDemo.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	// Allocate demo on stack.
	TextureDemo demo (1920, 1200, "D3D12 Texture Demo");
	return Win32Application::Run (&demo, hInstance, nCmdShow);
}
