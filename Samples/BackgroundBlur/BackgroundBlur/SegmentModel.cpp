#include "SegmentModel.h"

#include <iostream>
#include <filesystem>

using winrt::Windows::Foundation::PropertyValue;
using winrt::hstring;
using namespace winrt;
using namespace Windows::Foundation::Collections;

int g_scale = 5;
enum OnnxDataType : long {
	ONNX_UNDEFINED = 0,
	// Basic types.
	ONNX_FLOAT = 1,
	ONNX_UINT8 = 2,
	ONNX_INT8 = 3,
	ONNX_UINT16 = 4,
	ONNX_INT16 = 5,
	ONNX_INT32 = 6,
	ONNX_INT64 = 7,
	ONNX_STRING = 8,
	ONNX_BOOL = 9,

	// IEEE754 half-precision floating-point format (16 bits wide).
	// This format has 1 sign bit, 5 exponent bits, and 10 mantissa bits.
	ONNX_FLOAT16 = 10,

	ONNX_DOUBLE = 11,
	ONNX_UINT32 = 12,
	ONNX_UINT64 = 13,
	ONNX_COMPLEX64 = 14,     // complex with float32 real and imaginary components
	ONNX_COMPLEX128 = 15,    // complex with float64 real and imaginary components

	// Non-IEEE floating-point format based on IEEE754 single-precision
	// floating-point number truncated to 16 bits.
	// This format has 1 sign bit, 8 exponent bits, and 7 mantissa bits.
	ONNX_BFLOAT16 = 16,
}OnnxDataType;

interface DECLSPEC_UUID("9f251514-9d4d-4902-9d60-18988ab7d4b5") DECLSPEC_NOVTABLE
	IDXGraphicsAnalysis : public IUnknown
{

	STDMETHOD_(void, BeginCapture)() PURE;

	STDMETHOD_(void, EndCapture)() PURE;

}; 
IDXGraphicsAnalysis* pGraphicsAnalysis;


// TODO: Probably don't need to be globals
std::array<float, 3> mean = { 0.485f, 0.456f, 0.406f };
std::array<float, 3> stddev = { 0.229f, 0.224f, 0.225f };
auto outputBindProperties = PropertySet();

/****	Style transfer model	****/
void StyleTransfer::SetModels(int w, int h)
{
	// TODO: Use w/h or use the 720x720 of the mode
	SetImageSize(720, 720); // SIze model input sizes fixed to 720x720
	m_session = CreateLearningModelSession(GetModel());
	m_binding = LearningModelBinding(m_session);
}
void StyleTransfer::Run(IDirect3DSurface src, IDirect3DSurface dest)
{
	assert(m_session.Device().AdapterId() == nvidia);
	VideoFrame inVideoFrame = VideoFrame::CreateWithDirect3D11Surface(src);
	VideoFrame outVideoFrame = VideoFrame::CreateWithDirect3D11Surface(dest);
	SetVideoFrames(inVideoFrame, outVideoFrame);

	hstring inputName = m_session.Model().InputFeatures().GetAt(0).Name();
	m_binding.Bind(inputName, m_inputVideoFrame);
	hstring outputName = m_session.Model().OutputFeatures().GetAt(0).Name();

	auto outputBindProperties = PropertySet();
	outputBindProperties.Insert(L"DisableTensorCpuSync", PropertyValue::CreateBoolean(true));

	m_binding.Bind(outputName, m_outputVideoFrame, outputBindProperties); // TODO: See if can bind videoframe from MFT
	auto results = m_session.Evaluate(m_binding, L"");

	m_outputVideoFrame.CopyToAsync(outVideoFrame).get();
}
LearningModel StyleTransfer::GetModel()
{
	auto rel = std::filesystem::current_path();
	rel.append("Assets\\mosaic.onnx");
	return LearningModel::LoadFromFilePath(rel + L"");
}

