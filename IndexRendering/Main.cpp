/*
 * Main.cpp
 */

#include "pch.h"
#include "IndexRendering.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	IndexRendering sample(1024, 768, L"D3D12 Index Rendering Demo");
	return Win32Application::Run(&sample, hInstance, nCmdShow);
}
