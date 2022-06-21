#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "pch.h"
#include "DXResourceBinding.h"
#include "DXResourceBinding.g.cpp"
#include <DirectXMath.h>
#include "stdafx.h"
#include <commctrl.h>
#include <mfapi.h>
#include <iostream>

//#include "DXHelpers.h"

#include <cstdio>
#include <algorithm>
#include <numeric>
#include <functional>
#include <utility>
#include <string_view>
#include <span>
#include <optional>
#include <memory>

#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include "dml_provider_factory.h"
#include "onnxruntime_cxx_api.h"
#include "ORTHelpers.h"

#include "Win32Application.h"

#undef min

Microsoft::WRL::ComPtr<ID3D12Resource> d3dResource;
static std::optional<Ort::Session> preprocesingSession;
static std::optional<Ort::Session> inferenceSession;
static bool closeWindow = false;
D3D12Quad sample(800, 600, L"D3D12 Quad");

static HMODULE GetCurrentModule()
{ // NB: XP+ solution!
    HMODULE hModule = NULL;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCTSTR)GetCurrentModule,
        &hModule);

    return hModule;
}

namespace winrt::WinMLSamplesGalleryNative::implementation
{
	winrt::com_array<float> DXResourceBinding::LaunchWindow() {
        OutputDebugString(L"In Launch Window\n");

        OrtApi const& ortApi = Ort::GetApi(); // Uses ORT_API_VERSION
        const OrtDmlApi* ortDmlApi;
        ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void**>(&ortDmlApi));

        const wchar_t* preprocessingModelFilePath = L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/dx_preprocessor_efficient_net.onnx";
        Ort::Env ortEnvironment(ORT_LOGGING_LEVEL_WARNING, "DirectML_Direct3D_TensorAllocation_Test");
        Ort::SessionOptions preprocessingSessionOptions;
        preprocessingSessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
        preprocessingSessionOptions.DisableMemPattern();
        preprocessingSessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        ortApi.AddFreeDimensionOverrideByName(preprocessingSessionOptions, "batch_size", 1);
        OrtSessionOptionsAppendExecutionProvider_DML(preprocessingSessionOptions, 0);
        preprocesingSession = Ort::Session(ortEnvironment, preprocessingModelFilePath, preprocessingSessionOptions);

        const wchar_t* inferencemodelFilePath = L"C:/Users/numform/Windows-Machine-Learning/Samples/WinMLSamplesGallery/WinMLSamplesGalleryNative/efficientnet-lite4-11.onnx";
        Ort::SessionOptions inferenceSessionOptions;
        inferenceSessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
        inferenceSessionOptions.DisableMemPattern();
        inferenceSessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        ortApi.AddFreeDimensionOverrideByName(inferenceSessionOptions, "batch_size", 1);
        OrtSessionOptionsAppendExecutionProvider_DML(inferenceSessionOptions, 0);
        inferenceSession = Ort::Session(ortEnvironment, inferencemodelFilePath, inferenceSessionOptions);
        
        
        HINSTANCE hInstance = GetCurrentModule();
        //std::thread hwnd_th(StartHWind, hInstance, 10);
        //hwnd_th.detach();
        //Sleep(2000);

        std::thread d3d_th(Win32Application::Run, &sample, hInstance, 10);

        while (!sample.is_initialized) {
            OutputDebugString(L"Not done\n");

        }

        d3d_th.detach();
        OutputDebugString(L"Done. Detached\n");


        //Win32Application::Run(&sample, hInstance, 10);

        //auto results = Preproces(preprocesingSession, inferenceSession);

        winrt::com_array<float> eval_results(1000);
        for (int i = 0; i < 1000; i++) {
            eval_results[i] = 100;
        }
        return eval_results;

		//return results;
	}

    winrt::com_array<float> DXResourceBinding::EvalORT() {
        D3D12Quad::D3DInfo info = sample.GetD3DInfo();
        bool running = true;
        return Preprocess(*preprocesingSession, *inferenceSession, info.device.Get(),
            running, info.swapChain.Get(), info.frameIndex, info.commandAllocator.Get(), info.commandList.Get(),
            info.commandQueue.Get());
    }

    void DXResourceBinding::CloseWindow() {
        closeWindow = true;
    }
}