/****	Background blur model	****/
void BackgroundBlur::SetModels(int w, int h)
{
	w /= g_scale; h /= g_scale;
	SetImageSize(w, h);

	HRESULT getAnalysis = DXGIGetDebugInterface1(0, __uuidof(pGraphicsAnalysis), reinterpret_cast<void**>(&pGraphicsAnalysis));

	m_sessionPreprocess = CreateLearningModelSession(Normalize0_1ThenZScore(h, w, 3, mean, stddev));
	m_sessionPostprocess = CreateLearningModelSession(PostProcess(1, 3, h, w, 1));
	// Named dim override of FCN-Resnet so that unlock optimizations of fixed input size
	auto fcnDevice = m_bUseGPU ? LearningModelDevice(LearningModelDeviceKind::DirectXHighPerformance) : LearningModelDevice(LearningModelDeviceKind::Default); // Todo: Have a toggle between GPU/ CPU? 
	auto model = GetModel();
	auto options = LearningModelSessionOptions();
	options.BatchSizeOverride(0);
	options.CloseModelOnSessionCreation(true);
	// ****** TODO: Input name vs. dimension name? *****
	// Because input name is "input" but I want to set dim 2 & 3 of that input 
	options.OverrideNamedDimension(L"height", m_imageHeightInPixels);
	options.OverrideNamedDimension(L"width", m_imageWidthInPixels);
	m_session = LearningModelSession(model, fcnDevice, options);

	m_bindingPreprocess = LearningModelBinding(m_sessionPreprocess);
	m_binding = LearningModelBinding(m_session);
	m_bindingPostprocess = LearningModelBinding(m_sessionPostprocess);
}
LearningModel BackgroundBlur::GetModel()
{
	auto rel = std::filesystem::current_path();
	rel.append("Assets\\fcn-resnet50-12-int8.onnx");
	return LearningModel::LoadFromFilePath(rel + L"");
}
void BackgroundBlur::Run(IDirect3DSurface src, IDirect3DSurface dest)
{
	pGraphicsAnalysis->BeginCapture();
	assert(m_session.Device().AdapterId() == nvidia);
	VideoFrame inVideoFrame = VideoFrame::CreateWithDirect3D11Surface(src);
	VideoFrame outVideoFrame = VideoFrame::CreateWithDirect3D11Surface(dest);
	SetVideoFrames(inVideoFrame, outVideoFrame);

	// Shape validation
	assert((UINT32)m_inputVideoFrame.Direct3DSurface().Description().Height == m_imageHeightInPixels);
	assert((UINT32)m_inputVideoFrame.Direct3DSurface().Description().Width == m_imageWidthInPixels);

	assert(m_sessionPreprocess.Device().AdapterId() == nvidia);
	assert(m_sessionPostprocess.Device().AdapterId() == nvidia);

	// 2. Preprocessing: z-score normalization 
	std::vector<int64_t> shape = { 1, 3, m_imageHeightInPixels, m_imageWidthInPixels };
	ITensor intermediateTensor = TensorFloat::Create(shape);
	hstring inputName = m_sessionPreprocess.Model().InputFeatures().GetAt(0).Name();
	hstring outputName = m_sessionPreprocess.Model().OutputFeatures().GetAt(0).Name();

	m_bindingPreprocess.Bind(inputName, m_inputVideoFrame);
	outputBindProperties.Insert(L"DisableTensorCpuSync", PropertyValue::CreateBoolean(true));
	m_bindingPreprocess.Bind(outputName, intermediateTensor, outputBindProperties);
	m_sessionPreprocess.EvaluateAsync(m_bindingPreprocess, L"");

	// 3. Run through actual model
	std::vector<int64_t> FCNResnetOutputShape = { 1, 21, m_imageHeightInPixels, m_imageWidthInPixels };
	ITensor FCNResnetOutput = TensorFloat::Create(FCNResnetOutputShape);

	m_binding.Bind(m_session.Model().InputFeatures().GetAt(0).Name(), intermediateTensor);
	m_binding.Bind(m_session.Model().OutputFeatures().GetAt(0).Name(), FCNResnetOutput, outputBindProperties);
	m_session.EvaluateAsync(m_binding, L"");

	// Shape validation 
	assert(m_outputVideoFrame.Direct3DSurface().Description().Height == m_imageHeightInPixels);
	assert(m_outputVideoFrame.Direct3DSurface().Description().Width == m_imageWidthInPixels);

	// 4. Postprocessing
	outputBindProperties.Insert(L"DisableTensorCpuSync", PropertyValue::CreateBoolean(false));
	m_bindingPostprocess.Bind(m_sessionPostprocess.Model().InputFeatures().GetAt(0).Name(), m_inputVideoFrame); // InputImage
	m_bindingPostprocess.Bind(m_sessionPostprocess.Model().InputFeatures().GetAt(1).Name(), FCNResnetOutput); // InputScores
	m_bindingPostprocess.Bind(m_sessionPostprocess.Model().OutputFeatures().GetAt(0).Name(), m_outputVideoFrame);
	// TODO: Make this async as well, and add a completed 
	m_sessionPostprocess.EvaluateAsync(m_bindingPostprocess, L"").get();
	m_outputVideoFrame.CopyToAsync(outVideoFrame).get();
	pGraphicsAnalysis->EndCapture();
}

