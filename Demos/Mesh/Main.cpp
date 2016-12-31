#include "pch.h"
#include "MeshDemo.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	// Allocate demo on stack.
	MeshDemo demo (1024, 768, "D3D12 Mesh Demo");
	return Win32Application::Run (&demo, hInstance, nCmdShow);
}
