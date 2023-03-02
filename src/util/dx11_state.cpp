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
		std::array<ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> vsShaderResourceViews;

		ID3D11RasterizerState* rasterizerState;
		UINT numViewports;
		std::array<D3D11_VIEWPORT, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> viewports;
		UINT numRects;
		std::array<D3D11_RECT, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE> rects;

		std::array<ID3D11Buffer*, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> psConstantBuffers;
		ID3D11PixelShader* pixelShader;
		std::array<ID3D11SamplerState*, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT> psSamplers;
		std::array<ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> psShaderResourceViews;

		std::array<ID3D11RenderTargetView*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> renderTargetViews;
		ID3D11DepthStencilView* depthStencilViews;
		ID3D11DepthStencilState* depthStencilState;
		UINT stencilRef;
		ID3D11BlendState* blendState;
		std::array<FLOAT, 4> blendFactor;
		UINT sampleMask;

		ID3DDeviceContextState* contextState;
		ID3DDeviceContextState* prevState;
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

		/*
		auto& device = dynamic_cast<DX11Video&>(video).getDevice();
		ID3D11Device1* device1;
		device.QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&device1));
		std::array<D3D_FEATURE_LEVEL, 2> featureLevels = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };

		auto& s = dynamic_cast<DX11StateInternal&>(*state);
		auto result = device1->CreateDeviceContextState(0, featureLevels.data(), static_cast<UINT>(featureLevels.size()), D3D11_SDK_VERSION, __uuidof(ID3D11Device1), nullptr, &s.contextState);
		*/
	}
	auto& dc = dynamic_cast<DX11Video&>(video).getDeviceContext();
	auto& s = dynamic_cast<DX11StateInternal&>(*state);

	//dc.SwapDeviceContextState(s.contextState, &s.prevState);
	//return;

	dc.Flush();

	dc.IAGetIndexBuffer(&s.indexBuffer, &s.indexBufferFormat, &s.indexBufferOffset);
	dc.IAGetInputLayout(&s.inputLayout);
	dc.IAGetPrimitiveTopology(&s.topology);
	dc.IAGetVertexBuffers(0, 4, s.vertexBuffers.data(), s.vertexBufferStrides.data(), s.vertexBufferOffsets.data());

	dc.VSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, s.vsConstantBuffers.data());
	dc.VSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, s.vsSamplers.data());
	dc.VSGetShader(&s.vertexShader, nullptr, nullptr);
	dc.VSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, s.vsShaderResourceViews.data());

	dc.RSGetState(&s.rasterizerState);
	s.numViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
	dc.RSGetViewports(&s.numViewports, s.viewports.data());
	s.numRects = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
	dc.RSGetScissorRects(&s.numRects, s.rects.data());

	dc.PSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, s.psConstantBuffers.data());
	dc.PSGetShader(&s.pixelShader, nullptr, nullptr);
	dc.PSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, s.psSamplers.data());
	dc.PSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, s.psShaderResourceViews.data());

	dc.OMGetDepthStencilState(&s.depthStencilState, &s.stencilRef);
	dc.OMGetBlendState(&s.blendState, s.blendFactor.data(), &s.sampleMask);
	dc.OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, s.renderTargetViews.data(), &s.depthStencilViews);
	
	dc.ClearState();
}

void DX11State::load(VideoAPI& video)
{
	if (!state) {
		return;
	}
	auto& dc = dynamic_cast<DX11Video&>(video).getDeviceContext();
	auto& s = dynamic_cast<DX11StateInternal&>(*state);

	//dc.SwapDeviceContextState(s.prevState, &s.contextState);
	//return;

	dc.Flush();
	dc.ClearState();

	dc.IASetIndexBuffer(s.indexBuffer, s.indexBufferFormat, s.indexBufferOffset);
	dc.IASetInputLayout(s.inputLayout);
	dc.IASetPrimitiveTopology(s.topology);
	dc.IASetVertexBuffers(0, countValid(s.vertexBuffers), s.vertexBuffers.data(), s.vertexBufferStrides.data(), s.vertexBufferOffsets.data());

	dc.VSSetConstantBuffers(0, countValid(s.vsConstantBuffers), s.vsConstantBuffers.data());
	dc.VSSetSamplers(0, countValid(s.vsSamplers), s.vsSamplers.data());
	dc.VSSetShader(s.vertexShader, nullptr, 0);
	dc.VSSetShaderResources(0, countValid(s.vsShaderResourceViews), s.vsShaderResourceViews.data());

	dc.RSSetState(s.rasterizerState);
	dc.RSSetViewports(s.numViewports, s.viewports.data());
	dc.RSSetScissorRects(s.numRects, s.rects.data());

	dc.PSSetConstantBuffers(0, countValid(s.psConstantBuffers), s.psConstantBuffers.data());
	dc.PSSetShader(s.pixelShader, nullptr, 0);
	dc.PSSetSamplers(0, countValid(s.psSamplers), s.psSamplers.data());
	dc.PSSetShaderResources(0, countValid(s.psShaderResourceViews), s.psShaderResourceViews.data());

	dc.OMSetDepthStencilState(s.depthStencilState, s.stencilRef);
	dc.OMSetBlendState(s.blendState, s.blendFactor.data(), s.sampleMask);
	dc.OMSetRenderTargets(countValid(s.renderTargetViews), s.renderTargetViews.data(), s.depthStencilViews);
}
