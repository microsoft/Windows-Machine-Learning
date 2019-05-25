#include "pch.h"
#include <Windows.h>
#include "resource.h"
#include <winrt/windows.storage.streams.h>
#include <winrt/Windows.AI.MachineLearning.h>

using namespace winrt;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::AI::MachineLearning;

int main()
{
    init_apartment();

    // Find the resource
    const auto modelResource = FindResource(NULL, MAKEINTRESOURCE(IDR_SQUEEZENET_MODEL), DataFileTypeString);
    if (modelResource == nullptr)
    {
        printf("failed to find resource.");
        return 1;
    }

    // Load the resource
    const auto modelMem = LoadResource(NULL, modelResource);
    if (modelMem == nullptr)
    {
        printf("failed to load resource");
        return 2;
    }

    try
    {
        // get a byte point to the resource
        const unsigned char* pModelData = static_cast<const unsigned char*>(LockResource(modelMem));
        const auto size = SizeofResource(NULL, modelResource);

        // write the bytes into a stream
        InMemoryRandomAccessStream modelStream;
        DataWriter writer(modelStream);
        writer.WriteBytes(array_view<const unsigned char>(pModelData, pModelData + size));
        writer.StoreAsync().get();

        // clean up.
        FreeResource(modelMem);

        modelStream.Seek(0);

        // wrap the stream in a stream reference
        auto modelStreamReference = RandomAccessStreamReference::CreateFromStream(modelStream);

        // load the model from stream reference
        auto learningModel = LearningModel::LoadFromStream(modelStreamReference);

        // create the session binding
        auto learningModelSession = LearningModelSession(learningModel);
        auto learningModelBinding = LearningModelBinding(learningModelSession);

        // bind uninitialized data
        std::vector<int64_t> shape = { 1,3,224,224 };
        std::vector<float> inputData;
        inputData.resize(3 * 224 * 224);
        auto inputTensor = TensorFloat::CreateFromArray(shape, inputData);
        learningModelBinding.Bind(L"data_0", inputTensor);

        // evaluate
        auto results = learningModelSession.Evaluate(learningModelBinding, L"");
    }
    catch (...)
    {
        printf("caught an exception");
        return 3;
    }

    printf("success!");
    return 0;
}



//if (const HRSRC hResource = ::FindResource(hModule, MAKEINTRESOURCE(iRessourceID), L"DNN"))
//{
//    if (HGLOBAL hMem = ::LoadResource(hModule, hResource))
//    {
//        void* pData = ::LockResource(hMem);
//        const size_t iSize = ::SizeofResource(hModule, hResource);
//
//        ret = CNTK::Function::Load((const char*)pData, iSize);
//
//        ::FreeResource(hMem);
//    }
//}
