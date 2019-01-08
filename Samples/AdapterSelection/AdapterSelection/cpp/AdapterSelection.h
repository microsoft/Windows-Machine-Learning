#pragma once
#include <windows.h>
#include <winrt/Windows.AI.MachineLearning.h>
#include <dxgi.h>

class AdapterSelection
{
public:
    static std::vector<winrt::com_ptr<IDXGIAdapter1>> EnumerateAdapters(bool excludeSoftwareAdapter);
    static winrt::Windows::AI::MachineLearning::LearningModelDevice GetLearningModelDeviceFromAdapter(winrt::com_ptr<IDXGIAdapter1> spAdapter);
private:
    AdapterSelection() {}
};