//int WINAPI StartHWind(HINSTANCE hInstance,    //Main windows function
//    int nShowCmd)
//{
//    // create the window
//    if (!InitializeWindow(hInstance, nShowCmd, FullScreen, hwnd, Width, Height, WindowName, WindowTitle))
//    {
//        MessageBox(0, L"Window Initialization - Failed",
//            L"Error", MB_OK);
//        return 1;
//    }
//
//    // initialize direct3d
//    if (!InitD3D(Running, device, commandQueue, Width, Height, frameBufferCount, hwnd,
//        FullScreen, swapChain, frameIndex, rtvDescriptorHeap, rtvDescriptorSize,
//        renderTargets, commandAllocator, commandList, fence, fenceValue, fenceEvent,
//        rootSignature, pipelineStateObject, vertexBuffer, numCubeIndices, indexBuffer,
//        dsDescriptorHeap, depthStencilBuffer, constantBufferUploadHeaps, cbvGPUAddress,
//        textureBuffer, textureBufferUploadHeap, mainDescriptorHeap, vertexBufferView,
//        indexBufferView, viewport, scissorRect, cameraProjMat, cameraPosition, cameraTarget,
//        cameraUp, cameraViewMat, cube1Position, cube1RotMat, cube1WorldMat, cube2RotMat, cube2WorldMat))
//    {
//        MessageBox(0, L"Failed to initialize direct3d 12",
//            L"Error", MB_OK);
//        Cleanup(frameBufferCount, frameIndex, swapChain, device, commandQueue, rtvDescriptorHeap,
//            commandList, renderTargets, commandAllocator, fence, pipelineStateObject,
//            rootSignature, vertexBuffer, indexBuffer, depthStencilBuffer,
//            dsDescriptorHeap, constantBufferUploadHeaps, fenceValue, fenceEvent, Running);
//        return 1;
//    }
//
//    // start the main loop
//    Running = true;
//    mainloop();
//    closeWindow = false;
//
//    // we want to wait for the gpu to finish executing the command list before we start releasing everything
//    WaitForPreviousFrame(frameIndex, swapChain, fence, fenceValue, fenceEvent, Running);
//
//    // close the fence event
//    CloseHandle(fenceEvent);
//
//    // clean up everything
//    Cleanup(frameBufferCount, frameIndex, swapChain, device, commandQueue, rtvDescriptorHeap,
//        commandList, renderTargets, commandAllocator, fence, pipelineStateObject,
//        rootSignature, vertexBuffer, indexBuffer, depthStencilBuffer, dsDescriptorHeap,
//        constantBufferUploadHeaps, fenceValue, fenceEvent, Running);
//
//    if (!UnregisterClass(WindowName, hInstance))
//    {
//        auto error = GetLastError();
//        MessageBox(NULL, L"Error unregistering class",
//            L"Error", MB_OK | MB_ICONERROR);
//        return 1;
//    }
//
//    return 0;
//}
//
//void mainloop() {
//    MSG msg;
//    ZeroMemory(&msg, sizeof(MSG));
//
//    //initORT();
//
//    while (Running)
//    {
//        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
//        {
//            if (msg.message == WM_QUIT)
//                break;
//
//            TranslateMessage(&msg);
//            DispatchMessage(&msg);
//        }
//        else if (closeWindow) {
//            PostMessage(hwnd, WM_CLOSE, 0, 0);
//        }
//        else {
//            // run game code
//            // update the game logic
//            Update(cube1RotMat, cube1Position, cube1WorldMat, cameraViewMat,
//                cameraProjMat, cbvGPUAddress, frameIndex, cube2RotMat, cube2PositionOffset,
//                cube2WorldMat);
//            // execute the command queue (rendering the scene is the result of the gpu executing the command lists)
//            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = Render(commandList, commandAllocator,
//                commandQueue, fence, frameIndex, fenceValue, Running, swapChain,
//                pipelineStateObject, renderTargets, rtvDescriptorHeap, dsDescriptorHeap,
//                rtvDescriptorSize, fenceEvent, rootSignature, mainDescriptorHeap,
//                viewport, scissorRect, vertexBufferView, indexBufferView, constantBufferUploadHeaps,
//                numCubeIndices);
//            d3dResource = textureBuffer;
//        }
//
//    }
//}