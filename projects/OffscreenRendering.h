#pragma once

#include "Engine/lib/ComPtr/ComPtr.h"
#include "Engine/lib/Math/MyMath.h"
#include <d3d12.h>

class OffscreenRendering {

public:
	ComPtr<ID3D12Resource> CreateRenderTextureResource(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format, const Vector4& clearColor);


};