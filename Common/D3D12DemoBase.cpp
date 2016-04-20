/*
 * D3D12DemoBase.cpp
 */

#include "pch.h"
#include "D3D12DemoBase.h"

using namespace Microsoft::WRL;

//---------------------------------------------------------------------------------------
D3D12DemoBase::D3D12DemoBase(UINT width, UINT height, std::wstring name) :
	m_width(width),
	m_height(height),
	m_title(name),
	m_useWarpDevice(false)
{
    //-- Set working directory path:
    {
        WCHAR pathBuffer[512];
        GetWorkingDir(pathBuffer, _countof(pathBuffer));
        m_workingDirPath = std::wstring(pathBuffer);
    }

    //-- Set shared asset path:
    {
        WCHAR pathBuffer[512];
        GetSolutionDir(pathBuffer, _countof(pathBuffer));
        m_sharedAssetPath = std::wstring(pathBuffer) + L"Assets\\";
    }

	m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}


//---------------------------------------------------------------------------------------
D3D12DemoBase::~D3D12DemoBase()
{

}

//---------------------------------------------------------------------------------------
std::wstring D3D12DemoBase::GetAssetPath(LPCWSTR assetName)
{
    assert(assetName);

    // Compiled shader code .cso files should be in the current working directory,
    // where as all other assets (e.g. textures, meshes, etc.) should be located in
    // the shared asset path.
    const wchar_t * stringMatch = wcsstr(assetName, L".cso");
    if (stringMatch) {
        return m_workingDirPath + assetName;
    }

    // .obj files reside in Assets\Meshes\ folder.
    stringMatch = wcsstr(assetName, L".obj");
    if (stringMatch) {
        return m_sharedAssetPath + L"Meshes\\" + assetName;
    }

    return m_sharedAssetPath + assetName;

}


//---------------------------------------------------------------------------------------
std::string D3D12DemoBase::GetAssetPath(const char * assetName)
{
    const int BUFFER_LENGTH = 512;

    wchar_t wcsBuffer[BUFFER_LENGTH]; // wide character string buffer.
    size_t result = mbstowcs(wcsBuffer, assetName, BUFFER_LENGTH);
    if (result == BUFFER_LENGTH) {
        // assetName path is too long to fit in buffer.
        throw;
    }
    std::wstring assetPath = this->GetAssetPath(wcsBuffer);

    char mbsBuffer[BUFFER_LENGTH]; // multi-byte string buffer.
    result = wcstombs(mbsBuffer, assetPath.c_str(), BUFFER_LENGTH);
    if (result == BUFFER_LENGTH) {
        // assetName path is too long to fit in buffer.
        throw;
    }

    return std::string(mbsBuffer);
}

//---------------------------------------------------------------------------------------
// Helper function for acquiring the first available hardware adapter that supports
// Direct3D 12. If no such adapter can be found, *ppAdapter will be set to nullptr.
void D3D12DemoBase::GetHardwareAdapter (
    IDXGIFactory2 * pFactory,
    IDXGIAdapter1 ** ppAdapter
) {
	ComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;

    UINT adapterIndex(0);
    while(DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter))
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			// If you want a software adapter, pass in "/warp" on the command line.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(),
            D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}

        ++adapterIndex;
	}

    // Return first hardware adapter found, that supports the specified D3D feature set.
    // Call Detach() so that the ComPtr does not destroy the interface object when exiting
    // the current scope.
    *ppAdapter = adapter.Detach(); 

}

//---------------------------------------------------------------------------------------
// Helper function for setting the window's title text.
void D3D12DemoBase::SetCustomWindowText(LPCWSTR text)
{
	std::wstring windowText = m_title + L": " + text;
	SetWindowText(Win32Application::GetHwnd(), windowText.c_str());
}


//---------------------------------------------------------------------------------------
// Helper function for parsing any supplied command line args.
void D3D12DemoBase::ParseCommandLineArgs(WCHAR* argv[], int argc)
{
	for (int i = 1; i < argc; ++i)
	{
		if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 || 
			_wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
		{
			m_useWarpDevice = true;
			m_title = m_title + L" (WARP)";
		}
	}
}
