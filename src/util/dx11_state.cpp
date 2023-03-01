#include "dx11_state.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define UUID_DEFINED
#include <Windows.h>
#include <d3d11.h>
#include <D3D11_1.h>
#endif
#include "halley/src/plugins/dx11/src/dx11_video.h"

namespace {
	class DX11StateInternal final : public DX11State::IStateInternal {
	public:
		ID3D11Buffer* indexBuffer;
		DXGI_FORMAT indexBufferFormat;
		UINT indexBufferOffset;
		ID3D11InputLayout* inputLayout;
		D3D11_PRIMITIVE_TOPOLOGY topology;
		std::array<ID3D11Buffer*, 4> vertexBuffers;
		std::array<UINT, 4> vertexBufferStrides;
		std::array<UINT, 4> vertexBufferOffsets;
	};
}

void DX11State::save(VideoAPI& video)
{
	if (!state) {
		state = std::make_unique<DX11StateInternal>();
	}
	auto& dc = dynamic_cast<DX11Video&>(video).getDeviceContext();
	auto& s = dynamic_cast<DX11StateInternal&>(*state);

	dc.IAGetIndexBuffer(&s.indexBuffer, &s.indexBufferFormat, &s.indexBufferOffset);
	dc.IAGetInputLayout(&s.inputLayout);
	dc.IAGetPrimitiveTopology(&s.topology);
	dc.IAGetVertexBuffers(0, 4, s.vertexBuffers.data(), s.vertexBufferStrides.data(), s.vertexBufferOffsets.data());
}

void DX11State::load(VideoAPI& video)
{
	if (!state) {
		return;
	}
	auto& dc = dynamic_cast<DX11Video&>(video).getDeviceContext();
	auto& s = dynamic_cast<DX11StateInternal&>(*state);

	dc.IASetIndexBuffer(s.indexBuffer, s.indexBufferFormat, s.indexBufferOffset);
	dc.IASetInputLayout(s.inputLayout);
	dc.IASetPrimitiveTopology(s.topology);
	dc.IASetVertexBuffers(0, 4, s.vertexBuffers.data(), s.vertexBufferStrides.data(), s.vertexBufferOffsets.data());
}
