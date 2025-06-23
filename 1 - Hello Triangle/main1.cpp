#include <dxgi1_4.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <d3d12.h>
#include <chrono>
#include <span>
#include <concepts>
#include <cassert>
#include <SDL2/SDL.h>

import TypedD3D12;
import TypedDXGI;
import D3D12Extensions;
import SDL2pp;
//using namespace TypedD3D;
#undef CreateWindow;
int main()
{
	SDL2pp::unique_ptr<SDL2pp::Window> window = SDL2pp::CreateWindow("1 - Hello Triangle", { 1280, 720 }, {});

	Microsoft::WRL::ComPtr<ID3D12Debug6> debugLayer;
	D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	debugLayer->EnableDebugLayer();
	Microsoft::WRL::ComPtr<ID3D12DebugDevice2> debugDevice;
	//
	{
		TypedD3D::D3D12::Direct<ID3D12CommandQueue> queue;
		TypedD3D::Wrapper<ID3D12Device10> device = TypedD3D12::CreateDevice<ID3D12Device10>(D3D_FEATURE_LEVEL_12_2);

		debugDevice = TypedD3D::Cast<ID3D12DebugDevice2>(device.AsComPtr());
		TypedD3D::Wrapper<IDXGIFactory2> factory = TypedDXGI::CreateFactory2<IDXGIFactory2>({});
		xk::D3D12::CommandQueue<D3D12_COMMAND_LIST_TYPE_DIRECT> commandQueue{ device };
		xk::D3D12::SwapChain swapChain{ factory, device, commandQueue, window->GetInternalHandle(), 2, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH };
		TypedD3D12::Direct<ID3D12GraphicsCommandList7> commandList = device->CreateCommandList1<D3D12_COMMAND_LIST_TYPE_DIRECT, ID3D12GraphicsCommandList7>(0, D3D12_COMMAND_LIST_FLAG_NONE);
		TypedD3D::Array<TypedD3D12::Direct<ID3D12CommandAllocator>, 2> commandAllocators
		{
			device->CreateCommandAllocator<D3D12_COMMAND_LIST_TYPE_DIRECT>(),
			device->CreateCommandAllocator<D3D12_COMMAND_LIST_TYPE_DIRECT>(),
		};


		std::array<D3D12_INPUT_ELEMENT_DESC, 2> elements
		{
			D3D12_INPUT_ELEMENT_DESC{
				.SemanticName = "POSITION",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
			D3D12_INPUT_ELEMENT_DESC{
				.SemanticName = "COLOR",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.InputSlot = 0,
				.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
		};

		Microsoft::WRL::ComPtr<ID3DBlob> vertexBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> pixelBlob;

		TypedD3D::ThrowIfFailed(D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", D3DCOMPILE_DEBUG, {}, &vertexBlob, nullptr));
		TypedD3D::ThrowIfFailed(D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", D3DCOMPILE_DEBUG, {}, &pixelBlob, nullptr));

		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc
		{
			.NumParameters = 0,
			.pParameters = nullptr,
			.NumStaticSamplers = 0,
			.pStaticSamplers = nullptr,
			.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		};
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, nullptr);
		Microsoft::WRL::ComPtr<ID3D12RootSignature> nullSignature = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize());


		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc
		{
			.pRootSignature = nullSignature.Get(),
			.VS = { vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize() },
			.PS = { pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize() },
			.BlendState
			{
				.RenderTarget =
				{
					{
						.SrcBlend = D3D12_BLEND_ONE,
						.DestBlend = D3D12_BLEND_ZERO,
						.BlendOp = D3D12_BLEND_OP_ADD,
						.SrcBlendAlpha = D3D12_BLEND_ONE,
						.DestBlendAlpha = D3D12_BLEND_ZERO,
						.BlendOpAlpha = D3D12_BLEND_OP_ADD,
						.LogicOp = D3D12_LOGIC_OP_NOOP,
						.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
					}
				}
			},
			.SampleMask = 0xff'ff'ff'ff,
			.RasterizerState =
			{
				.FillMode = D3D12_FILL_MODE_SOLID,
				.CullMode = D3D12_CULL_MODE_BACK,
				.FrontCounterClockwise = false
			},
			.DepthStencilState{.DepthEnable = false },
			.InputLayout = { elements.data(), elements.size() },
			.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			.NumRenderTargets = 1,
			.RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB },
			.SampleDesc = {.Count = 1 }
		};

		TypedD3D12::Graphics<ID3D12PipelineState> pipeline = device->CreateGraphicsPipelineState(pipelineDesc);


		struct Vertex
		{
			xk::Math::Vector<float, 3> position;
			xk::Math::Vector<unsigned char, 4> color;
		};

		auto [vertexBuffer, vertexCount] = [&]
		{
			auto vertices = std::to_array<Vertex>(
				{
					{.position = { -0.5f, -0.5f, 0 }, .color = { 255, 0, 0, 255}},
					{.position = { 0, 0.5f, 0 }, .color = {0, 0, 255, 255}},
					{.position = { 0.5f, -0.5f, 0 }, .color = {0, 255, 0, 255}},
				});

			xk::D3D12::StagedBufferUpload vertexUpload = xk::D3D12::StageBufferUpload(std::span{ vertices }, device);

			TypedD3D::Wrapper<ID3D12Resource> vertexResource = xk::D3D12::CreateBuffer(device, vertexUpload);
			xk::D3D12::Record(commandList, commandAllocators[0].ToWrapper(), nullptr,
				[&](TypedD3D::IUnknownWrapper<ID3D12GraphicsCommandList7, TypedD3D12::DirectTraits> commandList)
			{

				xk::D3D12::UploadBuffer(commandList, vertexResource, vertexUpload);
				D3D12_BARRIER_GROUP group;
				group.NumBarriers = 1;
				group.Type = D3D12_BARRIER_TYPE_BUFFER;
				D3D12_BUFFER_BARRIER barrier = xk::D3D12::BufferBarrier(vertexResource.Get())
					.Before(xk::D3D12::NoAccessBarrier())
					.After(xk::D3D12::VertexBufferBarrier());
				group.pBufferBarriers = &barrier;

				commandList->Barrier({ &group, 1 });
			});

			TypedD3D::Array<TypedD3D12::Direct<ID3D12CommandList>, 1> submitList{ commandList };
			swapChain.GetQueue().ExecuteCommandLists(submitList);
			swapChain.GetQueue().GPUSignal();
			swapChain.GetQueue().Flush();

			return std::pair{ vertexResource, vertices.size() };
		}();

		D3D12_VIEWPORT viewport
		{
			.TopLeftX = 0,
			.TopLeftY = 0,
			.Width = static_cast<float>(window->GetSize().X()),
			.Height = static_cast<float>(window->GetSize().Y()),
			.MinDepth = 0,
			.MaxDepth = 1
		};

		D3D12_RECT rect
		{
			.left = 0,
			.top = 0,
			.right = static_cast<LONG>(window->GetSize().X()),
			.bottom = static_cast<LONG>(window->GetSize().Y())
		};
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView
		{
			.BufferLocation = vertexBuffer->GetGPUVirtualAddress(),
			.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * vertexCount),
			.StrideInBytes = static_cast<UINT>(sizeof(Vertex))
		};
		SDL2pp::Event event;
		while (true)
		{
			if (SDL2pp::PollEvent(event))
			{
				if (event.type == SDL2pp::EventType::SDL_QUIT)
				{
					break;
				}
			}
			else
			{
				swapChain.Frame([&](xk::D3D12::CommandQueue<D3D12_COMMAND_LIST_TYPE_DIRECT>& queue, UINT currentFrameIndex, TypedD3D12::RTV<D3D12_CPU_DESCRIPTOR_HANDLE> backBuffer)
				{
					xk::D3D12::Record(commandList, commandAllocators[currentFrameIndex].ToWrapper(), pipeline.Get()
						, [&](TypedD3D::IUnknownWrapper<ID3D12GraphicsCommandList7, TypedD3D12::DirectTraits> commandList)
					{
						D3D12_BARRIER_GROUP group;
						group.NumBarriers = 1;
						group.Type = D3D12_BARRIER_TYPE_TEXTURE;

						D3D12_TEXTURE_BARRIER barrier = xk::D3D12::TextureBarrier{ swapChain.GetBackBuffer().Get() }
							.Before(xk::D3D12::NoAccessTextureBarrier(D3D12_BARRIER_LAYOUT_COMMON))
							.After(xk::D3D12::RenderTargetTextureBarrier());
						group.pTextureBarriers = &barrier;
						commandList->Barrier({ &group, 1 });

						commandList->ClearRenderTargetView(backBuffer, std::array<const float, 4>{ 0.2f, 0.3f, 0.3f, 1.0f });
						commandList->OMSetRenderTargets(std::span(&backBuffer, 1), true, nullptr);
						commandList->SetGraphicsRootSignature(nullSignature.Get());
						commandList->RSSetViewports(std::span(&viewport, 1));
						commandList->RSSetScissorRects(std::span(&rect, 1));
						commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
						commandList->IASetVertexBuffers(0, std::span(&vertexBufferView, 1));
						commandList->DrawInstanced(3, 1, 0, 0);

						barrier = xk::D3D12::TextureBarrier{ swapChain.GetBackBuffer().Get() }
							.Before(xk::D3D12::RenderTargetTextureBarrier())
							.After(xk::D3D12::NoAccessTextureBarrier(D3D12_BARRIER_LAYOUT_COMMON));
						group.pTextureBarriers = &barrier;
						commandList->Barrier({ &group, 1 });
					});

					TypedD3D::Array<TypedD3D12::Direct<ID3D12CommandList>, 1> submitList{ commandList };
					queue.ExecuteCommandLists(submitList);
				});
			}
		}
	}

	debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_FLAGS::D3D12_RLDO_SUMMARY | D3D12_RLDO_FLAGS::D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
	return 0;
}