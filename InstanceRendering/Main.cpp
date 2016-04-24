/*
 * Main.cpp
 */

#include "pch.h"
#include "InstanceRendering.hpp"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	InstanceRendering demo(1024, 768, L"D3D12 Instance Rendering Demo");
	return Win32Application::Run(&demo, hInstance, nCmdShow);
}
