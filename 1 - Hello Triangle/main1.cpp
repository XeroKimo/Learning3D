
#include <d3d11.h>
#include <dxgi1_4.h>
#include <d3d11sdklayers.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>

import TypedD3D11;
import TypedDXGI;
import SDL2pp;

#undef CreateWindow;

int main()
{
	SDL2pp::unique_ptr<SDL2pp::Window> window = SDL2pp::CreateWindow("1 - Hello Triangle", { 1280, 720 }, {});

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

		std::array<D3D11_INPUT_ELEMENT_DESC, 2> elements
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
				.SemanticName = "COLOR",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.InputSlot = 0,
				.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
		};
		Microsoft::WRL::ComPtr<ID3DBlob> blob;
		TypedD3D::ThrowIfFailed(D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", {}, {}, &blob, nullptr));
		TypedD3D::Wrapper<ID3D11InputLayout> layout = device->CreateInputLayout(elements, *blob.Get());
		TypedD3D::Wrapper<ID3D11VertexShader> vertexShader = device->CreateVertexShader(*blob.Get(), nullptr);

		TypedD3D::ThrowIfFailed(D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", {}, {}, &blob, nullptr));
		TypedD3D::Wrapper<ID3D11PixelShader> pixelShader = device->CreatePixelShader(*blob.Get(), nullptr);

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
				.SysMemPitch = sizeof(Vertex),
				.SysMemSlicePitch = 0
			};

			return std::pair{ device->CreateBuffer(bufferDesc, &initialData), static_cast<UINT>(vertices.size()) };
		}();

		SDL2pp::Event event;
		while(true)
		{
			if(SDL2pp::PollEvent(event))
			{
				if(event.type == SDL2pp::EventType::SDL_QUIT)
				{
					break;
				}
			}
			else
			{
				deviceContext->ClearRenderTargetView(backBuffer, { 0.2f, 0.3f, 0.3f, 1.0f });
				deviceContext->OMSetRenderTargets(backBuffer, nullptr);
				deviceContext->RSSetViewports({ .Width = 1280, .Height = 720, .MinDepth = 0, .MaxDepth = 1 });
				deviceContext->IASetInputLayout(layout);
				deviceContext->IASetVertexBuffers(0, vertexBuffer, sizeof(Vertex), 0);
				deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				deviceContext->VSSetShader(vertexShader, nullptr);
				deviceContext->PSSetShader(pixelShader, nullptr);
				deviceContext->Draw(vertexCount, 0);
				swapChain->Present(0, 0);
			}
		}
	}

	debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_FLAGS::D3D11_RLDO_SUMMARY | D3D11_RLDO_FLAGS::D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
	return 0;
}