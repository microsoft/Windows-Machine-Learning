#include "SegmentModel.h"

#include <iostream>
#include <filesystem>

using winrt::Windows::Foundation::PropertyValue;
using winrt::hstring;
using namespace winrt;
using namespace Windows::Foundation::Collections;

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


int g_scale = 5;
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
	m_bSyncStarted = TRUE;

	assert(m_session.Device().AdapterId() == m_highPerfAdapter);
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

	m_bSyncStarted = FALSE;
	m_canRunEval.notify_one();
}
VideoFrame StyleTransfer::RunAsync(IDirect3DSurface src, IDirect3DSurface dest)
{
	// TODO: Implement async StyleTransfer
	//m_evalStatus = NULL;
	return NULL;
}
LearningModel StyleTransfer::GetModel()
{
	auto rel = std::filesystem::current_path();
	rel.append("Assets\\mosaic.onnx");
	return LearningModel::LoadFromFilePath(rel + L"");
}

BackgroundBlur::~BackgroundBlur() {
	if (m_sessionFused) m_sessionFused.Close();
	if (m_sessionPostprocess) m_sessionPostprocess.Close();
	if (m_sessionPreprocess) m_sessionPreprocess.Close();
	//if (m_bindFused) m_bindFused.Clear();
}
/****	Background blur model	****/
void BackgroundBlur::SetModels(int w, int h)
{
	w /= g_scale; h /= g_scale;
	SetImageSize(w, h);

	m_sessionPreprocess = CreateLearningModelSession(Normalize0_1ThenZScore(h, w, 3, mean, stddev));
	m_sessionPostprocess = CreateLearningModelSession(PostProcess(1, 3, h, w, 1));
	// Named dim override of FCN-Resnet so that unlock optimizations of fixed input size
	auto fcnDevice = m_bUseGPU ? LearningModelDevice(LearningModelDeviceKind::DirectXHighPerformance) : LearningModelDevice(LearningModelDeviceKind::Default); // Todo: Have a toggle between GPU/ CPU? 
	auto model = GetModel();
	auto FCNInputName = model.OutputFeatures().GetAt(0).Name();
	auto options = LearningModelSessionOptions();
	options.BatchSizeOverride(0);
	options.CloseModelOnSessionCreation(true);
	options.OverrideNamedDimension(L"height", m_imageHeightInPixels);
	options.OverrideNamedDimension(L"width", m_imageWidthInPixels);
	m_session = LearningModelSession(model, fcnDevice, options);

	auto joinOptions1 = LearningModelJoinOptions();
	joinOptions1.CloseModelOnJoin(true);
	joinOptions1.Link(L"Output", L"input");
	joinOptions1.JoinedNodePrefix(L"FCN_");
	joinOptions1.PromoteUnlinkedOutputsToFusedOutputs(true);
	auto modelExperimental = LearningModelExperimental(Normalize0_1ThenZScore(h, w, 3, mean, stddev));
	LearningModel stageOne = modelExperimental.JoinModel(GetModel(), joinOptions1);

	auto joinOptions2 = LearningModelJoinOptions();
	joinOptions2.CloseModelOnJoin(true);
	joinOptions2.Link(L"FCN_out", L"InputScores");
	joinOptions2.Link(L"OutputImageForward", L"InputImage");
	joinOptions2.JoinedNodePrefix(L"Post_");
	//joinOptions2.PromoteUnlinkedOutputsToFusedOutputs(false); // Causes winrt originate error in FusedGraphKernel.cpp, but works on CPU
	auto modelTwo = LearningModelExperimental(stageOne);
	LearningModel modelFused = modelTwo.JoinModel(PostProcess(1, 3, h, w, 1), joinOptions2);

	modelTwo.Save(L"modelTwo.onnx");

	m_sessionFused = CreateLearningModelSession(modelFused);
	m_bindFused = LearningModelBinding(m_sessionFused);

	m_bindingPreprocess = LearningModelBinding(m_sessionPreprocess);
	m_binding = LearningModelBinding(m_session);
	m_bindingPostprocess = LearningModelBinding(m_sessionPostprocess);
}
LearningModel BackgroundBlur::GetModel()
{
	auto rel = std::filesystem::current_path();
	rel.append("Assets\\fcn-resnet50-12.onnx");
	return LearningModel::LoadFromFilePath(rel + L"");
}

void BackgroundBlur::Run(IDirect3DSurface src, IDirect3DSurface dest)
{
	m_bSyncStarted = TRUE;
	assert(m_session.Device().AdapterId() == m_highPerfAdapter);
	VideoFrame inVideoFrame = VideoFrame::CreateWithDirect3D11Surface(src);
	VideoFrame outVideoFrame = VideoFrame::CreateWithDirect3D11Surface(dest);
	SetVideoFrames(inVideoFrame, outVideoFrame);

	// Shape validation
	assert((UINT32)m_inputVideoFrame.Direct3DSurface().Description().Height == m_imageHeightInPixels);
	assert((UINT32)m_inputVideoFrame.Direct3DSurface().Description().Width == m_imageWidthInPixels);

	hstring inputName = m_sessionFused.Model().InputFeatures().GetAt(0).Name();
	hstring outputName = m_sessionFused.Model().OutputFeatures().GetAt(1).Name();

	m_bindFused.Bind(inputName, m_inputVideoFrame);
	m_bindFused.Bind(outputName, m_outputVideoFrame);
	auto results = m_sessionFused.Evaluate(m_bindFused, L"");
	m_outputVideoFrame.CopyToAsync(outVideoFrame).get();
	m_bSyncStarted = FALSE;
	m_canRunEval.notify_one();
}

