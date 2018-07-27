//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include <ppltasks.h>

namespace mnist_cppcx
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public ref class MainPage sealed
    {
    public:
        MainPage();

    private:
        void recognizeButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void clearButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

        static constexpr wchar_t* ModelFileName = L"model.onnx";
        coreml_MNISTModel^ m_model;
        ::Windows::Foundation::IAsyncOperation<::Windows::Media::VideoFrame^>^ GetHandWrittenImage();
        ::Windows::Foundation::IAsyncOperation<::Windows::Media::VideoFrame^>^ CropAndDisplayInputImageAsync(::Windows::Media::VideoFrame^ inputVideoFrame);
    };
}
