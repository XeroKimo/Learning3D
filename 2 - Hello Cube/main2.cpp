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
#include <iostream>

import TypedD3D12;
import TypedDXGI;
import D3D12Extensions;
import SDL2pp;
import xk.Math;

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
		TypedD3D12::DSV<ID3D12DescriptorHeap> DSVs = device->CreateDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE>(1, 0);
		TypedD3D::Wrapper<ID3D12Resource> depthBuffer = [&]
		{
			D3D12_HEAP_PROPERTIES heapProperties
			{
				.Type = D3D12_HEAP_TYPE_DEFAULT,
				.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
				.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
				.CreationNodeMask = 0,
				.VisibleNodeMask = 0
			};

			D3D12_RESOURCE_DESC description
			{
				.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				.Alignment = 0,
				.Width = static_cast<UINT64>(window->GetSize().X()),
				.Height = static_cast<UINT64>(window->GetSize().Y()),
				.DepthOrArraySize = 1,
				.MipLevels = 1,
				.Format = DXGI_FORMAT_D32_FLOAT,
				.SampleDesc = {.Count = 1, . Quality = 0},
				.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE,
				.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
			};

			return device->CreateCommittedResource(heapProperties, D3D12_HEAP_FLAG_NONE, description, D3D12_RESOURCE_STATE_DEPTH_WRITE, nullptr);
		}();

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
				.SemanticName = "NORMAL",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32B32_FLOAT,
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
		std::array<D3D12_ROOT_PARAMETER1, 2> parameter;
		parameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		parameter[0].Descriptor.RegisterSpace = 0;
		parameter[0].Descriptor.ShaderRegister = 0;
		parameter[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
		parameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		parameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		parameter[1].Descriptor.RegisterSpace = 0;
		parameter[1].Descriptor.ShaderRegister = 1;
		parameter[1].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
		parameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc
		{
			.Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
			.Desc_1_1
			{
			.NumParameters = parameter.size(),
			.pParameters = parameter.data(),
			.NumStaticSamplers = 0,
			.pStaticSamplers = nullptr,
			.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			}
		};
		D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signatureBlob, nullptr);
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
			.DepthStencilState
			{
				.DepthEnable = true,
				.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
				.DepthFunc = D3D12_COMPARISON_FUNC_GREATER,
				.StencilEnable = false
			},
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
			xk::Math::Vector<float, 3> normal;
		};

		auto [vertexBuffer, vertexCount] = [&]
		{
			static constexpr xk::Math::Vector<float, 3> blf{ -0.5f, -0.5f, -0.5f };
			static constexpr xk::Math::Vector<float, 3> tlf{ -0.5f, 0.5f, -0.5f };
			static constexpr xk::Math::Vector<float, 3> trf{ 0.5f, 0.5f, -0.5f };
			static constexpr xk::Math::Vector<float, 3> brf{ 0.5f, -0.5f, -0.5f };

			static constexpr xk::Math::Vector<float, 3> blb{ -0.5f, -0.5f, 0.5f };
			static constexpr xk::Math::Vector<float, 3> tlb{ -0.5f, 0.5f, 0.5f };
			static constexpr xk::Math::Vector<float, 3> trb{ 0.5f, 0.5f, 0.5f };
			static constexpr xk::Math::Vector<float, 3> brb{ 0.5f, -0.5f, 0.5f };

			static constexpr xk::Math::Vector<float, 3> right{ 1, 0, 0 };
			static constexpr xk::Math::Vector<float, 3> up{ 0, 1, 0 };
			static constexpr xk::Math::Vector<float, 3> forward{ 0, 0, 1 };

			auto vertices = std::to_array<Vertex>(
				{
					{ blf, -forward },
					{ tlf, -forward },
					{ trf, -forward },
					{ brf, -forward },
					{ blb, forward },
					{ brb, forward },
					{ trb, forward },
					{ tlb, forward },
					{ blf, -up },
					{ brf, -up },
					{ brb, -up },
					{ blb, -up },
					{ tlf, up },
					{ tlb, up },
					{ trb, up },
					{ trf, up },
					{ blf, -right },
					{ blb, -right },
					{ tlb, -right },
					{ tlf, -right },
					{ brf, right },
					{ trf, right },
					{ trb, right },
					{ brb, right },
				});

			xk::D3D12::StagedBufferUpload vertexUpload = xk::D3D12::StageBufferUpload(std::span{ vertices }, TypedD3D::Wrapper<ID3D12Device>{ device });

			TypedD3D::Wrapper<ID3D12Resource> vertexResource = xk::D3D12::CreateBuffer(device, vertexUpload);
			xk::D3D12::Record(commandList, commandAllocators[0].ToWrapper(), nullptr,
				[&](TypedD3D::IUnknownWrapper<ID3D12GraphicsCommandList5, TypedD3D12::DirectTraits> commandList)
			{

				xk::D3D12::UploadBuffer(commandList, vertexResource, vertexUpload);
				D3D12_BARRIER_GROUP group;
				group.NumBarriers = 1;
				group.Type = D3D12_BARRIER_TYPE_BUFFER;
				D3D12_BUFFER_BARRIER barrier = xk::D3D12::BufferBarrier(vertexResource.Get())
					.Before(xk::D3D12::NoAccessBarrier())
					.After(xk::D3D12::VertexBufferBarrier());
				group.pBufferBarriers = &barrier;

				Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> tempList = TypedD3D::Cast<ID3D12GraphicsCommandList7>(commandList.Get());
				tempList.Get()->Release();
				tempList->Barrier(1, &group);
			});

			TypedD3D::Array<TypedD3D12::Direct<ID3D12CommandList>, 1> submitList{ commandList };
			swapChain.GetQueue().ExecuteCommandLists(submitList);
			swapChain.GetQueue().GPUSignal();
			swapChain.GetQueue().Flush();

			return std::pair{ vertexResource, vertices.size() };
		}();

		auto [indexBuffer, indexCount] = [&]
		{
			auto indices = std::to_array<UINT32>(
				{
					0, 1, 2, 0, 2, 3, //Back face
					4, 5, 6, 4, 6, 7, //Front Face
					8, 9, 10, 8, 10, 11, //Down face
					12, 13, 14, 12, 14, 15, //Up Face
					16, 17, 18, 16, 18, 19, //Left face
					20, 21, 22, 20, 22, 23, //Right Face
				});

			xk::D3D12::StagedBufferUpload indexUpload = xk::D3D12::StageBufferUpload(std::span{ indices }, TypedD3D::Wrapper<ID3D12Device>{ device });			
			
			TypedD3D::Wrapper<ID3D12Resource> indexResource = xk::D3D12::CreateBuffer(device, indexUpload);
			xk::D3D12::Record(commandList, commandAllocators[0].ToWrapper(), nullptr,
				[&](TypedD3D::IUnknownWrapper<ID3D12GraphicsCommandList5, TypedD3D12::DirectTraits> commandList)
			{

				xk::D3D12::UploadBuffer(commandList, indexResource, indexUpload);
				D3D12_BARRIER_GROUP group;
				group.NumBarriers = 1;
				group.Type = D3D12_BARRIER_TYPE_BUFFER;
				D3D12_BUFFER_BARRIER barrier = xk::D3D12::BufferBarrier(indexResource.Get())
					.Before(xk::D3D12::NoAccessBarrier())
					.After(xk::D3D12::IndexBufferBarrier());
				group.pBufferBarriers = &barrier;

				Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> tempList = TypedD3D::Cast<ID3D12GraphicsCommandList7>(commandList.Get());
				tempList.Get()->Release();
				tempList->Barrier(1, &group);
			});

			TypedD3D::Array<TypedD3D12::Direct<ID3D12CommandList>, 1> submitList{ commandList };
			swapChain.GetQueue().ExecuteCommandLists(submitList);
			swapChain.GetQueue().GPUSignal();
			swapChain.GetQueue().Flush();

			return std::pair{ indexResource, indices.size() };
		}();

		xk::D3D12::ConstantBuffer constantBuffer{ device, 1 << 16 };


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
		D3D12_INDEX_BUFFER_VIEW indexBufferView
		{
			.BufferLocation = indexBuffer->GetGPUVirtualAddress(),
			.SizeInBytes = static_cast<UINT>(sizeof(UINT32) * indexCount),
			.Format = DXGI_FORMAT_R32_UINT
		};


		xk::Math::Vector<float, 3> cameraPos{ 0, 0, -3 };
		xk::Math::Vector<float, 3> cameraMovementDirection{ 0, 0, 0 };
		float cameraSpeed = 4;
		xk::Math::SquareMatrix<float, 4> projectionMatrix = xk::Math::PerspectiveProjectionLH(xk::Math::Degree{ 90.f }, 1280.f / 720.f, 0.001f, 1000.f);


		xk::Math::Quaternion<float> objectRotation;

		SDL2pp::Event event;
		std::chrono::time_point previous = std::chrono::steady_clock::now();
		while (true)
		{
			if (SDL2pp::PollEvent(event))
			{
				if (event.type == SDL2pp::EventType::SDL_QUIT)
				{
					break;
				}
				if(event.type == SDL2pp::EventType::SDL_KEYDOWN)
				{
					if(event.key.keysym.sym == SDLK_w)
					{
						cameraMovementDirection.Z() = 1;
					}
					if(event.key.keysym.sym == SDLK_s)
					{
						cameraMovementDirection.Z() = -1;
					}
					if(event.key.keysym.sym == SDLK_a)
					{
						cameraMovementDirection.X() = -1;
					}
					if(event.key.keysym.sym == SDLK_d)
					{
						cameraMovementDirection.X() = 1;
					}
				}
				if(event.type == SDL2pp::EventType::SDL_KEYUP)
				{
					if(event.key.keysym.sym == SDLK_w)
					{
						cameraMovementDirection.Z() = 0;
					}
					if(event.key.keysym.sym == SDLK_s)
					{
						cameraMovementDirection.Z() = 0;
					}
					if(event.key.keysym.sym == SDLK_a)
					{
						cameraMovementDirection.X() = 0;
					}
					if(event.key.keysym.sym == SDLK_d)
					{
						cameraMovementDirection.X() = 0;
					}
				}
			}
			else
			{
				auto current = std::chrono::steady_clock::now();
				auto delta = current - previous;
				cameraPos += cameraMovementDirection * cameraSpeed * std::chrono::duration<float>(delta).count();

				objectRotation *= xk::Math::Quaternion<float>{ xk::Math::Normalize<float, 3>({ 1, 1, 1 }), xk::Math::Degree<float>{ 90.f * std::chrono::duration<float>{ delta }.count()} };

				swapChain.Frame([&](xk::D3D12::CommandQueue<D3D12_COMMAND_LIST_TYPE_DIRECT>& queue, UINT currentFrameIndex, TypedD3D12::RTV<D3D12_CPU_DESCRIPTOR_HANDLE> backBuffer)
				{
					Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> tempList = TypedD3D::Cast<ID3D12GraphicsCommandList7>(commandList.Get());
					tempList.Get()->Release();

					xk::D3D12::Record(commandList, commandAllocators[currentFrameIndex].ToWrapper(), pipeline.Get()
						, [&](TypedD3D::IUnknownWrapper<ID3D12GraphicsCommandList5, TypedD3D12::DirectTraits> commandList)
					{
						D3D12_BARRIER_GROUP group;
						group.NumBarriers = 1;
						group.Type = D3D12_BARRIER_TYPE_TEXTURE;

						D3D12_TEXTURE_BARRIER barrier = xk::D3D12::TextureBarrier{ swapChain.GetBackBuffer().Get() }
							.Before(xk::D3D12::NoAccessTextureBarrier(D3D12_BARRIER_LAYOUT_COMMON))
							.After(xk::D3D12::RenderTargetTextureBarrier());
						group.pTextureBarriers = &barrier;
						tempList->Barrier(1, &group);

						commandList->ClearRenderTargetView(backBuffer, std::array<const float, 4>{ 0.2f, 0.3f, 0.3f, 1.0f });
						commandList->OMSetRenderTargets(std::span(&backBuffer, 1), true, nullptr);
						commandList->SetGraphicsRootSignature(nullSignature.Get());
						commandList->RSSetViewports(std::span(&viewport, 1));
						commandList->RSSetScissorRects(std::span(&rect, 1));
						commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
						commandList->IASetVertexBuffers(0, std::span(&vertexBufferView, 1));
						commandList->IASetIndexBuffer(&indexBufferView);
						commandList->SetGraphicsRootConstantBufferView(0, constantBuffer.Upload(projectionMatrix * xk::Math::TransformMatrix(-cameraPos)));
						commandList->SetGraphicsRootConstantBufferView(1, constantBuffer.Upload(objectRotation.ToRotationMatrix()* xk::Math::SquareMatrix<float, 4>::Identity()));
						commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);

						barrier = xk::D3D12::TextureBarrier{ swapChain.GetBackBuffer().Get() }
							.Before(xk::D3D12::RenderTargetTextureBarrier())
							.After(xk::D3D12::NoAccessTextureBarrier(D3D12_BARRIER_LAYOUT_COMMON));
						group.pTextureBarriers = &barrier;
						tempList->Barrier(1, &group);
					});

					TypedD3D::Array<TypedD3D12::Direct<ID3D12CommandList>, 1> submitList{ commandList };
					queue.ExecuteCommandLists(TypedD3D::Span{ submitList });
				});

				previous = current;
			}
		}
	}

	debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_FLAGS::D3D12_RLDO_SUMMARY | D3D12_RLDO_FLAGS::D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
	return 0;
}