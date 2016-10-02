#include "pch.h"
#include "TexturedCubeDemo.hpp"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	TexturedCubeDemo demo(1024, 768, L"D3D12 Textured Cube Demo");
	return Win32Application::Run(&demo, hInstance, nCmdShow);
}