winrt::Windows::Foundation::IAsyncOperation<LearningModelEvaluationResult> BackgroundBlur::RunAsync()
{

	// Shape validation
	assert((UINT32)m_inputVideoFrame.Direct3DSurface().Description().Height == m_imageHeightInPixels);
	assert((UINT32)m_inputVideoFrame.Direct3DSurface().Description().Width == m_imageWidthInPixels);

	assert(m_sessionPreprocess.Device().AdapterId() == nvidia);
	assert(m_sessionPostprocess.Device().AdapterId() == nvidia);

	// 2. Preprocessing: z-score normalization 
	std::vector<int64_t> shape = { 1, 3, m_imageHeightInPixels, m_imageWidthInPixels };
	ITensor intermediateTensor = TensorFloat::Create(shape);
	hstring inputName = m_sessionPreprocess.Model().InputFeatures().GetAt(0).Name();
	hstring outputName = m_sessionPreprocess.Model().OutputFeatures().GetAt(0).Name();

	m_bindingPreprocess.Bind(inputName, m_inputVideoFrame);
	outputBindProperties.Insert(L"DisableTensorCpuSync", PropertyValue::CreateBoolean(true));
	m_bindingPreprocess.Bind(outputName, intermediateTensor, outputBindProperties);
	m_sessionPreprocess.EvaluateAsync(m_bindingPreprocess, L""); 

	// 3. Run through actual model
	std::vector<int64_t> FCNResnetOutputShape = { 1, 21, m_imageHeightInPixels, m_imageWidthInPixels };
	ITensor FCNResnetOutput = TensorFloat::Create(FCNResnetOutputShape);

	m_binding.Bind(m_session.Model().InputFeatures().GetAt(0).Name(), intermediateTensor);
	m_binding.Bind(m_session.Model().OutputFeatures().GetAt(0).Name(), FCNResnetOutput, outputBindProperties);
	m_session.EvaluateAsync(m_binding, L""); 

	// Shape validation 
	assert(m_outputVideoFrame.Direct3DSurface().Description().Height == m_imageHeightInPixels);
	assert(m_outputVideoFrame.Direct3DSurface().Description().Width == m_imageWidthInPixels);

	// 4. Postprocessing
	outputBindProperties.Insert(L"DisableTensorCpuSync", PropertyValue::CreateBoolean(true));
	m_bindingPostprocess.Bind(m_sessionPostprocess.Model().InputFeatures().GetAt(0).Name(), m_inputVideoFrame); // InputImage
	m_bindingPostprocess.Bind(m_sessionPostprocess.Model().InputFeatures().GetAt(1).Name(), FCNResnetOutput); // InputScores
	m_bindingPostprocess.Bind(m_sessionPostprocess.Model().OutputFeatures().GetAt(0).Name(), m_outputVideoFrame);
	// TODO: Make this async as well, and add a completed 
	return m_sessionPostprocess.EvaluateAsync(m_bindingPostprocess, L"");
}

