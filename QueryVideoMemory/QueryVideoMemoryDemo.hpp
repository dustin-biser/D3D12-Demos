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
        std::wstring name
    );

	void initializeDemo() override;

	void update() override;

	void render (
		ID3D12GraphicsCommandList * drawCmdList
	) override;

	void OutputMemoryBudgets();

};
