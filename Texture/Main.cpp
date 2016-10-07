#include "pch.h"
#include "TextureDemo.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	TextureDemo demo(1024, 768, L"D3D12 Texture Demo");
	return Win32Application::Run(&demo, hInstance, nCmdShow);
}
