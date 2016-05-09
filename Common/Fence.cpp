/*
 * Fence.cpp
 */

#include "pch.h"
#include "Fence.hpp"

//---------------------------------------------------------------------------------------
Fence::Fence(ID3D12Device * device)
{
    CHECK_DX_RESULT(
        device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->obj))
    );
    this->cpuValue = 1;

    // Create an event handle to use for frame synchronization.
    this->event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (this->event == nullptr) {
        CHECK_DX_RESULT (
            HRESULT_FROM_WIN32(GetLastError())
        );
    }

}

//---------------------------------------------------------------------------------------
Fence::~Fence()
{
    CloseHandle(this->event);
}
