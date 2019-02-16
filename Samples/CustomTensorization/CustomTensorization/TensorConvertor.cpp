#include "pch.h"
#include "TensorConvertor.h"
#include <MemoryBuffer.h>
// d3dx12.h can be downloaded from https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/Libraries/D3DX12/d3dx12.h
// d3dx12.h is a cool helper library, that lets us use Updatesubresources().
#include "d3dx12.h"
#include "robuffer.h"
#include "Windows.AI.MachineLearning.Native.h"
#include <windows.h>

#define FENCE_SIGNAL_VALUE 1
#define CHECK_HRESULT winrt::check_hresult
using namespace winrt;
using namespace winrt::Windows::AI::MachineLearning;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Media;
using namespace winrt::Windows::Graphics::Imaging;

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace TensorizationHelper
{
    std::wstring GetModulePath(bool visiblePath)
    {
        std::wstring val;
        wchar_t modulePath[MAX_PATH] = { 0 };
        GetModuleFileNameW((HINSTANCE)&__ImageBase, modulePath, _countof(modulePath));
        wchar_t drive[_MAX_DRIVE];
        wchar_t dir[_MAX_DIR];
        wchar_t filename[_MAX_FNAME];
        wchar_t ext[_MAX_EXT];
        errno_t err = _wsplitpath_s(modulePath, drive, _MAX_DRIVE, dir, _MAX_DIR, filename, _MAX_FNAME, ext, _MAX_EXT);

        val = drive;
        val += dir;
		if (visiblePath) {
			int32_t i = val.find(filename);
			// only return the modified path when there is a parent folder which contains a CustomTensorization folder
			// This will match when a user is running this sample in the repository's directory structure
			if (i != std::wstring::npos) {
				val = val.substr(0, i + wcslen(filename)) + L"\\" + filename + L"\\";
			}
		}
        return val;
    }

    TensorFloat SoftwareBitmapToSoftwareTensor(SoftwareBitmap softwareBitmap)
    {
        /* Manully tensorize from CPU resource, steps:
        1. Get the access to buffer of softwarebitmap
        2. Transform the data in buffer to a vector of float
        */

        // 1. Get the access to buffer of softwarebitmap
        BYTE* pData = nullptr;
        UINT32 size = 0;
        BitmapBuffer spBitmapBuffer(softwareBitmap.LockBuffer(BitmapBufferAccessMode::Read));
        winrt::Windows::Foundation::IMemoryBufferReference reference = spBitmapBuffer.CreateReference();
        auto spByteAccess = reference.as<::Windows::Foundation::IMemoryBufferByteAccess>();
        CHECK_HRESULT(spByteAccess->GetBuffer(&pData, &size));

        uint32_t height = softwareBitmap.PixelHeight();
        uint32_t width = softwareBitmap.PixelWidth();
        BitmapPixelFormat pixelFormat = softwareBitmap.BitmapPixelFormat();
        uint32_t channels = BitmapPixelFormat::Gray8 == pixelFormat ? 1 : 3;

        std::vector<int64_t> shape = { 1, channels, height , width };
        float* pCPUTensor;
        uint32_t uCapacity;

        // The channels of image stored in buffer is in order of BGRA-BGRA-BGRA-BGRA. 
        // Then we transform it to the order of BBBBB....GGGGG....RRRR....AAAA(dropped) 
        TensorFloat tf = TensorFloat::Create(shape);
        com_ptr<ITensorNative> itn = tf.as<ITensorNative>();
        CHECK_HRESULT(itn->GetBuffer(reinterpret_cast<BYTE**>(&pCPUTensor), &uCapacity));

        // 2. Transform the data in buffer to a vector of float
        if (BitmapPixelFormat::Bgra8 == pixelFormat)
        {
            for (UINT32 i = 0; i < size; i += 4)
            {
                // suppose the model expects BGR image.
                // index 0 is B, 1 is G, 2 is R, 3 is alpha(dropped).
                UINT32 pixelInd = i / 4;
                pCPUTensor[pixelInd] = (float)pData[i];
                pCPUTensor[(height * width) + pixelInd] = (float)pData[i + 1];
                pCPUTensor[(height * width * 2) + pixelInd] = (float)pData[i + 2];
            }
        }
        else if (BitmapPixelFormat::Rgba8 == pixelFormat)
        {
            for (UINT32 i = 0; i < size; i += 4)
            {
                // suppose the model expects BGR image.
                // index 0 is B, 1 is G, 2 is R, 3 is alpha(dropped).
                UINT32 pixelInd = i / 4;
                pCPUTensor[pixelInd] = (float)pData[i + 2];
                pCPUTensor[(height * width) + pixelInd] = (float)pData[i + 1];
                pCPUTensor[(height * width * 2) + pixelInd] = (float)pData[i];
            }
        }
        else if (BitmapPixelFormat::Gray8 == pixelFormat)
        {
            for (UINT32 i = 0; i < size; i += 4)
            {
                // suppose the model expects BGR image.
                // index 0 is B, 1 is G, 2 is R, 3 is alpha(dropped).
                UINT32 pixelInd = i / 4;
                float red = (float)pData[i + 2];
                float green = (float)pData[i + 1];
                float blue = (float)pData[i];
                float gray = 0.2126f * red + 0.7152f * green + 0.0722f * blue;
                pCPUTensor[pixelInd] = gray;
            }
        }
        // Pixel Value Normalization can be done at here. We are using the range from 0-255, 
        // but the range can be normilized to 0-1 before we return the TensorFloat.
        return tf;
    }

    TensorFloat SoftwareBitmapToDX12Tensor(
        SoftwareBitmap softwareBitmap)
    {
        /* Manully tensorize from GPU resource, steps:
        1. create the d3d device.
        2. create a command queue
        3. Create a command list.
        4. create an upload heap.
        5. create an upload resource.
        6. on that cmd list call UpdateSubresources to copy the cpu -> gpu memory
        7. close that cmd list.
        8. execute the cmd queue.
        9. add a fence to wait util cmd executed
        */

        // Copy and transform the data from SoftwareBitmap buffer to a vector of float first as in LoadInputImageFromCPU().
        float* pCPUTensor;
        uint32_t uCapacity;

        // CPU tensorization
        TensorFloat tf = SoftwareBitmapToSoftwareTensor(softwareBitmap);
        com_ptr<ITensorNative> itn = tf.as<ITensorNative>();
        CHECK_HRESULT(itn->GetBuffer(reinterpret_cast<BYTE**>(&pCPUTensor), &uCapacity));

        // 1. create the d3d device.
        com_ptr<ID3D12Device> pD3D12Device = nullptr;
        CHECK_HRESULT(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), reinterpret_cast<void**>(&pD3D12Device)));

        // 2. create the command queue.
        com_ptr<ID3D12CommandQueue> dxQueue = nullptr;
        D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
        commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        CHECK_HRESULT(pD3D12Device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), reinterpret_cast<void**>(&dxQueue)));
        com_ptr<ILearningModelDeviceFactoryNative> devicefactory = get_activation_factory<LearningModelDevice, ILearningModelDeviceFactoryNative>();
        com_ptr<::IUnknown> spUnk;
        CHECK_HRESULT(devicefactory->CreateFromD3D12CommandQueue(dxQueue.get(), spUnk.put()));

        // 3. Create ID3D12GraphicsCommandList and Allocator
        D3D12_COMMAND_LIST_TYPE queuetype = dxQueue->GetDesc().Type;
        com_ptr<ID3D12CommandAllocator> alloctor;
        com_ptr<ID3D12GraphicsCommandList> cmdList;

        CHECK_HRESULT(pD3D12Device->CreateCommandAllocator(
            queuetype,
            winrt::guid_of<ID3D12CommandAllocator>(),
            alloctor.put_void()));

        CHECK_HRESULT(pD3D12Device->CreateCommandList(
            0,
            queuetype,
            alloctor.get(),
            nullptr,
            winrt::guid_of<ID3D12CommandList>(),
            cmdList.put_void()));

        // 4. Create Committed Resource
        // 3 is number of channels we use. R G B without alpha.
        UINT64 bufferbytesize = 3 * sizeof(float) * softwareBitmap.PixelWidth()*softwareBitmap.PixelHeight();
        D3D12_HEAP_PROPERTIES heapProperties = {
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN,
            0,
            0
        };
        D3D12_RESOURCE_DESC resourceDesc = {
            D3D12_RESOURCE_DIMENSION_BUFFER,
            0,
            bufferbytesize,
            1,
            1,
            1,
            DXGI_FORMAT_UNKNOWN,
        { 1, 0 },
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
        };

        com_ptr<ID3D12Resource> pGPUResource = nullptr;
        com_ptr<ID3D12Resource> imageUploadHeap;
        CHECK_HRESULT(pD3D12Device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            __uuidof(ID3D12Resource),
            pGPUResource.put_void()
        ));

        // 5. Create the GPU upload buffer.
        CHECK_HRESULT(pD3D12Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(bufferbytesize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            __uuidof(ID3D12Resource),
            imageUploadHeap.put_void()));

        // 6. Copy from Cpu to GPU
        D3D12_SUBRESOURCE_DATA CPUData = {};
        CPUData.pData = reinterpret_cast<BYTE*>(pCPUTensor);
        CPUData.RowPitch = bufferbytesize;
        CPUData.SlicePitch = bufferbytesize;
        UpdateSubresources(cmdList.get(), pGPUResource.get(), imageUploadHeap.get(), 0, 0, 1, &CPUData);

        // 7. Close the command list and execute it to begin the initial GPU setup.
        CHECK_HRESULT(cmdList->Close());

        // 8. execute the cmd queue.
        ID3D12CommandList* ppCommandLists[] = { cmdList.get() };
        dxQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // 9. add a fence to wait util cmd executed

        //Create Event
        HANDLE directEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        //Create Fence
        Microsoft::WRL::ComPtr<ID3D12Fence> spDirectFence = nullptr;
        CHECK_HRESULT(pD3D12Device->CreateFence(
            0,
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(spDirectFence.ReleaseAndGetAddressOf())));
        //Adds fence to queue
        CHECK_HRESULT(dxQueue->Signal(spDirectFence.Get(), FENCE_SIGNAL_VALUE));
        CHECK_HRESULT(spDirectFence->SetEventOnCompletion(FENCE_SIGNAL_VALUE, directEvent));

        //Wait for signal
        DWORD retVal = WaitForSingleObject(directEvent, INFINITE);
        if (retVal != WAIT_OBJECT_0)
        {
            printf("Unexpected error");
        }

        // GPU tensorize
        com_ptr<ITensorStaticsNative> tensorfactory = get_activation_factory<TensorFloat, ITensorStaticsNative>();
        com_ptr<::IUnknown> spUnkTensor;
        TensorFloat input1imagetensor(nullptr);
        int64_t shapes[4] = { 1,3, softwareBitmap.PixelWidth(), softwareBitmap.PixelHeight() };
        CHECK_HRESULT(tensorfactory->CreateFromD3D12Resource(pGPUResource.get(), shapes, 4, spUnkTensor.put()));
        spUnkTensor.try_as(input1imagetensor);

        return input1imagetensor;
    }
}