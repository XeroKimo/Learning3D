module;

#include <d3d12.h>
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <chrono>
#include <array>
#include <span>
#include <cassert>

export module D3D12Extensions;
import TypedD3D12;
import TypedDXGI;

namespace xk::D3D12
{
	export class Fence
	{
		template<D3D12_COMMAND_LIST_TYPE Type>
		friend class CommandQueue;

		UINT64 lastSignaledValue = 0;
		Microsoft::WRL::ComPtr<ID3D12Fence> fence;

	public:
		Fence(TypedD3D::Wrapper<ID3D12Device> device) :
			fence{ device->CreateFence(lastSignaledValue, D3D12_FENCE_FLAG_NONE) }
		{
		}

	public:
		UINT64 GetLastSignaledValue() const
		{
			return lastSignaledValue;
		}

		UINT64 GetCompletedValue()
		{
			return fence->GetCompletedValue();
		}

		UINT64 Signal(UINT64 value)
		{
			lastSignaledValue = value;
			fence->Signal(lastSignaledValue);
			return lastSignaledValue;
		}

		void Reset()
		{
			Signal(0);
		}

		HRESULT SetEventOnCompletion(HANDLE hEvent)
		{
			return SetEventOnCompletion(lastSignaledValue, hEvent);
		}

		HRESULT SetEventOnCompletion(UINT64 fenceValue, HANDLE hEvent)
		{
			return fence->SetEventOnCompletion(fenceValue, hEvent);
		}

		void Flush()
		{
			SetEventOnCompletion(lastSignaledValue, nullptr);
			assert(fence->GetCompletedValue() == lastSignaledValue);
		}

		void CPUThreadWait(UINT64 fenceValue)
		{
			if (fence->GetCompletedValue() >= fenceValue)
				return;

			SetEventOnCompletion(fenceValue, nullptr);
		}

		void CPUThreadWait(UINT64 fenceValue, HANDLE hEvent, std::chrono::duration<DWORD, std::milli> waitInterval /*= (std::chrono::duration<DWORD, std::milli>::max)()*/)
		{
			if (fence->GetCompletedValue() >= fenceValue)
				return;

			SetEventOnCompletion(fenceValue, hEvent);
			WaitForSingleObject(hEvent, waitInterval.count());
		}
	};

	export template<D3D12_COMMAND_LIST_TYPE Type>
		class CommandQueue
	{
		template<D3D12_COMMAND_LIST_TYPE Type>
		friend class CommandQueue;

		Fence fence;
		TypedD3D::D3D12::D3D12CommandQueue_t<Type> commandQueue;
	public:
		CommandQueue(TypedD3D::Wrapper<ID3D12Device> device) :
			fence{ device },
			commandQueue{ device->CreateCommandQueue<D3D12_COMMAND_LIST_TYPE_DIRECT>(D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE, 0) }
		{

		}

	public:
		TypedD3D::D3D12::D3D12CommandQueue_t<Type> Get() const { return commandQueue; }

		UINT64 GPUSignal()
		{
			fence.lastSignaledValue++;
			commandQueue->Signal(*fence.fence.Get(), fence.lastSignaledValue);
			return fence.lastSignaledValue;
		}

		UINT64 CPUSignal(UINT64 value)
		{
			return fence.Signal(value);
		}

		void BeginEvent(
			UINT Metadata,
			const void* pData,
			UINT Size)
		{
			commandQueue->BeginEvent(Metadata, pData, Size);
		}

		void CopyTileMappings(
			ID3D12Resource& pDstResource,
			const D3D12_TILED_RESOURCE_COORDINATE& pDstRegionStartCoordinate,
			ID3D12Resource& pSrcResource,
			const D3D12_TILED_RESOURCE_COORDINATE& pSrcRegionStartCoordinate,
			const D3D12_TILE_REGION_SIZE& pRegionSize,
			D3D12_TILE_MAPPING_FLAGS Flags)
		{
			commandQueue->CopyTileMappings(
				pDstResource,
				pDstRegionStartCoordinate,
				pSrcResource,
				pSrcRegionStartCoordinate,
				pRegionSize,
				Flags);
		}

		void EndEvent() { commandQueue->EndEvent(); }

