//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace mnist_cppcx;

using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Storage;
using namespace Concurrency;
// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media;
using namespace Windows::AI::MachineLearning;
using namespace Platform;
using namespace concurrency;

MainPage::MainPage()
{
    InitializeComponent();

    // Set supported inking device types.
    inkCanvas->InkPresenter->InputDeviceTypes = Windows::UI::Core::CoreInputDeviceTypes::Mouse | Windows::UI::Core::CoreInputDeviceTypes::Pen;
    Windows::UI::Input::Inking::InkDrawingAttributes^ attributes = ref new Windows::UI::Input::Inking::InkDrawingAttributes();
    attributes->Color = Windows::UI::Colors::White;
    attributes->Size = Size(22, 22);
    attributes->IgnorePressure = true;
    attributes->IgnoreTilt = true;
    inkCanvas->InkPresenter->UpdateDefaultDrawingAttributes(attributes);

    // Initialize model
    std::wstring fullModelName(L"ms-appx:///Assets/");
    fullModelName += ModelFileName;
    create_task(StorageFile::GetFileFromApplicationUriAsync(ref new Uri(ref new Platform::String(fullModelName.c_str()))))
        .then([](StorageFile^ file) {
            return create_task(coreml_MNISTModel::CreateFromStreamAsync(file)); })
        .then([this](coreml_MNISTModel ^model) {
            this->m_model = model;
        });
}


void mnist_cppcx::MainPage::recognizeButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    coreml_MNISTInput^ input = ref new coreml_MNISTInput();
    create_task(GetHandWrittenImage())
    .then([this, input](VideoFrame^ vf) {
        input->image = ImageFeatureValue::CreateFromVideoFrame(vf);
        return create_task(this->m_model->EvaluateAsync(input)); })
    .then([this](coreml_MNISTOutput^ output) {
        float maxProb = 0;
        int maxKey = 0;
        IMapView<int64_t, float>^ map = output->prediction->GetAt(0)->GetView();
        for (const auto& pair : map)
        {
            if (pair->Value > maxProb)
            {
                maxProb = pair->Value;
                maxKey = (int64_t)pair->Key;
            }
        }
        numberLabel->Text = maxKey.ToString();
    });
}

void mnist_cppcx::MainPage::clearButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    inkCanvas->InkPresenter->StrokeContainer->Clear();
    numberLabel->Text = "";
}

::Windows::Foundation::IAsyncOperation<VideoFrame^>^ MainPage::GetHandWrittenImage()
{
    return create_async([this] {
        Imaging::RenderTargetBitmap ^ renderBitmap = ref new Imaging::RenderTargetBitmap();
        return create_task(renderBitmap->RenderAsync(inkGrid))
            .then([renderBitmap]() { return renderBitmap->GetPixelsAsync(); })
            .then([this, renderBitmap](IBuffer ^ buffer) {
            SoftwareBitmap ^ softwareBitMap = SoftwareBitmap::CreateCopyFromBuffer(
                buffer, BitmapPixelFormat::Rgba8, renderBitmap->PixelWidth,
                renderBitmap->PixelHeight, BitmapAlphaMode::Ignore);
            this->secondImage->Source = renderBitmap;
            VideoFrame ^ vf = VideoFrame::CreateWithSoftwareBitmap(softwareBitMap);
            return create_task(CropAndDisplayInputImageAsync(vf)); })
            .then([](VideoFrame^ cropped_vf) {
                return cropped_vf;
            });
    });
}

::Windows::Foundation::IAsyncOperation<VideoFrame^>^ MainPage::CropAndDisplayInputImageAsync(VideoFrame^ inputVideoFrame)
{
    return create_async([inputVideoFrame]() {
        bool useDX = inputVideoFrame->SoftwareBitmap == nullptr;

        unsigned int h = 28;
        unsigned int w = 28;
        unsigned int frameHeight = (unsigned int)useDX
            ? inputVideoFrame->Direct3DSurface->Description.Height
            : inputVideoFrame->SoftwareBitmap->PixelHeight;
        unsigned int frameWidth = (unsigned int)useDX
            ? inputVideoFrame->Direct3DSurface->Description.Width
            : inputVideoFrame->SoftwareBitmap->PixelWidth;

        float requiredAR = (float)w / (float)h;
        w = (unsigned int) min(requiredAR * frameHeight, frameWidth);
        h = (unsigned int) min(frameWidth / requiredAR, frameHeight);

        ::Windows::Graphics::Imaging::BitmapBounds cropBounds = ::Windows::Graphics::Imaging::BitmapBounds();
        cropBounds.X = (unsigned int)(frameWidth - w) / 2;
        cropBounds.Y = 0;
        cropBounds.Width = w;
        cropBounds.Height = h;

        VideoFrame^ cropped_vf = ref new VideoFrame(BitmapPixelFormat::Rgba8, w, h, BitmapAlphaMode::Ignore);

        inputVideoFrame->CopyToAsync(cropped_vf, cropBounds, nullptr);
        return cropped_vf;
    });
}
