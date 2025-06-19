module;

#include <d3d12.h>
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <chrono>
#include <array>
#include <span>
#include <cassert>
#include <ranges>
#include <concepts>

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

		template<std::convertible_to<typename TypedD3D12::D3D12CommandQueue_t<Type>::traits_type::list_value_type> ListTy, size_t Extents>
		void ExecuteCommandLists(
			TypedD3D::Array<ListTy, Extents> commandLists)
		{
			commandQueue->ExecuteCommandLists(TypedD3D::Span{ commandLists });
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



	export struct StagedBufferUpload
	{
		TypedD3D::Wrapper<ID3D12Resource> stagingResource;
		std::size_t offset;
		std::size_t size;
	};

	export template<class Ty>
	StagedBufferUpload StageBufferUpload(Ty data, TypedD3D::Wrapper<ID3D12Resource> stagingResource)
	{
		if constexpr (std::ranges::sized_range<Ty> && std::ranges::contiguous_range<Ty>)
		{
			static_assert(std::is_trivially_copyable_v<std::ranges::range_value_t<Ty>>);

			const auto byteSize = std::ranges::size(data) * sizeof(std::ranges::range_value_t<Ty>);
			assert(byteSize <= stagingResource->GetDesc().Width);
			std::byte* ptr = stagingResource->Map(0, nullptr);
			std::memcpy(ptr, std::ranges::data(data), byteSize);
			stagingResource->Unmap(0, nullptr);
			return { stagingResource, 0, byteSize };
		}
		else
		{
			static_assert(std::is_trivially_copyable_v<Ty>);

			assert(sizeof(Ty) <= stagingResource->GetDesc().Width);
			std::byte* ptr = stagingResource->Map(0, nullptr);
			std::memcpy(ptr, &data, sizeof(data));
			stagingResource->Unmap(0, nullptr);
			return { stagingResource, 0, sizeof(data) };
		}
	}

	export template<class Ty>
	StagedBufferUpload StageBufferUpload(Ty data, TypedD3D::Wrapper<ID3D12Device> device)
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
	StagedBufferUpload StageBufferUpload(std::span<Ty, Extent> data, TypedD3D::Wrapper<ID3D12Device> device)
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

		return StageBufferUpload(data, device->CreateCommittedResource(uploadProperties, D3D12_HEAP_FLAG_NONE,
			description,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr));
	}

	export TypedD3D::Wrapper<ID3D12Resource> CreateBuffer(TypedD3D::Wrapper<ID3D12Device> device, StagedBufferUpload stagedUpload)
	{
		D3D12_HEAP_PROPERTIES headProperties
		{
			.Type = D3D12_HEAP_TYPE_DEFAULT,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 0,
			.VisibleNodeMask = 0
		};

		D3D12_RESOURCE_DESC description
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = 0,
			.Width = stagedUpload.size,
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = {.Count = 1, . Quality = 0},
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAG_NONE
		};

		return device->CreateCommittedResource(headProperties,
			D3D12_HEAP_FLAG_NONE,
			description,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr);
	}

	export TypedD3D::Wrapper<ID3D12Resource> UploadBuffer(TypedD3D12::Direct<ID3D12GraphicsCommandList> commandList, TypedD3D::Wrapper<ID3D12Resource> destinationResource, StagedBufferUpload stagedUpload)
	{
		if (destinationResource->GetDesc().Width < stagedUpload.size)
			throw std::exception("Destination has insufficient size");

		commandList->CopyBufferRegion(*destinationResource.Get(), 0, *stagedUpload.stagingResource.Get(), stagedUpload.offset, stagedUpload.size);

		return destinationResource;
	}

	export TypedD3D::Wrapper<ID3D12Resource> UploadBuffer(TypedD3D12::Direct<ID3D12GraphicsCommandList> commandList, StagedBufferUpload stagedUpload)
	{
		return UploadBuffer(commandList, CreateBuffer(commandList->GetDevice(), stagedUpload), stagedUpload);
	}

	export enum class RenderTargetBarrierSync
	{
		All = D3D12_BARRIER_SYNC_ALL,
		Draw = D3D12_BARRIER_SYNC_DRAW,
		RenderTarget = D3D12_BARRIER_SYNC_RENDER_TARGET
	};

	export enum class BeforeTag {};
	export enum class AfterTag {};

	export struct GlobalBarrier
	{
		D3D12_GLOBAL_BARRIER barrier{};

		template<std::invocable<D3D12_GLOBAL_BARRIER, BeforeTag> Func>
		GlobalBarrier Before(Func func)
		{ 
			return { func(barrier, BeforeTag{}) };
		}

		template<std::invocable<D3D12_GLOBAL_BARRIER, AfterTag> Func>
		GlobalBarrier After(Func func)
		{ 
			return { func(barrier, AfterTag{}) };
		}

		operator D3D12_GLOBAL_BARRIER() const { return barrier; }
	};

	export struct TextureBarrier
	{
		D3D12_TEXTURE_BARRIER barrier{};

		TextureBarrier(D3D12_TEXTURE_BARRIER barrier) :
			barrier{ barrier }
		{

		}

		TextureBarrier(ID3D12Resource* resource) :
			barrier{ .pResource = resource }
		{

		}

		template<std::invocable<D3D12_TEXTURE_BARRIER, BeforeTag> Func>
		TextureBarrier Before(Func func)
		{ 
			return { func(barrier, BeforeTag{}) };
		}

		template<std::invocable<D3D12_TEXTURE_BARRIER, AfterTag> Func>
		TextureBarrier After(Func func)
		{ 
			return { func(barrier, AfterTag{}) };
		}

		operator D3D12_TEXTURE_BARRIER() const { return barrier; }
	};

	export struct BufferBarrier
	{
		D3D12_BUFFER_BARRIER barrier{};

		BufferBarrier(D3D12_BUFFER_BARRIER barrier) :
			barrier{ barrier }
		{

		}

		BufferBarrier(ID3D12Resource* resource) :
			barrier
			{
				.pResource = resource,
				.Offset = 0,
				.Size = resource->GetDesc().Width,
			}
		{

		}

		BufferBarrier(ID3D12Resource* resource, UINT64 offset, UINT64 size) :
			barrier
			{
				.pResource = resource,
				.Offset = offset,
				.Size = size,
			}
		{

		}

		template<std::invocable<D3D12_BUFFER_BARRIER, BeforeTag> Func>
		BufferBarrier Before(Func func)
		{ 
			return { func(barrier, BeforeTag{}) };
		}

		template<std::invocable<D3D12_BUFFER_BARRIER, AfterTag> Func>
		BufferBarrier After(Func func)
		{ 
			return { func(barrier, AfterTag{}) };
		}

		operator D3D12_BUFFER_BARRIER() const { return barrier; }
	};

	//Call NoneTextureBarrier if trying to insert a texture barrier
	export auto NoAccessBarrier()
	{
		return [=](auto barrier, auto tag) requires (!std::convertible_to<decltype(barrier), D3D12_TEXTURE_BARRIER>)
		{
			if constexpr (std::same_as<BeforeTag, std::remove_reference_t<decltype(tag)>>)
			{
				barrier.SyncBefore = D3D12_BARRIER_SYNC_NONE;
				barrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
			}
			else
			{
				barrier.SyncAfter = D3D12_BARRIER_SYNC_NONE;
				barrier.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
			}
			return barrier;
		};
	}

	export auto NoAccessTextureBarrier(D3D12_BARRIER_LAYOUT layout)
	{
		return [=](D3D12_TEXTURE_BARRIER barrier, auto tag)
		{
			if constexpr (std::same_as<BeforeTag, std::remove_reference_t<decltype(tag)>>)
			{
				barrier.SyncBefore = D3D12_BARRIER_SYNC_NONE;
				barrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
				barrier.LayoutBefore = layout;
			}
			else
			{
				barrier.SyncAfter = D3D12_BARRIER_SYNC_NONE;
				barrier.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
				barrier.LayoutAfter = layout;
			}
			return barrier;
		};
	}

	export auto RenderTargetTextureBarrier(RenderTargetBarrierSync sync = RenderTargetBarrierSync::RenderTarget)
	{
		return [=](D3D12_TEXTURE_BARRIER barrier, auto tag)
		{
			if constexpr (std::same_as<BeforeTag, std::remove_reference_t<decltype(tag)>>)
			{
				barrier.SyncBefore = static_cast<D3D12_BARRIER_SYNC>(sync);
				barrier.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET;
				barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
			}
			else
			{
				barrier.SyncAfter = static_cast<D3D12_BARRIER_SYNC>(sync);
				barrier.AccessAfter = D3D12_BARRIER_ACCESS_RENDER_TARGET;
				barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
			}
			return barrier;
		};
	}

	export auto VertexBufferBarrier()
	{
		return [=](D3D12_BUFFER_BARRIER barrier, auto tag)
		{
			if constexpr (std::same_as<BeforeTag, std::remove_reference_t<decltype(tag)>>)
			{
				barrier.SyncBefore = D3D12_BARRIER_SYNC_VERTEX_SHADING;
				barrier.AccessBefore = D3D12_BARRIER_ACCESS_VERTEX_BUFFER;
			}
			else
			{
				barrier.SyncAfter = D3D12_BARRIER_SYNC_VERTEX_SHADING;
				barrier.AccessAfter = D3D12_BARRIER_ACCESS_VERTEX_BUFFER;
			}
			return barrier;
		};
	}

	export template<std::derived_from<ID3D12GraphicsCommandList> ListTy, template<class> class Trait, std::invocable<TypedD3D::IUnknownWrapper<ListTy, Trait>> Func>
	TypedD3D::IUnknownWrapper<ListTy, Trait> Record(TypedD3D::IUnknownWrapper<ListTy, Trait> commandList, TypedD3D::IUnknownWrapper<ID3D12CommandAllocator, Trait> allocator, ID3D12PipelineState* pipeline, Func func)
	{
		allocator->Reset();
		commandList->Reset(allocator, pipeline);
		func(commandList);
		commandList->Close();
		return commandList;
	}

	export template<std::derived_from<ID3D12GraphicsCommandList> ListTy, template<class> class Trait, std::invocable<TypedD3D::IUnknownWrapper<ListTy, Trait>> Func>
	TypedD3D::IUnknownWrapper<ListTy, Trait> Record(TypedD3D::IUnknownWrapper<ListTy, Trait> commandList, TypedD3D::ConstElementReference<TypedD3D::IUnknownWrapper<ID3D12CommandAllocator, Trait>> allocator, ID3D12PipelineState* pipeline, Func func)
	{
		return Record(commandList, allocator.ToWrapper(), pipeline, func);
	}

	export template<std::derived_from<ID3D12GraphicsCommandList> ListTy, template<class> class Trait, std::invocable<TypedD3D::IUnknownWrapper<ListTy, Trait>> Func>
	TypedD3D::IUnknownWrapper<ListTy, Trait> Record(TypedD3D::IUnknownWrapper<ListTy, Trait> commandList, TypedD3D::ElementReference<TypedD3D::IUnknownWrapper<ID3D12CommandAllocator, Trait>> allocator, ID3D12PipelineState* pipeline, Func func)
	{
		return Record(commandList, allocator.ToWrapper(), pipeline, func);
	}

}