		TypedD3D12::ClockCalibrationData GetClockCalibration()
		{
			return commandQueue->GetClockCalibration();
		}

		D3D12_COMMAND_QUEUE_DESC GetDesc() { return commandQueue->GetDesc(); }

		UINT64 GetTimestampFrequency()
		{
			return commandQueue->GetTimestampFrequency();
		}

		void SetMarker(
			UINT Metadata,
			const void* pData,
			UINT Size)
		{
			commandQueue->SetMarker(Metadata, pData, Size);
		}

		void UpdateTileMappings(
			ID3D12Resource& pResource,
			UINT NumResourceRegions,
			const D3D12_TILED_RESOURCE_COORDINATE* pResourceRegionStartCoordinates,
			const D3D12_TILE_REGION_SIZE* pResourceRegionSizes,
			ID3D12Heap* pHeap,
			UINT NumRanges,
			const D3D12_TILE_RANGE_FLAGS* pRangeFlags,
			const UINT* pHeapRangeStartOffsets,
			const UINT* pRangeTileCounts,
			D3D12_TILE_MAPPING_FLAGS Flags)
		{
			commandQueue->UpdateTileMappings(
				pResource,
				NumResourceRegions,
				pResourceRegionStartCoordinates,
				pResourceRegionSizes,
				pHeap,
				NumRanges,
				pRangeFlags,
				pHeapRangeStartOffsets,
				pRangeTileCounts,
				Flags);
		}

		HRESULT GPUWait(
			Fence& Fence,
			UINT64 Value)
		{
			return commandQueue->Wait(*Fence.fence.Get(), Value);
		}		
		
		template<D3D12_COMMAND_LIST_TYPE OtherType>
		HRESULT GPUWait(
			CommandQueue<OtherType>& OtherQueue,
			UINT64 Value)
		{
			return GPUWait(OtherQueue.Fence, Value);
		}

		template<std::convertible_to<typename TypedD3D12::D3D12CommandQueue_t<Type>::traits_type::list_value_type> ListTy, size_t Extents>
		void ExecuteCommandLists(
			TypedD3D::Span<ListTy, Extents> commandLists)
		{
			commandQueue->ExecuteCommandLists(commandLists);
		}

		UINT64 GetLastSignaledValue() const
		{
			return fence.lastSignaledValue;
		}

		UINT64 GetCompletedValue()
		{
			return fence.GetCompletedValue();
		}

		void Reset()
		{
			fence.Reset();
		}

		HRESULT SetEventOnCompletion(HANDLE hEvent)
		{
			return fence.SetEventOnCompletion(hEvent);
		}

		HRESULT SetEventOnCompletion(UINT64 fenceValue, HANDLE hEvent)
		{
			return fence.SetEventOnCompletion(fenceValue, hEvent);
		}

		void Flush()
		{
			GPUSignal();
			fence.Flush();
		}

		void CPUWait(UINT64 fenceValue)
		{
			fence.CPUThreadWait(fenceValue);
		}

		void CPUWait(UINT64 fenceValue, HANDLE hEvent, std::chrono::duration<DWORD, std::milli> waitInterval /*= (std::chrono::duration<DWORD, std::milli>::max)()*/)
		{
			fence.CPUThreadWait(fenceValue, hEvent, waitInterval);
		}
	};

	export class SwapChain
	{
		CommandQueue<D3D12_COMMAND_LIST_TYPE_DIRECT>* associatedQueue;
		TypedD3D::Wrapper<IDXGISwapChain3> swapChain;
		std::array<UINT64, 3> fenceValues{};
		UINT currentFrameIndex = 0;

