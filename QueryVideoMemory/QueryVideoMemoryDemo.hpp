#pragma once

#include <memory>
#include <vector>
#include <DirectXMath.h>

#include "Common/D3D12DemoBase.hpp"

class QueryVideoMemoryDemo : public D3D12DemoBase {
public:
	QueryVideoMemoryDemo (
        uint width,
        uint height,
        std::string name
    );

	void InitializeDemo (
		ID3D12GraphicsCommandList * uploadCmdList
	) override;

	void Update() override;

	void Render (
		ID3D12GraphicsCommandList * drawCmdList
	) override;

	void OutputMemoryBudgets();

};
