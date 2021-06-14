#include "pch.h"
#include "MelSpectrogram.h"
#include "FileHelper.h"
#include <winrt/Microsoft.AI.MachineLearning.Experimental.h>


using namespace winrt;

using namespace Microsoft::AI::MachineLearning::Experimental;
using namespace Microsoft::AI::MachineLearning;
static const wchar_t MS_EXPERIMENTAL_DOMAIN[] = L"com.microsoft.experimental";
using Operator = LearningModelOperator;

#define INT64(x) static_cast<int64_t>(x)
#define SIZET(x) static_cast<size_t>(x)
#define INT32(x) static_cast<int32_t>(x)



template <typename T>
static auto MakePureFrequency(float frequency_in_hertz, size_t signal_size, size_t sample_rate) {
    float amplitude = 4;
    float angular_velocity = frequency_in_hertz * 2 * 3.1415f;
    std::vector<T> signal(signal_size);
    for (size_t i = 0; i < signal_size; i++) {
        T time = i / static_cast<T>(sample_rate);
        signal[i] = amplitude * cos(angular_velocity * time);
    }
    return signal;
}

template <typename T>
static auto MakeMiddleC(size_t signal_size, size_t sample_rate) {
    float middle_c_in_hertz = 261.626f;
    return MakePureFrequency<T>(middle_c_in_hertz, signal_size, sample_rate);
}

template <typename T>
static auto MakeC2(size_t signal_size, size_t sample_rate) {
    float middle_c_in_hertz = 261.626f * 2;
    return MakePureFrequency<T>(middle_c_in_hertz, signal_size, sample_rate);
}

template <typename T>
static auto MakeC4(size_t signal_size, size_t sample_rate) {
    float middle_c_in_hertz = 261.626f * 4;
    return MakePureFrequency<T>(middle_c_in_hertz, signal_size, sample_rate);
}

template <typename T>
static auto MakeThreeTones(size_t signal_size, size_t sample_rate) {
    auto middle_c = MakeMiddleC<T>(signal_size, sample_rate);
    auto c2 = MakeC2<T>(signal_size, sample_rate);
    auto c4 = MakeC4<T>(signal_size, sample_rate);
    for (size_t i = 0; i < signal_size; i++) {
        middle_c[i] = (i < signal_size / 3) ?
            middle_c[i] :
            (i < 2 * signal_size / 3) ?
            (middle_c[i] + c2[i]) :
            (middle_c[i] + c2[i] + c4[i]);
    }
    return middle_c;
}

static void WindowFunction(const wchar_t* window_operator_name, TensorKind kind) {
    std::vector<int64_t> scalar_shape = {};
    std::vector<int64_t> output_shape = { 32 };
    auto double_data_type = TensorInt64Bit::CreateFromArray({}, { 11 });

    auto window_operator =
        Operator(window_operator_name, MS_EXPERIMENTAL_DOMAIN)
        .SetInput(L"size", L"Input")
        .SetOutput(L"output", L"Output");

    if (kind == TensorKind::Double) {
        window_operator.SetAttribute(L"output_datatype", double_data_type);
    }

    auto model =
        LearningModelBuilder::Create(13)
        .Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Input", TensorKind::Int64, scalar_shape))
        .Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output", kind, output_shape))
        .Operators().Add(window_operator)
        .CreateModel();

    LearningModelSession session(model);
    LearningModelBinding binding(session);

    binding.Bind(L"Input", TensorInt64Bit::CreateFromArray(scalar_shape, { 32 }));

    // Evaluate
    auto result = session.Evaluate(binding, L"");

    // Check results
    printf("Output\n");
    if (kind == TensorKind::Float) {
        auto y_tensor = result.Outputs().Lookup(L"Output").as<TensorFloat>();
        auto y_ivv = y_tensor.GetAsVectorView();
        for (int i = 0; i < output_shape[0]; i++) {
            printf("%f, ", y_ivv.GetAt(i));
        }
    }
    if (kind == TensorKind::Double) {
        auto y_tensor = result.Outputs().Lookup(L"Output").as<TensorDouble>();
        auto y_ivv = y_tensor.GetAsVectorView();
        for (int i = 0; i < output_shape[0]; i++) {
            printf("%f, ", y_ivv.GetAt(i));
        }
    }
    printf("\n");
}

