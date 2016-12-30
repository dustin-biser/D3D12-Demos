#include "pch.h"
#include "ConstantBufferDemo.hpp"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	ConstantBufferDemo demo(1024, 768, "D3D12 Constant Buffer Demo");
	return Win32Application::Run (&demo, hInstance, nCmdShow);
}
