#include "pch.h"

#include "QueryVideoMemoryDemo.hpp"
using namespace DirectX;
using Microsoft::WRL::ComPtr;

#include <iostream>
using namespace std;



//---------------------------------------------------------------------------------------
QueryVideoMemoryDemo::QueryVideoMemoryDemo (
    uint width, 
    uint height,
    std::wstring name
)   
    :   D3D12DemoBase(width, height, name)
{

}


//---------------------------------------------------------------------------------------
void QueryVideoMemoryDemo::initializeDemo()
{

	OutputMemoryBudgets();
}


//---------------------------------------------------------------------------------------
void QueryVideoMemoryDemo::OutputMemoryBudgets()
{
    DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;

    // Query video memory
	QueryVideoMemoryInfo(m_device, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, videoMemoryInfo);
	cout << endl;
    cout << "Video Memory:\n"
         << "Budget: " << videoMemoryInfo.Budget << " bytes" << endl
         << "AvailableForReservation: " << videoMemoryInfo.AvailableForReservation << " bytes" << endl
         << "CurrantUsage: " << videoMemoryInfo.CurrentUsage << " bytes" << endl;
    cout << endl;

    // Query system memory
	QueryVideoMemoryInfo(m_device, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, videoMemoryInfo);
    cout << "System Memory:\n"
         << "Budget: " << videoMemoryInfo.Budget << " bytes" << endl
         << "AvailableForReservation: " << videoMemoryInfo.AvailableForReservation << " bytes" << endl
         << "CurrantUsage: " << videoMemoryInfo.CurrentUsage << " bytes" << endl;
    cout << endl;

}


//---------------------------------------------------------------------------------------
void QueryVideoMemoryDemo::update()
{

}

//---------------------------------------------------------------------------------------
void QueryVideoMemoryDemo::render()
{

}


//---------------------------------------------------------------------------------------
void QueryVideoMemoryDemo::cleanupDemo()
{

}

