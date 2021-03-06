/*
 * Main.cpp
 */

#include "pch.h"
#include "IndexRendering.hpp"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	// Allocate demo on the stack.
	IndexRendering demo (1024, 768, "D3D12 Index Rendering");
	return Win32Application::Run (&demo, hInstance, nCmdShow);
}