static void ModelBuilding_HannWindow() {
    WindowFunction(L"HannWindow", TensorKind::Float);
    WindowFunction(L"HannWindow", TensorKind::Double);
}

static void STFT(size_t batch_size, size_t signal_size, size_t dft_size,
    size_t hop_size, size_t sample_rate, bool is_onesided = false) {
    auto n_dfts = static_cast<size_t>(1 + floor((signal_size - dft_size) / hop_size));
    auto input_shape = std::vector<int64_t>{ 1, INT64(signal_size) };
    auto output_shape =
        std::vector<int64_t>{
          INT64(batch_size),
          INT64(n_dfts),
          is_onesided ? ((INT64(dft_size) >> 1) + 1) : INT64(dft_size),
          2
    };
    auto dft_length = TensorInt64Bit::CreateFromArray({}, { INT64(dft_size) });

    auto model =
        LearningModelBuilder::Create(13)
        .Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Input.TimeSignal", TensorKind::Float, input_shape))
        .Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output.STFT", TensorKind::Float, output_shape))
        .Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output.HannWindow", TensorKind::Float, { INT64(dft_size) }))
        .Operators().Add(Operator(L"HannWindow", MS_EXPERIMENTAL_DOMAIN)
            .SetConstant(L"size", dft_length)
            .SetOutput(L"output", L"Output.HannWindow"))
        .Operators().Add(Operator(L"STFT", MS_EXPERIMENTAL_DOMAIN)
            .SetAttribute(L"onesided", TensorInt64Bit::CreateFromArray({}, { INT64(is_onesided) }))
            .SetInput(L"signal", L"Input.TimeSignal")
            .SetInput(L"window", L"Output.HannWindow")
            .SetConstant(L"frame_length", dft_length)
            .SetConstant(L"frame_step", TensorInt64Bit::CreateFromArray({}, { INT64(hop_size) }))
            .SetOutput(L"output", L"Output.STFT"))
        .CreateModel();

    LearningModelSession session(model);
    LearningModelBinding binding(session);

    // Create signal binding
    auto signal = MakeMiddleC<float>(signal_size, sample_rate);
    printf("\n");
    printf("Input.TimeSignal:\n");
    for (size_t i = 0; i < dft_size; i++) {
        printf("%f, ", signal[i]);
    }

    // Bind
    binding.Bind(L"Input.TimeSignal", TensorFloat::CreateFromArray(input_shape, signal));

    // Evaluate
    auto result = session.Evaluate(binding, L"");

    printf("\n");
    printf("Output.HannWindow\n");
    auto window_tensor = result.Outputs().Lookup(L"Output.HannWindow").as<TensorFloat>();
    auto window_ivv = window_tensor.GetAsVectorView();
    for (uint32_t i = 0; i < window_ivv.Size(); i++) {
        printf("%f, ", window_ivv.GetAt(i));
    }
    printf("\n");
    printf("Output.STFT\n");
    // Check results
    auto y_tensor = result.Outputs().Lookup(L"Output.STFT").as<TensorFloat>();
    auto y_ivv = y_tensor.GetAsVectorView();
    auto size = y_ivv.Size();
    //WINML_EXPECT_EQUAL(size, n_dfts * output_shape[2] * 2);
    for (size_t dft_idx = 0; dft_idx < n_dfts; dft_idx++) {
        for (size_t i = 0; INT64(i) < output_shape[2]; i++) {
            auto real_idx = static_cast<uint32_t>((i * 2) + (2 * dft_idx * output_shape[2]));
            printf("(%d, %f , %fi), ", static_cast<uint32_t>(i), y_ivv.GetAt(real_idx), y_ivv.GetAt(real_idx + 1));
        }
    }

    printf("\n");
}
static void ModelBuilding_MelWeightMatrix() {
    std::vector<int64_t> output_shape = { INT64(9), INT64(8) };
    auto builder =
        LearningModelBuilder::Create(13)
        .Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output.MelWeightMatrix", TensorKind::Float, output_shape))
        .Operators().Add(Operator(L"MelWeightMatrix", MS_EXPERIMENTAL_DOMAIN)
            .SetConstant(L"num_mel_bins", TensorInt64Bit::CreateFromArray({}, { INT64(8) }))
            .SetConstant(L"dft_length", TensorInt64Bit::CreateFromArray({}, { INT64(16) }))
            .SetConstant(L"sample_rate", TensorInt64Bit::CreateFromArray({}, { INT64(8192) }))
            .SetConstant(L"lower_edge_hertz", TensorFloat::CreateFromArray({}, { 0 }))
            .SetConstant(L"upper_edge_hertz", TensorFloat::CreateFromArray({}, { 8192 / 2.f }))
            .SetOutput(L"output", L"Output.MelWeightMatrix"));
    auto model = builder.CreateModel();

    LearningModelSession session(model);
    LearningModelBinding binding(session);

    auto result = session.Evaluate(binding, L"");

    printf("\n");
    printf("Output.MelWeightMatrix\n");
    {
        auto y_tensor = result.Outputs().Lookup(L"Output.MelWeightMatrix").as<TensorFloat>();
        auto y_ivv = y_tensor.GetAsVectorView();
        for (unsigned i = 0; i < y_ivv.Size(); i++) {
            printf("%f, ", y_ivv.GetAt(i));
        }
    }

    printf("\n");
}

