#include "pch.h"
#include "QueryVideoMemoryDemo.hpp"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	// Allocate demo on the stack.
	QueryVideoMemoryDemo demo(1024, 768, "D3D12 Query Buffer Demo");
	return Win32Application::Run (&demo, hInstance, nCmdShow);
}
