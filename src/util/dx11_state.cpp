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

		std::array<ID3D11Buffer*, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> vsConstantBuffers;
		ID3D11VertexShader* vertexShader;
		std::array<ID3D11SamplerState*, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT> vsSamplers;

		ID3D11RasterizerState* rasterizerState;

		std::array<ID3D11Buffer*, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> psConstantBuffers;
		ID3D11PixelShader* pixelShader;
		std::array<ID3D11SamplerState*, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT> psSamplers;

		std::array<ID3D11RenderTargetView*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> renderTargetViews;
		ID3D11DepthStencilView* depthStencilViews;
		ID3D11DepthStencilState* depthStencilState;
		UINT stencilRef;
		ID3D11BlendState* blendState;
		std::array<FLOAT, 4> blendFactor;
		UINT sampleMask;
	};
}

template <typename T>
UINT countValid(const T& vs)
{
	const auto n = vs.size();
	for (size_t i = 0; i < n; ++i) {
		if (!vs[i]) {
			return static_cast<UINT>(i);
		}
	}
	return static_cast<UINT>(n);
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

	dc.VSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, s.vsConstantBuffers.data());
	dc.VSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, s.vsSamplers.data());
	dc.VSGetShader(&s.vertexShader, nullptr, nullptr);

	dc.RSGetState(&s.rasterizerState);

	dc.PSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, s.psConstantBuffers.data());
	dc.PSGetShader(&s.pixelShader, nullptr, nullptr);
	dc.PSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, s.psSamplers.data());

	dc.OMGetDepthStencilState(&s.depthStencilState, &s.stencilRef);
	dc.OMGetBlendState(&s.blendState, s.blendFactor.data(), &s.sampleMask);
	dc.OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, s.renderTargetViews.data(), &s.depthStencilViews);
}

void DX11State::load(VideoAPI& video)
{
	if (!state) {
		return;
	}
	auto& dc = dynamic_cast<DX11Video&>(video).getDeviceContext();
	auto& s = dynamic_cast<DX11StateInternal&>(*state);

	dc.ClearState();

	dc.IASetIndexBuffer(s.indexBuffer, s.indexBufferFormat, s.indexBufferOffset);
	dc.IASetInputLayout(s.inputLayout);
	dc.IASetPrimitiveTopology(s.topology);
	dc.IASetVertexBuffers(0, countValid(s.vertexBuffers), s.vertexBuffers.data(), s.vertexBufferStrides.data(), s.vertexBufferOffsets.data());

	dc.VSSetConstantBuffers(0, countValid(s.vsConstantBuffers), s.vsConstantBuffers.data());
	dc.VSSetSamplers(0, countValid(s.vsSamplers), s.vsSamplers.data());
	dc.VSSetShader(s.vertexShader, nullptr, 0);

	dc.RSSetState(s.rasterizerState);

	dc.PSSetConstantBuffers(0, countValid(s.psConstantBuffers), s.psConstantBuffers.data());
	dc.PSSetShader(s.pixelShader, nullptr, 0);
	dc.PSSetSamplers(0, countValid(s.psSamplers), s.psSamplers.data());

	dc.OMSetDepthStencilState(s.depthStencilState, s.stencilRef);
	dc.OMSetBlendState(s.blendState, s.blendFactor.data(), s.sampleMask);
	dc.OMSetRenderTargets(countValid(s.renderTargetViews), s.renderTargetViews.data(), s.depthStencilViews);
}
