/*
 * Main.cpp
 */

#include "pch.h"
#include "FrameBuffering.hpp"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	FrameBuffering demo(1024, 768, L"D3D12 Frame Buffering Demo");
	return Win32Application::Run(&demo, hInstance, nCmdShow);
}
