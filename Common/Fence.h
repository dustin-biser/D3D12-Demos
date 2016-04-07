// Fence.h

#pragma once

#include <wrl.h>
#include "NumericTypes.h"
#include <d3d12.h>

// Primitive for synchronizing D3D12 Resources between CPU and GPU
class Fence {
public:
    Fence(ID3D12Device * device);
    ~Fence();

    Microsoft::WRL::ComPtr<ID3D12Fence> obj;
    uint64 value;
    HANDLE event;
};