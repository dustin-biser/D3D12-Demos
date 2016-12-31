#include "pch.h"

#include "QueryVideoMemoryDemo.hpp"
using namespace DirectX;
using Microsoft::WRL::ComPtr;

#include <iostream>
#include <sstream>
using namespace std;



//---------------------------------------------------------------------------------------
QueryVideoMemoryDemo::QueryVideoMemoryDemo (
    uint width, 
    uint height,
    std::string name
)   
    :   D3D12DemoBase(width, height, name)
{

}

//---------------------------------------------------------------------------------------
void QueryVideoMemoryDemo::InitializeDemo (
	ID3D12GraphicsCommandList * uploadCmdList
) {
	OutputMemoryBudgets();
}


//---------------------------------------------------------------------------------------
void QueryVideoMemoryDemo::OutputMemoryBudgets()
{
    DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;

    // Query video memory
	QueryVideoMemoryInfo(m_device.Get(), DXGI_MEMORY_SEGMENT_GROUP_LOCAL, videoMemoryInfo);
	std::stringstream stream;
    stream << "Video Memory:\n"
         << "Budget: " << videoMemoryInfo.Budget << " bytes" << endl
         << "AvailableForReservation: " << videoMemoryInfo.AvailableForReservation << " bytes" << endl
         << "CurrantUsage: " << videoMemoryInfo.CurrentUsage << " bytes" << endl;
    stream << endl;

    // Query system memory
	QueryVideoMemoryInfo(m_device.Get(), DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, videoMemoryInfo);
    stream << "System Memory:\n"
         << "Budget: " << videoMemoryInfo.Budget << " bytes" << endl
         << "AvailableForReservation: " << videoMemoryInfo.AvailableForReservation << " bytes" << endl
         << "CurrantUsage: " << videoMemoryInfo.CurrentUsage << " bytes" << endl;
    stream << endl;

	LOG_INFO ("Query Video Memory\n%s", stream.str ().c_str ());
}


//---------------------------------------------------------------------------------------
void QueryVideoMemoryDemo::Update()
{

}

//---------------------------------------------------------------------------------------
void QueryVideoMemoryDemo::Render (
	ID3D12GraphicsCommandList * drawCmdList
) {

}