LearningModel BackgroundBlur::PostProcess(long n, long c, long h, long w, long axis)
{
	auto builder = LearningModelBuilder::Create(12)
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"InputImage", TensorKind::Float, { n, c, h, w }))
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"InputScores", TensorKind::Float, { -1, -1, h, w })) // Different input type? 
		.Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"OutputImage", TensorKind::Float, { n, c, h, w }))
		// Argmax Model Outputs
		.Operators().Add(LearningModelOperator(L"ArgMax")
			.SetInput(L"data", L"InputScores")
			.SetAttribute(L"keepdims", TensorInt64Bit::CreateFromArray({ 1 }, { 1 }))
			.SetAttribute(L"axis", TensorInt64Bit::CreateFromIterable({ 1 }, { axis })) // Correct way of passing axis? 
			.SetOutput(L"reduced", L"Reduced"))
		.Operators().Add(LearningModelOperator(L"Cast")
			.SetInput(L"input", L"Reduced")
			.SetAttribute(L"to", TensorInt64Bit::CreateFromIterable({}, { OnnxDataType::ONNX_FLOAT }))
			.SetOutput(L"output", L"ArgmaxOutput"))
		// Extract the foreground using the argmax scores to create a mask
		.Operators().Add(LearningModelOperator(L"Clip")
			.SetInput(L"input", L"ArgmaxOutput")
			.SetConstant(L"max", TensorFloat::CreateFromIterable({ 1 }, { 1.f }))
			.SetOutput(L"output", L"MaskBinary"))
		.Operators().Add(LearningModelOperator(L"Mul")
			.SetInput(L"A", L"InputImage")
			.SetInput(L"B", L"MaskBinary")
			.SetOutput(L"C", L"ForegroundImage"))

		// Extract the blurred background using the negation of the foreground mask
		.Operators().Add(LearningModelOperator(L"AveragePool") // AveragePool to create blurred background
			.SetInput(L"X", L"InputImage")
			.SetAttribute(L"kernel_shape", TensorInt64Bit::CreateFromArray(std::vector<int64_t>{2}, std::array<int64_t, 2>{20, 20}))
			.SetAttribute(L"auto_pad", TensorString::CreateFromArray(std::vector<int64_t>{1}, std::array<hstring, 1>{L"SAME_UPPER"}))
			.SetOutput(L"Y", L"BlurredImage"))
		.Operators().Add(LearningModelOperator(L"Mul")
			.SetInput(L"A", L"MaskBinary")
			.SetConstant(L"B", TensorFloat::CreateFromIterable({ 1 }, { -1.f }))
			.SetOutput(L"C", L"NegMask"))
		.Operators().Add(LearningModelOperator(L"Add") // BackgroundMask = (1- foreground Mask)
			.SetConstant(L"A", TensorFloat::CreateFromIterable({ 1 }, { 1.f }))
			.SetInput(L"B", L"NegMask")
			.SetOutput(L"C", L"BackgroundMask"))
		.Operators().Add(LearningModelOperator(L"Mul") // Extract the blurred background
			.SetInput(L"A", L"BlurredImage")
			.SetInput(L"B", L"BackgroundMask")
			.SetOutput(L"C", L"BackgroundImage"))

		// Combine foreground and background
		.Operators().Add(LearningModelOperator(L"Add")
			.SetInput(L"A", L"ForegroundImage")
			.SetInput(L"B", L"BackgroundImage")
			.SetOutput(L"C", L"OutputImage"))
		;

	return builder.CreateModel();
}