		UINT64 rtvOffset;
		TypedD3D12::RTV<ID3D12DescriptorHeap> rtvDescriptors;
	public:
		SwapChain(TypedD3D::Wrapper<IDXGIFactory2> factory, TypedD3D::Wrapper<ID3D12Device> device, CommandQueue<D3D12_COMMAND_LIST_TYPE_DIRECT>& commandQueue, HWND hwnd, UINT frameBufferCount, DXGI_SWAP_CHAIN_FLAG flags) :
			associatedQueue{ &commandQueue },
			swapChain{ factory->CreateSwapChainForHwnd<IDXGISwapChain3>(associatedQueue->Get(), hwnd,
				DXGI_SWAP_CHAIN_DESC1{
					.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
					.SampleDesc{.Count = 1 },
					.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT,
					.BufferCount = frameBufferCount,
					.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
					.Flags = static_cast<UINT>(flags)
				}, nullptr, nullptr) },
			rtvDescriptors{ device->CreateDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE>(frameBufferCount, 0) }
		{
			rtvOffset = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			auto handle = rtvDescriptors->GetCPUDescriptorHandleForHeapStart();

			D3D12_RENDER_TARGET_VIEW_DESC viewDesc
			{
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
				.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
				.Texture2D = {.MipSlice = 0 },
			};
			for (UINT i = 0; i < frameBufferCount; i++)
			{
				device->CreateRenderTargetView(*swapChain->GetBuffer<ID3D12Resource>(i).Get(), &viewDesc, handle);
				handle = handle.Offset(1, rtvOffset);
			}
		}

		~SwapChain()
		{
			associatedQueue->Flush();
		}

		TypedD3D::Wrapper<ID3D12Resource> GetBackBuffer() { return swapChain->GetBuffer<ID3D12Resource>(currentFrameIndex); }

		template<class Ty>
		void Frame(Ty function)
		{
			function(*associatedQueue, currentFrameIndex, rtvDescriptors->GetCPUDescriptorHandleForHeapStart().Offset(currentFrameIndex, rtvOffset));
			swapChain->Present(0, 0);

			fenceValues[currentFrameIndex] = associatedQueue->GPUSignal();
			currentFrameIndex = swapChain->GetCurrentBackBufferIndex();
			associatedQueue->CPUWait(fenceValues[currentFrameIndex]);
		}

		CommandQueue<D3D12_COMMAND_LIST_TYPE_DIRECT>& GetQueue() { return *associatedQueue; }
	};




	export template<class Ty>
		requires std::is_trivially_copyable_v<Ty>
	TypedD3D::Wrapper<ID3D12Resource> StageUploadBuffer(Ty data, TypedD3D::Wrapper<ID3D12Resource> stagingResource)
	{
		assert(sizeof(Ty) <= stagingResource->GetDesc().Width);
		std::byte* ptr = stagingResource->Map(0, nullptr);
		std::memcpy(ptr, &data, sizeof(data));
		stagingResource->Unmap(0, nullptr);
		return stagingResource;
	}

	export template<class Ty>
		requires std::is_trivially_copyable_v<Ty>
	TypedD3D::Wrapper<ID3D12Resource> StageUploadBuffer(Ty data, TypedD3D::Wrapper<ID3D12Device> device)
	{
		D3D12_HEAP_PROPERTIES uploadProperties
		{
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 0,
			.VisibleNodeMask = 0
		};

		D3D12_RESOURCE_DESC description
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = 0,
			.Width = sizeof(Ty),
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = {.Count = 1, . Quality = 0},
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAG_NONE
		};

		return StageUploadBuffer(data, device->CreateCommittedResource(uploadProperties, D3D12_HEAP_FLAG_NONE,
			description,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr));
	}


	export template<class Ty, size_t Extent>
		TypedD3D::Wrapper<ID3D12Resource> StageUploadBuffer(std::span<Ty, Extent> data, TypedD3D::Wrapper<ID3D12Resource> stagingResource)
	{
		assert(sizeof(Ty) <= stagingResource->GetDesc().Width);
		std::byte* ptr = stagingResource->Map(0, nullptr);
		std::memcpy(ptr, data.data(), data.size_bytes());
		stagingResource->Unmap(0, nullptr);
		return stagingResource;
	}

	export template<class Ty, size_t Extent>
		TypedD3D::Wrapper<ID3D12Resource> StageUploadBuffer(std::span<Ty, Extent> data, TypedD3D::Wrapper<ID3D12Device> device)
	{
		D3D12_HEAP_PROPERTIES uploadProperties
		{
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 0,
			.VisibleNodeMask = 0
		};

		D3D12_RESOURCE_DESC description
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = 0,
			.Width = data.size_bytes(),
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = {.Count = 1, . Quality = 0},
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAG_NONE
		};

		return StageUploadBuffer(data, device->CreateCommittedResource(uploadProperties, D3D12_HEAP_FLAG_NONE,
			description,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr));
	}
}