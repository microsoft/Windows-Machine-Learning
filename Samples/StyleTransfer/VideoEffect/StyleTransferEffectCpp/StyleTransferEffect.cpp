#include "pch.h"
#include "StyleTransferEffect.h"
#include "StyleTransferEffect.g.cpp"
#include <ppltasks.h>
#include <sstream>

using namespace std;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Streams;
using namespace concurrency;

namespace winrt::StyleTransferEffectCpp::implementation
{
    StyleTransferEffect::StyleTransferEffect() :
        Session(nullptr)
    {
    }

    IVectorView<VideoEncodingProperties> StyleTransferEffect::SupportedEncodingProperties() {
        VideoEncodingProperties props = VideoEncodingProperties();
        props.Subtype(L"ARGB32");
        return single_threaded_vector(std::vector<VideoEncodingProperties>{props}).GetView();
    }

    bool StyleTransferEffect::TimeIndependent() { return true; }
    MediaMemoryTypes StyleTransferEffect::SupportedMemoryTypes() { return MediaMemoryTypes::GpuAndCpu; }
    bool StyleTransferEffect::IsReadOnly() { return false; }
    void StyleTransferEffect::DiscardQueuedFrames() {}

    void StyleTransferEffect::Close(MediaEffectClosedReason) {
        OutputDebugString(L"Close Begin | ");
        std::lock_guard<mutex> guard{ Processing };
        // Make sure evalAsyncs are done before clearing resources
        for (int i = 0; i < swapChainEntryCount; i++) {
            if (bindings[i]->activetask != nullptr &&
                bindings[i]->binding != nullptr)
            {
                OutputDebugString(std::to_wstring(i).c_str());
                bindings[i]->activetask.get();
                bindings[i]->binding.Clear();
            }
        }
        if (Session != nullptr) Session.Close();
        OutputDebugString(L"Close\n");
    }

    void StyleTransferEffect::SubmitEval(VideoFrame input, VideoFrame output) {
        auto currentBinding = bindings[0].get();
        if (currentBinding->activetask == nullptr
            || currentBinding->activetask.Status() != Windows::Foundation::AsyncStatus::Started)
        {
            auto now = std::chrono::high_resolution_clock::now();
            OutputDebugString(L"PF Start new Eval ");
            OutputDebugString(std::to_wstring(swapChainIndex).c_str());
            OutputDebugString(L" | ");
            // submit an eval and wait for it to finish submitting work
            {
                std::lock_guard<mutex> guard{ Processing };
                currentBinding->binding.Bind(InputImageDescription, input);
                std::rotate(bindings.begin(), bindings.begin() + 1, bindings.end());
                finishedFrameIndex = (finishedFrameIndex - 1 + swapChainEntryCount) % swapChainEntryCount;
            }
            currentBinding->activetask = Session.EvaluateAsync(
                currentBinding->binding,
                std::to_wstring(swapChainIndex).c_str());
            currentBinding->activetask.Completed([&, currentBinding, now](auto&& asyncInfo, winrt::Windows::Foundation::AsyncStatus const) {
                OutputDebugString(L"PF Eval completed |");
                VideoFrame evalOutput = asyncInfo.GetResults()
                    .Outputs()
                    .Lookup(OutputImageDescription)
                    .try_as<VideoFrame>();
                int bindingIdx;
                bool finishedFrameUpdated;
                {
                    std::lock_guard<mutex> guard{ Processing };
                    auto binding = std::find_if(bindings.begin(),
                        bindings.end(),
                        [currentBinding](const auto& b)
                        {
                            return b.get() == currentBinding;
                        });
                    bindingIdx = std::distance(bindings.begin(), binding);
                    finishedFrameUpdated = bindingIdx >= finishedFrameIndex;
                    finishedFrameIndex = finishedFrameUpdated ? bindingIdx : finishedFrameIndex;
                }
                if (finishedFrameUpdated)
                {
                    OutputDebugString(L"PF Copy | ");
                    evalOutput.CopyToAsync(currentBinding->outputCache);
                }

                auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - now);
                // Convert to FPS: milli to seconds, invert
                Notifier.SetFrameRate(1000.f / timePassed.count());
                OutputDebugString(L"PF End ");
                });
        }
        if (bindings[finishedFrameIndex]->outputCache != nullptr) {
            OutputDebugString(L"\nStart CopyAsync ");
            OutputDebugString(std::to_wstring(finishedFrameIndex).c_str());
            {
                // Lock so that don't have multiple sources copying to output at once
                std::lock_guard<mutex> guard{ Processing };
                bindings[finishedFrameIndex]->outputCache.CopyToAsync(output).get();
            }
            OutputDebugString(L" | Stop CopyAsync\n");
        }
        // return without waiting for the submit to finish, setup the completion handler
    }

    void StyleTransferEffect::ProcessFrame(ProcessVideoFrameContext context) {
        OutputDebugString(L"PF Start | ");
        VideoFrame inputFrame = context.InputFrame();
        VideoFrame outputFrame = context.OutputFrame();

        SubmitEval(inputFrame, outputFrame);

        swapChainIndex = (++swapChainIndex) % swapChainEntryCount;
        // move on to the next entry after each call to PF. 
    }

    void StyleTransferEffect::SetEncodingProperties(
        VideoEncodingProperties props,
        IDirect3DDevice device) {
        encodingProperties = props;
    }

    void StyleTransferEffect::SetProperties(IPropertySet config) {
        this->configuration = config;
        hstring modelName;
        bool useGpu;

        IInspectable val = config.TryLookup(L"ModelName");
        if (val)
        {
            modelName = unbox_value<hstring>(val);
        }
        else
        {
            winrt::throw_hresult(E_FAIL);
        }

        val = configuration.TryLookup(L"UseGpu");
        if (val)
        {
            useGpu = unbox_value<bool>(val);
        }
        else
        {
            winrt::throw_hresult(E_FAIL);
        }

        val = configuration.TryLookup(L"Notifier");
        if (val)
        {
            Notifier = val.try_as<StyleTransferEffectNotifier>();
        }
        else
        {
            winrt::throw_hresult(E_FAIL);
        }

        val = configuration.TryLookup(L"NumThreads");
        if (val)
        {
            swapChainEntryCount = unbox_value<int>(val);
        }
        else
        {
            winrt::throw_hresult(E_FAIL);
        }

        OutputDebugString(L"Num Threads: ");
        OutputDebugString(std::to_wstring(swapChainEntryCount).c_str());
        LearningModel _model = LearningModel::LoadFromFilePath(modelName);
        LearningModelDeviceKind _device = useGpu ? LearningModelDeviceKind::DirectX
            : LearningModelDeviceKind::Cpu;
        Session = LearningModelSession{ _model, LearningModelDevice(_device) };

        InputImageDescription = L"inputImage";
        OutputImageDescription = L"outputImage";

        // Create set of bindings to cycle through
        for (int i = 0; i < swapChainEntryCount; i++) {
            bindings.push_back(std::make_unique<SwapChainEntry>());
            bindings[i]->binding = LearningModelBinding(Session);
            bindings[i]->binding.Bind(OutputImageDescription,
                VideoFrame(Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8, 720, 720));
        }
    }
}