LearningModel Invert(long n, long c, long h, long w)
{
	
	auto builder = LearningModelBuilder::Create(11)
		// Loading in buffers and reshape
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Input", TensorKind::Float, { n, c, h, w }))
		.Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output", TensorKind::Float, { n, c, h, w }))
		.Operators().Add(LearningModelOperator(L"Mul")
			.SetInput(L"A", L"Input")
			.SetConstant(L"B", TensorFloat::CreateFromIterable({ 1 }, { -1.f }))
			//.SetConstant(L"B", TensorFloat::CreateFromIterable({3}, {0.114f, 0.587f, 0.299f}))
			.SetOutput(L"C", L"MulOutput")
		)
		.Operators().Add(LearningModelOperator(L"Add")
			.SetConstant(L"A", TensorFloat::CreateFromIterable({ 1 }, { 255.f }))
			.SetInput(L"B", L"MulOutput")
			.SetOutput(L"C", L"Output")
		)
		;

	return builder.CreateModel();
}

LearningModel Normalize0_1ThenZScore(long h, long w, long c, const std::array<float, 3>& means, const std::array<float, 3>& stddev)
{
	assert(means.size() == c);
	assert(stddev.size() == c);

	auto builder = LearningModelBuilder::Create(12)
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Input", L"The NCHW image", TensorKind::Float, {1, c, h, w}))
		.Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output", L"The NCHW image normalized with mean and stddev.", TensorKind::Float, {1, c, h, w}))
		.Operators().Add(LearningModelOperator(L"Div") // Normalize from 0-255 to 0-1 by dividing by 255
			.SetInput(L"A", L"Input")
			.SetConstant(L"B", TensorFloat::CreateFromArray({}, std::array<const float,1>{ 255.f }))
			.SetOutput(L"C", L"DivOutput"))
		.Operators().Add(LearningModelOperator(L"Reshape")
			.SetConstant(L"data", TensorFloat::CreateFromArray({ c }, means))
			.SetConstant(L"shape", TensorInt64Bit::CreateFromIterable({ 4 }, { 1, c, 1, 1 }))
			.SetOutput(L"reshaped", L"MeansReshaped"))
		.Operators().Add(LearningModelOperator(L"Reshape")
			.SetConstant(L"data", TensorFloat::CreateFromArray({ c }, stddev))
			.SetConstant(L"shape", TensorInt64Bit::CreateFromIterable({ 4 }, { 1, c, 1, 1 }))
			.SetOutput(L"reshaped", L"StdDevReshaped"))
		.Operators().Add(LearningModelOperator(L"Sub") // Shift by the means
			.SetInput(L"A", L"DivOutput")
			.SetInput(L"B", L"MeansReshaped")
			.SetOutput(L"C", L"SubOutput"))
		.Operators().Add(LearningModelOperator(L"Div")  // Divide by stddev
			.SetInput(L"A", L"SubOutput")
			.SetInput(L"B", L"StdDevReshaped")
			.SetOutput(L"C", L"Output"));
	return builder.CreateModel();
}

LearningModel ReshapeFlatBufferToNCHW(long n, long c, long h, long w)
{
	auto builder = LearningModelBuilder::Create(11)
		// Loading in buffers and reshape
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Input", TensorKind::UInt8, { 1, n * c * h * w }))
		.Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output", TensorKind::Float, {n, c, h, w}))
		.Operators().Add(LearningModelOperator((L"Cast"))
			.SetInput(L"input", L"Input")
			.SetOutput(L"output", L"SliceOutput")
			.SetAttribute(L"to",
				TensorInt64Bit::CreateFromIterable({}, {OnnxDataType::ONNX_FLOAT})))
		.Operators().Add(LearningModelOperator(L"Reshape")
			.SetInput(L"data", L"SliceOutput")
			.SetConstant(L"shape", TensorInt64Bit::CreateFromIterable({4}, {n, h, w, c}))
			.SetOutput(L"reshaped", L"ReshapeOutput"))
		.Operators().Add(LearningModelOperator(L"Transpose")
			.SetInput(L"data", L"ReshapeOutput")
			.SetAttribute(L"perm", TensorInt64Bit::CreateFromArray({ 4 }, { 0, 3, 1, 2 }))
			.SetOutput(L"transposed", L"Output"))
	;
	return builder.CreateModel();
}