//void BackgroundBlur::Run(IDirect3DSurface src, IDirect3DSurface dest)
//{
//	m_bSyncStarted = TRUE;
//	assert(m_session.Device().AdapterId() == m_highPerfAdapter);
//	VideoFrame inVideoFrame = VideoFrame::CreateWithDirect3D11Surface(src);
//	VideoFrame outVideoFrame = VideoFrame::CreateWithDirect3D11Surface(dest);
//	SetVideoFrames(inVideoFrame, outVideoFrame);
//
//	// Shape validation
//	assert((UINT32)m_inputVideoFrame.Direct3DSurface().Description().Height == m_imageHeightInPixels);
//	assert((UINT32)m_inputVideoFrame.Direct3DSurface().Description().Width == m_imageWidthInPixels);
//
//	assert(m_sessionPreprocess.Device().AdapterId() == m_highPerfAdapter);
//	assert(m_sessionPostprocess.Device().AdapterId() == m_highPerfAdapter);
//
//	// 2. Preprocessing: z-score normalization 
//	std::vector<int64_t> shape = { 1, 3, m_imageHeightInPixels, m_imageWidthInPixels };
//	ITensor intermediateTensor = TensorFloat::Create(shape);
//	hstring inputName = m_sessionPreprocess.Model().InputFeatures().GetAt(0).Name();
//	hstring outputName = m_sessionPreprocess.Model().OutputFeatures().GetAt(0).Name();
//
//	m_bindingPreprocess.Bind(inputName, m_inputVideoFrame);
//	outputBindProperties.Insert(L"DisableTensorCpuSync", PropertyValue::CreateBoolean(true));
//	m_bindingPreprocess.Bind(outputName, intermediateTensor, outputBindProperties);
//	m_sessionPreprocess.EvaluateAsync(m_bindingPreprocess, L"");
//
//	// 3. Run through actual model
//	std::vector<int64_t> FCNResnetOutputShape = { 1, 21, m_imageHeightInPixels, m_imageWidthInPixels };
//	ITensor FCNResnetOutput = TensorFloat::Create(FCNResnetOutputShape);
//
//	m_binding.Bind(m_session.Model().InputFeatures().GetAt(0).Name(), intermediateTensor);
//	m_binding.Bind(m_session.Model().OutputFeatures().GetAt(0).Name(), FCNResnetOutput, outputBindProperties);
//	m_session.EvaluateAsync(m_binding, L"");
//
//	// Shape validation 
//	assert(m_outputVideoFrame.Direct3DSurface().Description().Height == m_imageHeightInPixels);
//	assert(m_outputVideoFrame.Direct3DSurface().Description().Width == m_imageWidthInPixels);
//
//	// 4. Postprocessing
//	outputBindProperties.Insert(L"DisableTensorCpuSync", PropertyValue::CreateBoolean(false));
//	m_bindingPostprocess.Bind(m_sessionPostprocess.Model().InputFeatures().GetAt(0).Name(), m_inputVideoFrame); // InputImage
//	m_bindingPostprocess.Bind(m_sessionPostprocess.Model().InputFeatures().GetAt(1).Name(), FCNResnetOutput); // InputScores
//	m_bindingPostprocess.Bind(m_sessionPostprocess.Model().OutputFeatures().GetAt(0).Name(), m_outputVideoFrame);
//	// TODO: Make this async as well, and add a completed 
//	m_sessionPostprocess.EvaluateAsync(m_bindingPostprocess, L"").get();
//	m_outputVideoFrame.CopyToAsync(outVideoFrame).get();
//	m_bSyncStarted = FALSE;
//	m_canRunEval.notify_one();
//}

VideoFrame BackgroundBlur::RunAsync(IDirect3DSurface src, IDirect3DSurface dest)
{
	assert(m_session.Device().AdapterId() == m_highPerfAdapter);
	VideoFrame inVideoFrame = VideoFrame::CreateWithDirect3D11Surface(src);
	VideoFrame outVideoFrame = VideoFrame::CreateWithDirect3D11Surface(dest);
	SetVideoFrames(inVideoFrame, outVideoFrame);

	// Shape validation
	assert((UINT32)m_inputVideoFrame.Direct3DSurface().Description().Height == m_imageHeightInPixels);
	assert((UINT32)m_inputVideoFrame.Direct3DSurface().Description().Width == m_imageWidthInPixels);

	assert(m_sessionPreprocess.Device().AdapterId() == m_highPerfAdapter);
	assert(m_sessionPostprocess.Device().AdapterId() == m_highPerfAdapter);

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
	m_evalStatus = m_sessionPostprocess.EvaluateAsync(m_bindingPostprocess, L"");
	// m_evalStatus = m_outputVideoFrame.CopyToAsync(outVideoFrame);
	/*auto makeOutput = [&outVideoFrame]() -> winrt::Windows::Foundation::IAsyncOperation<VideoFrame> { co_return outVideoFrame; };
	m_evalStatus = makeOutput();*/

	// todo: go back to have this return AsyncStatus for when done with copytoasync? 
	m_outputVideoFrame.CopyToAsync(outVideoFrame);
	return outVideoFrame;
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
			.SetConstant(L"min", TensorFloat::CreateFromIterable({ 1 }, { 0.f }))
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
		.Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"OutputImageForward", L"The NCHW image forwarded through the model.", TensorKind::Float, {1, c, h, w}))
		.Operators().Add(LearningModelOperator(L"Div") // Normalize from 0-255 to 0-1 by dividing by 255
			.SetInput(L"A", L"Input")
			.SetConstant(L"B", TensorFloat::CreateFromArray({}, { 255.f }))
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
			.SetOutput(L"C", L"Output"))
		.Operators().Add(LearningModelOperator(L"Identity")
			.SetInput(L"input", L"Input")
			.SetOutput(L"output", L"OutputImageForward"));
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

