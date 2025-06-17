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
		TypedD3D::Wrapper<ID3D12Device5> device = TypedD3D12::CreateDevice<ID3D12Device5>(D3D_FEATURE_LEVEL_12_2);

		debugDevice = TypedD3D::Cast<ID3D12DebugDevice2>(device.AsComPtr());
		TypedD3D::Wrapper<IDXGIFactory2> factory = TypedDXGI::CreateFactory2<IDXGIFactory2>({});
		xk::D3D12::CommandQueue<D3D12_COMMAND_LIST_TYPE_DIRECT> commandQueue{ device };
		xk::D3D12::SwapChain swapChain{ factory, device, commandQueue, window->GetInternalHandle(), 2, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH };
		TypedD3D12::Direct<ID3D12GraphicsCommandList5> commandList = TypedD3D::Cast<ID3D12GraphicsCommandList5>(device->CreateCommandList1<D3D12_COMMAND_LIST_TYPE_DIRECT>(0, D3D12_COMMAND_LIST_FLAG_NONE));
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
			.DepthStencilState{ .DepthEnable = false },
			.InputLayout = { elements.data(), elements.size() },
			.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			.NumRenderTargets = 1,
			.RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB },
			.SampleDesc = { .Count = 1 }
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
				
			D3D12_HEAP_PROPERTIES vertexHeap
			{
				.Type = D3D12_HEAP_TYPE_DEFAULT,
				.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
				.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
				.CreationNodeMask = 0,
				.VisibleNodeMask = 0
			};

			D3D12_RESOURCE_DESC vertexDesc
			{
				.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
				.Alignment = 0,
				.Width = static_cast<UINT>(vertices.size() * sizeof(Vertex)),
				.Height = 1,
				.DepthOrArraySize = 1,
				.MipLevels = 1,
				.Format = DXGI_FORMAT_UNKNOWN,
				.SampleDesc = {.Count = 1, . Quality = 0},
				.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
				.Flags = D3D12_RESOURCE_FLAG_NONE
			};
				
			TypedD3D::Wrapper<ID3D12Resource> vertexResource = device->CreateCommittedResource(
				vertexHeap,
				D3D12_HEAP_FLAG_NONE,
				vertexDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr);

			TypedD3D::Wrapper<ID3D12Resource> vertexUpload = xk::D3D12::StageUploadBuffer(std::span{ vertices }, TypedD3D::Wrapper<ID3D12Device>{ device });

			commandAllocators[0]->Reset();
			commandList->Reset(commandAllocators[0], nullptr);
			commandList->CopyBufferRegion(*vertexResource.Get(), 0, *vertexUpload.Get(), 0, sizeof(Vertex)* vertices.size());
			D3D12_BARRIER_GROUP group;
			group.NumBarriers = 1;
			group.Type = D3D12_BARRIER_TYPE_BUFFER;
			D3D12_BUFFER_BARRIER barrier{};
			barrier.SyncBefore = D3D12_BARRIER_SYNC_NONE;
			barrier.SyncAfter = D3D12_BARRIER_SYNC_VERTEX_SHADING;
			barrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
			barrier.AccessAfter = D3D12_BARRIER_ACCESS_VERTEX_BUFFER;
			barrier.pResource = vertexResource.Get();
			barrier.Offset = 0;
			barrier.Size = vertexDesc.Width;
			group.pBufferBarriers = &barrier;

			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> tempList = TypedD3D::Cast<ID3D12GraphicsCommandList7>(commandList.Get());
			tempList.Get()->Release();
			tempList->Barrier(1, &group);
			
			commandList->Close();

			TypedD3D::Array<TypedD3D12::Direct<ID3D12CommandList>, 1> submitList{ commandList };
			swapChain.GetQueue().ExecuteCommandLists(TypedD3D::Span{ submitList });
			swapChain.GetQueue().GPUSignal();
			swapChain.GetQueue().Flush();
			//UpdateSubresources(commandList.Get(), vertexResource.Get(), vertexUpload.Get(), 0, 0, 1, &vertexData);

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
					Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> tempList = TypedD3D::Cast<ID3D12GraphicsCommandList7>(commandList.Get());
					tempList.Get()->Release();
					commandAllocators[currentFrameIndex]->Reset();
					commandList->Reset(commandAllocators[currentFrameIndex], nullptr);

					D3D12_BARRIER_GROUP group;
					group.NumBarriers = 1;
					group.Type = D3D12_BARRIER_TYPE_TEXTURE;
					D3D12_TEXTURE_BARRIER barrier{};
					barrier.SyncBefore = D3D12_BARRIER_SYNC_NONE;
					barrier.SyncAfter = D3D12_BARRIER_SYNC_RENDER_TARGET;
					barrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
					barrier.AccessAfter = D3D12_BARRIER_ACCESS_RENDER_TARGET;
					barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_COMMON;
					barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
					barrier.pResource = swapChain.GetBackBuffer().Get();
					group.pTextureBarriers = &barrier;
					tempList->Barrier(1, &group);

					commandList->ClearRenderTargetView(backBuffer, std::array<const float, 4>{ 0.2f, 0.3f, 0.3f, 1.0f });
					commandList->OMSetRenderTargets(std::span(&backBuffer, 1), true, nullptr);
					commandList->SetPipelineState(pipeline.Get());
					commandList->SetGraphicsRootSignature(nullSignature.Get());
					commandList->RSSetViewports(std::span(&viewport, 1));
					commandList->RSSetScissorRects(std::span(&rect, 1));
					commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					commandList->IASetVertexBuffers(0, std::span(&vertexBufferView, 1));
					commandList->DrawInstanced(3, 1, 0, 0);

					barrier.SyncBefore = D3D12_BARRIER_SYNC_RENDER_TARGET;
					barrier.SyncAfter = D3D12_BARRIER_SYNC_NONE;
					barrier.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET;
					barrier.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
					barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
					barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_PRESENT;
					barrier.pResource = swapChain.GetBackBuffer().Get();
					group.pTextureBarriers = &barrier;
					tempList->Barrier(1, &group);


					commandList->Close();
					TypedD3D::Array<TypedD3D12::Direct<ID3D12CommandList>, 1> submitList{ commandList };
					queue.ExecuteCommandLists(TypedD3D::Span{ submitList });

				});
				//				//deviceContext->ClearRenderTargetView(backBuffer, { 0.2f, 0.3f, 0.3f, 1.0f });
				//				//deviceContext->OMSetRenderTargets(backBuffer, nullptr);
				//				//deviceContext->RSSetViewports({ .Width = 1280, .Height = 720, .MinDepth = 0, .MaxDepth = 1 });
				//				//deviceContext->IASetInputLayout(layout);
				//				//deviceContext->IASetVertexBuffers(0, vertexBuffer, sizeof(Vertex), 0);
				//				//deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				//				//deviceContext->VSSetShader(vertexShader, nullptr);
				//				//deviceContext->PSSetShader(pixelShader, nullptr);
				//				//deviceContext->Draw(vertexCount, 0);
				//				//swapChain->Present(0, 0);
			}
		}
	}

	debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_FLAGS::D3D12_RLDO_SUMMARY | D3D12_RLDO_FLAGS::D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
	return 0;
}