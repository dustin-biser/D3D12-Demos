/*
 * Fence.cpp
 */

#include "pch.h"
#include "Fence.hpp"

//---------------------------------------------------------------------------------------
Fence::Fence(ID3D12Device * device)
{
    ThrowIfFailed(
        device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->obj))
    );
    this->value = 1;

    // Create an event handle to use for frame synchronization.
    this->event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (this->event == nullptr) {
        ThrowIfFailed (
            HRESULT_FROM_WIN32(GetLastError())
        );
    }

}

//---------------------------------------------------------------------------------------
Fence::~Fence()
{
    CloseHandle(this->event);
}