void MelSpectrogram::MelSpectrogramOnThreeToneSignal(
    size_t batch_size, size_t signal_size, size_t window_size, size_t dft_size,
    size_t hop_size, size_t n_mel_bins, size_t sampling_rate) {
    auto n_dfts = static_cast<size_t>(1 + floor((signal_size - dft_size) / hop_size));
    auto onesided_dft_size = (dft_size >> 1) + 1;
    std::vector<int64_t> signal_shape = { INT64(batch_size), INT64(signal_size) };
    std::vector<int64_t> mel_spectrogram_shape = { INT64(batch_size), 1, INT64(n_dfts), INT64(n_mel_bins) };

    auto builder =
        LearningModelBuilder::Create(13)
        .Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Input.TimeSignal", L"One-dimensional audio waveform", TensorKind::Float, signal_shape))
        .Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output.MelSpectrogram", L"Output description", TensorKind::Float, mel_spectrogram_shape))
        .Operators().Add(Operator(L"HannWindow")
            .SetConstant(L"size", TensorInt64Bit::CreateFromArray({}, { INT64(window_size) }))
            .SetOutput(L"output", L"hann_window"))
        .Operators().Add(Operator(L"STFT")
            .SetName(L"STFT_NAMED_NODE")
            .SetInput(L"signal", L"Input.TimeSignal")
            .SetInput(L"window", L"hann_window")
            .SetConstant(L"frame_length", TensorInt64Bit::CreateFromArray({}, { INT64(dft_size) }))
            .SetConstant(L"frame_step", TensorInt64Bit::CreateFromArray({}, { INT64(hop_size) }))
            .SetOutput(L"output", L"stft_output"))
        .Operators().Add(Operator(L"ReduceSumSquare")
            .SetInput(L"data", L"stft_output")
            .SetAttribute(L"axes", TensorInt64Bit::CreateFromArray({ 1 }, { 3 }))
            .SetAttribute(L"keepdims", TensorInt64Bit::CreateFromArray({}, { 0 }))
            .SetOutput(L"reduced", L"magnitude_squared"))
        .Operators().Add(Operator(L"Div")
            .SetInput(L"A", L"magnitude_squared")
            .SetConstant(L"B", TensorFloat::CreateFromArray({}, { static_cast<float>(dft_size) }))
            .SetOutput(L"C", L"power_frames"))
        .Operators().Add(Operator(L"MelWeightMatrix")
            .SetConstant(L"num_mel_bins", TensorInt64Bit::CreateFromArray({}, { INT64(n_mel_bins) }))
            .SetConstant(L"dft_length", TensorInt64Bit::CreateFromArray({}, { INT64(dft_size) }))
            .SetConstant(L"sample_rate", TensorInt64Bit::CreateFromArray({}, { INT64(sampling_rate) }))
            .SetConstant(L"lower_edge_hertz", TensorFloat::CreateFromArray({}, { 0 }))
            .SetConstant(L"upper_edge_hertz", TensorFloat::CreateFromArray({}, { sampling_rate / 2.f }))
            .SetOutput(L"output", L"mel_weight_matrix"))
        .Operators().Add(Operator(L"Reshape")
            .SetInput(L"data", L"power_frames")
            .SetConstant(L"shape", TensorInt64Bit::CreateFromArray({ 2 }, { INT64(batch_size * n_dfts), INT64(onesided_dft_size) }))
            .SetOutput(L"reshaped", L"reshaped_output"))
        .Operators().Add(Operator(L"MatMul")
            .SetInput(L"A", L"reshaped_output")
            .SetInput(L"B", L"mel_weight_matrix")
            .SetOutput(L"Y", L"mel_spectrogram"))
        .Operators().Add(Operator(L"Reshape")
            .SetInput(L"data", L"mel_spectrogram")
            .SetConstant(L"shape", TensorInt64Bit::CreateFromArray({ 4 }, mel_spectrogram_shape))
            .SetOutput(L"reshaped", L"Output.MelSpectrogram"));
    auto model = builder.CreateModel();

    LearningModelSession session(model);
    LearningModelBinding binding(session);

    // Bind input
    auto signal = MakeThreeTones<float>(signal_size, sampling_rate);
    binding.Bind(L"Input.TimeSignal", TensorFloat::CreateFromArray(signal_shape, signal));

    // Bind output
    auto output_image =
        winrt::Windows::Media::VideoFrame(
            winrt::Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8,
            INT32(n_mel_bins),
            INT32(n_dfts));
    binding.Bind(L"Output.MelSpectrogram", output_image);

    // Evaluate
    auto start = std::chrono::high_resolution_clock::now();
    auto result = session.Evaluate(binding, L"");
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> evaluate_duration_in_microseconds = end - start;
    printf("Evaluate Took: %f\n", evaluate_duration_in_microseconds.count());

    // Check the output video frame object by saving output image to disk
    std::wstring out_name = L"mel_spectrogram.jpg";

    // Save the output
    std::wstring modulePath = FileHelper::GetModulePath();
    winrt::Windows::Storage::StorageFolder folder = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(modulePath).get();
    winrt::Windows::Storage::StorageFile file = folder.CreateFileAsync(out_name, winrt::Windows::Storage::CreationCollisionOption::ReplaceExisting).get();
    winrt::Windows::Storage::Streams::IRandomAccessStream write_stream = file.OpenAsync(winrt::Windows::Storage::FileAccessMode::ReadWrite).get();
    winrt::Windows::Graphics::Imaging::BitmapEncoder encoder = winrt::Windows::Graphics::Imaging::BitmapEncoder::CreateAsync(winrt::Windows::Graphics::Imaging::BitmapEncoder::JpegEncoderId(), write_stream).get();
    encoder.SetSoftwareBitmap(output_image.SoftwareBitmap());
    encoder.FlushAsync().get();

    // Save the model
    builder.Save(L"spectrogram.onnx");
    printf("\n");
}


