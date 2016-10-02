#include "pch.h"
#include "QueryVideoMemoryDemo.hpp"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	QueryVideoMemoryDemo demo(1024, 768, L"D3D12 Query Buffer Demo");

	// Run demo, but hide application window.  Only show console output window.
	return Win32Application::Run(&demo, hInstance, SW_HIDE);
}
