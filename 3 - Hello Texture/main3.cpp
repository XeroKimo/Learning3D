
#include <d3d11.h>
#include <dxgi1_4.h>
#include <d3d11sdklayers.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <chrono>
#include <SDL2/SDL_image.h>

import TypedD3D11;
import TypedDXGI;
import xk.Math;
import SDL2pp;

#undef main
#undef CreateWindow

int main()
{
	struct SDLImageLifetime
	{
		SDLImageLifetime()
		{
			IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
		}

		~SDLImageLifetime()
		{
			IMG_Quit();
		}
	} init;
	SDL2pp::unique_ptr<SDL2pp::Window> window = SDL2pp::CreateWindow("3 - Hello Texture", { 1280, 720 }, {});

	Microsoft::WRL::ComPtr<ID3D11Debug> debugDevice;

	{
		auto [device, deviceContext] = TypedD3D11::CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, D3D_FEATURE_LEVEL_11_1, D3D11_SDK_VERSION);
		debugDevice = TypedD3D::Cast<ID3D11Debug>(device.AsComPtr());
		TypedD3D::Wrapper<IDXGIFactory2> factory = TypedDXGI::CreateFactory2<IDXGIFactory2>({});
		TypedD3D::Wrapper<IDXGISwapChain> swapChain = factory->CreateSwapChainForHwnd(
			device,
			window->GetInternalHandle(),
			DXGI_SWAP_CHAIN_DESC1
			{
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.SampleDesc{.Count = 1 },
				.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT,
				.BufferCount = 2,
				.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD
			},
			nullptr,
			nullptr);
		D3D11_RENDER_TARGET_VIEW_DESC viewDesc
		{
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
			.Texture2D = {.MipSlice = 0}
		};

		TypedD3D::Wrapper<ID3D11RenderTargetView> backBuffer = device->CreateRenderTargetView(swapChain->GetBuffer<ID3D11Resource>(0), &viewDesc);

		auto elements = std::to_array<D3D11_INPUT_ELEMENT_DESC>(
		{
			D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "POSITION",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
			D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "NORMAL",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
			D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "TEXCOORD",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
		});
		Microsoft::WRL::ComPtr<ID3DBlob> blob;
		TypedD3D::ThrowIfFailed(D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", {}, {}, &blob, nullptr));
		TypedD3D::Wrapper<ID3D11InputLayout> layout = device->CreateInputLayout(elements, *blob.Get());
		TypedD3D::Wrapper<ID3D11VertexShader> vertexShader = device->CreateVertexShader(*blob.Get(), nullptr);

		TypedD3D::ThrowIfFailed(D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", {}, {}, &blob, nullptr));
		TypedD3D::Wrapper<ID3D11PixelShader> pixelShader = device->CreatePixelShader(*blob.Get(), nullptr);

		struct Vertex
		{
			xk::Math::Vector<float, 3> position;
			xk::Math::Vector<float, 3> normal;
			xk::Math::Vector<float, 2> uv;
		};

		TypedD3D::Wrapper<ID3D11Buffer> vertexBuffer = [&]
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

			static constexpr xk::Math::Vector<float, 2> bluv{ 0, 1 };
			static constexpr xk::Math::Vector<float, 2> bruv{ 1, 1 };
			static constexpr xk::Math::Vector<float, 2> tluv{ 0, 0 };
			static constexpr xk::Math::Vector<float, 2> truv{ 1, 0 };

			auto vertices = std::to_array<Vertex>(
				{
					{ blf, -forward, bluv },
					{ tlf, -forward, tluv },
					{ trf, -forward, truv },
					{ brf, -forward, bruv },
					{ blb, forward, bluv },
					{ brb, forward, tluv },
					{ trb, forward, truv },
					{ tlb, forward, bruv },
					{ blf, -up, bluv },
					{ brf, -up, tluv },
					{ brb, -up, truv, },
					{ blb, -up, bruv },
					{ tlf, up, bluv },
					{ tlb, up, tluv },
					{ trb, up, truv },
					{ trf, up, bruv },
					{ blf, -right, bluv },
					{ blb, -right, tluv },
					{ tlb, -right, truv },
					{ tlf, -right, bruv },
					{ brf, right, bluv },
					{ trf, right, tluv },
					{ trb, right, truv},
					{ brb, right, bruv },
				});

			D3D11_BUFFER_DESC bufferDesc
			{
				.ByteWidth = sizeof(vertices),
				.Usage = D3D11_USAGE_IMMUTABLE,
				.BindFlags = D3D11_BIND_VERTEX_BUFFER,
				.CPUAccessFlags = 0,
				.MiscFlags = 0,
				.StructureByteStride = 0
			};

			D3D11_SUBRESOURCE_DATA initialData
			{
				.pSysMem = &vertices,
				.SysMemPitch = sizeof(vertices),
				.SysMemSlicePitch = 0
			};

			return device->CreateBuffer(bufferDesc, &initialData);
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

			D3D11_BUFFER_DESC bufferDesc
			{
				.ByteWidth = sizeof(indices),
				.Usage = D3D11_USAGE_IMMUTABLE,
				.BindFlags = D3D11_BIND_INDEX_BUFFER,
				.CPUAccessFlags = 0,
				.MiscFlags = 0,
				.StructureByteStride = 0
			};

			D3D11_SUBRESOURCE_DATA initialData
			{
				.pSysMem = &indices,
				.SysMemPitch = sizeof(UINT32),
				.SysMemSlicePitch = 0
			};

			return std::pair{ device->CreateBuffer(bufferDesc, &initialData), static_cast<UINT>(indices.size()) };
		}();

		xk::Math::Vector<float, 3> cameraPos{ 0, 0, -3 };
		xk::Math::Vector<float, 3> cameraMovementDirection{ 0, 0, 0 };
		float cameraSpeed = 4;
		xk::Math::SquareMatrix<float, 4> projectionMatrix = xk::Math::PerspectiveProjectionLH(xk::Math::Degree{ 90.f }, 1280.f / 720.f, 0.001f, 1000.f);
		TypedD3D::Wrapper<ID3D11Buffer> cameraBuffer = [&]
		{
			auto initialValue = projectionMatrix * xk::Math::TransformMatrix(-cameraPos);
			D3D11_BUFFER_DESC bufferDesc
			{
				.ByteWidth = sizeof(projectionMatrix),
				.Usage = D3D11_USAGE_DYNAMIC,
				.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
				.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
				.MiscFlags = 0,
				.StructureByteStride = 0
			};

			D3D11_SUBRESOURCE_DATA initialData
			{
				.pSysMem = &initialValue,
				.SysMemPitch = sizeof(initialValue),
				.SysMemSlicePitch = 0
			};
			return device->CreateBuffer(bufferDesc, &initialData);
		}();


		xk::Math::Quaternion<float> objectRotation;
		TypedD3D::Wrapper<ID3D11Buffer> objectBuffer = [&]
		{
			D3D11_BUFFER_DESC bufferDesc
			{
				.ByteWidth = sizeof(xk::Math::SquareMatrix<float, 4>),
				.Usage = D3D11_USAGE_DYNAMIC,
				.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
				.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
				.MiscFlags = 0,
				.StructureByteStride = 0
			};
			return device->CreateBuffer(bufferDesc, nullptr);
		}();

		TypedD3D::Wrapper<ID3D11DepthStencilView> dsv = [&]
		{
			auto desc = backBuffer->GetResource<ID3D11Texture2D>()->GetDesc();
			desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			desc.Format = DXGI_FORMAT_D32_FLOAT;
			auto texture = device->CreateTexture2D(desc, nullptr);
			return device->CreateDepthStencilView(texture);
		}();
		TypedD3D::Wrapper<ID3D11RasterizerState> rasterizerState = [&]
		{
			D3D11_RASTERIZER_DESC desc
			{
				.FillMode = D3D11_FILL_SOLID,
				.CullMode = D3D11_CULL_BACK,
				.FrontCounterClockwise = false,
				.DepthBias = 0,
				.DepthBiasClamp = 0,
				.SlopeScaledDepthBias = 0,
				.DepthClipEnable = true,
				.ScissorEnable = false,
				.MultisampleEnable = false,
				.AntialiasedLineEnable = false,
			};
			return device->CreateRasterizerState(desc);
		}();

		TypedD3D::Wrapper<ID3D11ShaderResourceView> texture = [&]
		{
			SDL2pp::unique_ptr<SDL2pp::Surface> surface = IMG_Load("container.png");
			auto error = SDL_GetError();
			D3D11_TEXTURE2D_DESC desc
			{
				.Width = static_cast<UINT>(surface.get()->w),
				.Height = static_cast<UINT>(surface.get()->h),
				.MipLevels = 1,
				.ArraySize = 1,
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.SampleDesc {.Count = 1},
				.Usage = D3D11_USAGE_IMMUTABLE,
				.BindFlags = D3D11_BIND_SHADER_RESOURCE,
				.CPUAccessFlags = 0,
				.MiscFlags = {}
			};

			D3D11_SUBRESOURCE_DATA data
			{ 
				.pSysMem = surface.get()->pixels,
				.SysMemPitch = static_cast<UINT>(surface.get()->pitch),
				.SysMemSlicePitch = static_cast<UINT>(surface.get()->pitch * surface.get()->h),
			};

			TypedD3D::Wrapper<ID3D11Texture2D> texture = device->CreateTexture2D(desc, &data);

			return device->CreateShaderResourceView(texture, nullptr);
		}();

		TypedD3D::Wrapper<ID3D11ShaderResourceView> texture2 = [&]
		{
			SDL2pp::unique_ptr<SDL2pp::Surface> surface = IMG_Load("awesomeface.png");
			auto error = SDL_GetError();
			D3D11_TEXTURE2D_DESC desc
			{
				.Width = static_cast<UINT>(surface.get()->w),
				.Height = static_cast<UINT>(surface.get()->h),
				.MipLevels = 1,
				.ArraySize = 1,
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.SampleDesc {.Count = 1},
				.Usage = D3D11_USAGE_IMMUTABLE,
				.BindFlags = D3D11_BIND_SHADER_RESOURCE,
				.CPUAccessFlags = 0,
				.MiscFlags = {}
			};

			D3D11_SUBRESOURCE_DATA data
			{
				.pSysMem = surface.get()->pixels,
				.SysMemPitch = static_cast<UINT>(surface.get()->pitch),
				.SysMemSlicePitch = static_cast<UINT>(surface.get()->pitch * surface.get()->h),
			};

			TypedD3D::Wrapper<ID3D11Texture2D> texture = device->CreateTexture2D(desc, &data);

			return device->CreateShaderResourceView(texture, nullptr);
		}();

		TypedD3D::Wrapper<ID3D11SamplerState> linearSampler = device->CreateSamplerState(
			{ 
				.Filter = D3D11_FILTER_ANISOTROPIC,
				.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
				.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
				.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
				.MipLODBias = 0,
				.MaxAnisotropy = 16,
				.ComparisonFunc = D3D11_COMPARISON_NEVER,
				.BorderColor = { 1, 1, 1, 1},
				.MinLOD = -FLT_MAX,
				.MaxLOD = FLT_MAX,
			});

		SDL2pp::Event event;
		std::chrono::time_point previous = std::chrono::steady_clock::now();
		while(true)
		{
			if(SDL2pp::PollEvent(event))
			{
				if(event.type == SDL2pp::EventType::SDL_QUIT)
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

				deviceContext->ClearRenderTargetView(backBuffer, { 0.2f, 0.3f, 0.3f, 1.0f });
				deviceContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1, 0);
				deviceContext->OMSetRenderTargets(backBuffer, dsv);
				deviceContext->RSSetState(rasterizerState);
				deviceContext->RSSetViewports({ .Width = 1280, .Height = 720, .MinDepth = 0, .MaxDepth = 1 });
				deviceContext->IASetInputLayout(layout);
				deviceContext->IASetVertexBuffers(0, vertexBuffer, sizeof(Vertex), 0);
				deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
				deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				{
					auto subresource = deviceContext->Map(objectBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0);
					//objectRotation *= xk::Math::Quaternion<float>{ xk::Math::Normalize<float, 3>({ 0, 0, 1 }), xk::Math::Degree<float>{ 90.f * 0.016f } };
					objectRotation *= xk::Math::Quaternion<float>{ xk::Math::Normalize<float, 3>({ 1, 1, 1 }), xk::Math::Degree<float>{ 90.f * std::chrono::duration<float>{ delta }.count()} };
					auto objectTransform = objectRotation.ToRotationMatrix() * xk::Math::SquareMatrix<float, 4>::Identity();
					std::memcpy(subresource.pData, &objectTransform, sizeof(objectTransform));
					deviceContext->Unmap(objectBuffer, 0);
				}
				{
					auto subresource = deviceContext->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0);
					auto cameraTransform = projectionMatrix * xk::Math::TransformMatrix(-cameraPos);
					std::memcpy(subresource.pData, &cameraTransform, sizeof(cameraTransform));
					deviceContext->Unmap(cameraBuffer, 0);
				}

				deviceContext->VSSetShader(vertexShader, nullptr);
				deviceContext->VSSetConstantBuffers(0, cameraBuffer);
				deviceContext->VSSetConstantBuffers(1, objectBuffer);
				deviceContext->PSSetShader(pixelShader, nullptr);
				deviceContext->PSSetSamplers(0, linearSampler);
				deviceContext->PSSetShaderResources(0, texture);
				deviceContext->PSSetShaderResources(1, texture2);
				deviceContext->DrawIndexed(indexCount, 0, 0);
				swapChain->Present(0, 0);
				previous = current;
			}
		}
	}

	debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_FLAGS::D3D11_RLDO_SUMMARY | D3D11_RLDO_FLAGS::D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
	return 0;
}