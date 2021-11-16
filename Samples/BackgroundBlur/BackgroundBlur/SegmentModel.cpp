#include "SegmentModel.h"
#include <winrt/windows.foundation.collections.h>
using winrt::Windows::Foundation::PropertyValue;
using winrt::hstring;
using namespace winrt;
using namespace Windows::Foundation::Collections;
#include <iostream>
#include <filesystem>
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

SegmentModel::SegmentModel() :
	m_sess(NULL),
	m_bufferSess(NULL),
	m_useGPU(true)
{
}

SegmentModel::SegmentModel(UINT32 w, UINT32 h) :
	m_sess(NULL),
	m_bufferSess(NULL),
	m_useGPU(true)
{
	// TODO: Adapt based on video_FOURCC- might not always be RGBA

	SetImageSize(w, h);
	// 4 channels: RGBA
	m_sess = CreateLearningModelSession(ReshapeFlatBufferToNCHWAndInvert(1, 4, h, w));

	m_bufferSess = CreateLearningModelSession(ReshapeFlatBufferToNCHW(1, 3, h, w));
}

void SegmentModel::Run(const BYTE** pSrc, BYTE** pDest, DWORD cbImageSize)
{
	// TODO: Make all these sessions earlier, so just passing inputs/outputs here

	// 1. Get input buffer as a Tensor
	winrt::array_view<const byte> source{ *pSrc, *pSrc + cbImageSize }; // TODO: Does this work when topdown vs. bottomup
	std::vector<int64_t> shape = { 1, 3, m_imageHeightInPixels, m_imageWidthInPixels };
	ITensor inputRawTensor = TensorUInt8Bit::CreateFromArray(std::vector<int64_t>{1, cbImageSize}, source);
	ITensor tensorizedImg = TensorFloat::Create(shape);
	auto tensorizationBinding = Evaluate(m_bufferSess, std::vector<ITensor*>{&inputRawTensor}, &tensorizedImg);

	// 2. Normalize input tensor
	std::vector<float> mean = { 0.485f, 0.456f, 0.406f };
	std::vector<float> std = { 0.229f, 0.224f, 0.225f };
	LearningModelSession normalizationSession = CreateLearningModelSession(Normalize0_1ThenZScore(m_imageHeightInPixels, m_imageWidthInPixels, 3, mean, std));
	ITensor intermediateTensor = TensorFloat::Create(shape);
	auto normalizationBinding = Evaluate(normalizationSession, std::vector<ITensor*>{&tensorizedImg}, & intermediateTensor);

	// 3. Run through actual model
	std::vector<int64_t> FCNResnetOutputShape = { 1, 21, m_imageHeightInPixels, m_imageWidthInPixels };
	LearningModelSession FCNResnetSession = CreateLearningModelSession(FCNResnet());
	ITensor FCNResnetOutput = TensorFloat::Create(FCNResnetOutputShape);
	auto FCNResnetBinding = Evaluate(FCNResnetSession, std::vector<ITensor*>{&intermediateTensor}, & FCNResnetOutput);

	// 4.Extract labels with argmax
	ITensor rawLabels = TensorFloat::Create({1, 1, m_imageHeightInPixels, m_imageWidthInPixels});
	LearningModelSession argmaxSession = CreateLearningModelSession(Argmax(1, m_imageHeightInPixels, m_imageWidthInPixels));
	auto argmaxBinding = Evaluate(argmaxSession, std::vector<ITensor*>{&FCNResnetOutput}, & rawLabels);

	// 5. Get the foreground
	ITensor foreground = TensorUInt8Bit::Create(std::vector<int64_t>{1, m_imageHeightInPixels, m_imageWidthInPixels, 3});
	LearningModelSession foregroundSession = CreateLearningModelSession(GetBackground(1, 3, m_imageHeightInPixels, m_imageWidthInPixels));
	// Enable tensorcpusync for the last evaluate so can extract and give back to buffer
	// Will remove once can just pass along d3d reources back to MFT
	auto foregroundBinding = Evaluate(foregroundSession, std::vector<ITensor*>{&tensorizedImg, &rawLabels}, & foreground, true);
	
	UINT32 outCapacity = 0;
	if (m_useGPU)
	{
		// v1: just get the reference- should fail
		auto reference = foreground.as<TensorUInt8Bit>().CreateReference().data();

		// v3: get from a d3dresource
		/*ID3D12Resource* res = NULL;
		HRESULT hr = foreground.as<ITensorNative>()->GetD3D12Resource(&res);
		UINT DstRowPitch = 0, DstDepthPitch = 0, SrcSubresource = 0;
		hr = res->ReadFromSubresource((void*)*pDest, DstRowPitch, DstDepthPitch, SrcSubresource, NULL);*/
		return;
	}
	else 
	{
		auto reference = foreground.as<TensorUInt8Bit>().CreateReference().data();
		CopyMemory(*pDest, reference, cbImageSize);
	}
}

void SegmentModel::RunTest(const BYTE** pSrc, BYTE** pDest, DWORD cbImageSize) 
{
	// Right now the single input type I allow is topdown, this should work
	winrt::array_view<const byte> source{*pSrc, *pSrc + cbImageSize}; // TODO: Does this work when topdown vs. bottomup
	std::vector<int64_t> shape = { 1, m_imageHeightInPixels, m_imageWidthInPixels, 4 };

	ITensor inputRawTensor = TensorUInt8Bit::CreateFromArray(std::vector<int64_t>{1, cbImageSize}, source);
	ITensor outputTensor = TensorUInt8Bit::Create(shape);
	auto binding = Evaluate(m_sess, { &inputRawTensor }, &outputTensor, true);

	UINT32 outCapacity = 0;
	if (m_useGPU)
	{
		// v1: just get the reference- should fail
		auto reference = outputTensor.as<TensorUInt8Bit>().CreateReference().data();
		CopyMemory(*pDest, reference, cbImageSize);

		// v3: get from a d3dresource
		/*ID3D12Resource* res = NULL;
		HRESULT hr = outputTensor.as<ITensorNative>()->GetD3D12Resource(&res);
		UINT DstRowPitch = 0, DstDepthPitch = 0, SrcSubresource = 0;
		hr = res->ReadFromSubresource((void*)*pDest, DstRowPitch, DstDepthPitch, SrcSubresource, NULL);*/
		return;
	}
}

void SegmentModel::SetImageSize(UINT32 w, UINT32 h) 
{
	m_imageWidthInPixels = w;
	m_imageHeightInPixels = h;
}

LearningModel SegmentModel::Argmax(long axis, long h, long w)
{
	auto builder = LearningModelBuilder::Create(12)
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Data", TensorKind::Float, array_view<int64_t const>{ -1, -1, h, w })) // Different input type? 
		.Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output", TensorKind::Float, array_view<int64_t const>{ -1, -1, h, w })) // Output of int64? 
		.Operators().Add( LearningModelOperator(L"ArgMax")
			.SetInput(L"data", L"Data")
			.SetAttribute(L"keepdims", TensorInt64Bit::CreateFromArray(std::vector<int64_t>{1}, std::vector<int64_t>{ 1 }))
			.SetAttribute(L"axis", TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{1}, std::vector<int64_t> { axis })) // Correct way of passing axis? 
			.SetOutput(L"reduced", L"Reduced"))
		.Operators().Add( LearningModelOperator(L"Cast")
			.SetInput(L"input", L"Reduced")
			.SetAttribute(L"to", TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{}, std::vector<int64_t>{OnnxDataType::ONNX_FLOAT}))
			.SetOutput(L"output", L"Output"))
		;

	return builder.CreateModel();
}

LearningModel SegmentModel::FCNResnet()
{
	auto rel = std::filesystem::current_path();
	rel.append("Assets\\fcn-resnet50-11.onnx");
	return LearningModel::LoadFromFilePath(rel + L"");
}

LearningModel SegmentModel::GetBackground(long n, long c, long h, long w)
{
	auto builder = LearningModelBuilder::Create(12)
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"InputImage", TensorKind::Float, array_view<int64_t const>{ n, c, h, w }))
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"InputMask", TensorKind::Float, array_view<int64_t const>{ n, 1, h, w })) // Broadcast to each color channel
		.Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output", TensorKind::UInt8, array_view<int64_t const>{ n, h, w, c }))
		// Averagepool input image
		/*.Operators().Add(LearningModelOperator(L"AveragePool")
			.SetInput(L"X", L"InputImage")
			.SetAttribute(L"kernel_shape", TensorInt64Bit::CreateFromArray(std::vector<int64_t>{2}, std::vector<int64_t>{10, 10}))
			.SetAttribute(L"auto_pad", TensorString::CreateFromArray(std::vector<int64_t>{1}, std::vector<hstring>{L"SAME_UPPER"}))
			.SetOutput(L"Y", L"BlurredImage"))*/
		// Make mask
		.Operators().Add(LearningModelOperator(L"Clip")
			.SetInput(L"input", L"InputMask")
			.SetConstant(L"max", TensorFloat::CreateFromIterable(std::vector<int64_t>{ 1 }, std::vector<float>{ 1.f }))
			.SetOutput(L"output", L"MaskBinary"))
		.Operators().Add(LearningModelOperator(L"Mul")
			.SetInput(L"A", L"MaskBinary")
			.SetConstant(L"B", TensorFloat::CreateFromIterable(std::vector<int64_t>{1}, std::vector<float>{-1.f}))
			.SetOutput(L"C", L"NegMask"))
		.Operators().Add(LearningModelOperator(L"Add") // BackgroundMask = (1- foreground Mask)
			.SetConstant(L"A", TensorFloat::CreateFromIterable(std::vector<int64_t>{1}, std::vector<float>{1.f}))
			.SetInput(L"B", L"NegMask")
			.SetOutput(L"C", L"BackgroundMask"))
		// Extract blurred background 
		.Operators().Add(LearningModelOperator(L"Mul")
			.SetInput(L"A", L"InputImage")
			.SetInput(L"B", L"BackgroundMask")
			.SetOutput(L"C", L"Background"))
		// TODO: REmove once compose w foreground
		.Operators().Add(LearningModelOperator(L"Transpose")
			.SetInput(L"data", L"Background")
			.SetAttribute(L"perm", TensorInt64Bit::CreateFromArray(std::vector<int64_t>{ 4 }, std::vector<int64_t>{ 0, 2, 3, 1 }))
			.SetOutput(L"transposed", L"TransposeOutput"))
		.Operators().Add(LearningModelOperator(L"Cast")
			.SetInput(L"input", L"TransposeOutput")
			.SetOutput(L"output", L"Output")
			.SetAttribute(L"to",
				TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{}, std::vector<int64_t>{OnnxDataType::ONNX_UINT8})))
		;

	return builder.CreateModel();
}

LearningModel SegmentModel::GetForeground(long n, long c, long h, long w)
{
	auto builder = LearningModelBuilder::Create(12)
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"InputImage", TensorKind::Float, array_view<int64_t const>{ n, c, h, w }))
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"InputMask", TensorKind::Float, array_view<int64_t const>{ n, 1, h, w })) // Broadcast to each color channel
		.Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output", TensorKind::UInt8, array_view<int64_t const>{ n, h, w, c }))
		.Operators().Add(LearningModelOperator(L"Clip")
			.SetInput(L"input", L"InputMask")
			.SetConstant(L"max", TensorFloat::CreateFromIterable(std::vector<int64_t>{ 1 }, std::vector<float>{ 1.f }))
			.SetOutput(L"output", L"MaskBinary"))
		.Operators().Add(LearningModelOperator(L"Mul")
			.SetInput(L"A", L"InputImage")
			.SetInput(L"B", L"MaskBinary")
			.SetOutput(L"C", L"Foreground"))
		// Convert to buffer output- detensorization? 
		.Operators().Add(LearningModelOperator(L"Transpose")
			.SetInput(L"data", L"Foreground")
			.SetAttribute(L"perm", TensorInt64Bit::CreateFromArray(std::vector<int64_t>{ 4 }, std::vector<int64_t>{ 0, 2, 3, 1 }))
			.SetOutput(L"transposed", L"TransposeOutput"))
		/*.Operators().Add(LearningModelOperator(L"Reshape")
			.SetInput(L"data", L"TransposeOutput")
			.SetConstant(L"shape", TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{ 2 }, std::vector<int64_t>{ 1, n*c*h*w }))
			.SetOutput(L"reshaped", L"ReshapeOutput"))*/
		.Operators().Add(LearningModelOperator(L"Cast")
			.SetInput(L"input", L"TransposeOutput")
			.SetOutput(L"output", L"Output")
			.SetAttribute(L"to",
				TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{}, std::vector<int64_t>{OnnxDataType::ONNX_UINT8})))
		;

	return builder.CreateModel();
}

LearningModel SegmentModel::Normalize0_1ThenZScore(long h, long w, long c, const std::vector<float>& means, const std::vector<float>& stddev)
{
	assert(means.size() == c);
	assert(stddev.size() == c);

	auto builder = LearningModelBuilder::Create(12)
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Input", L"The NCHW image", TensorKind::Float, array_view<int64_t const>{1, c, h, w}))
		.Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output", L"The NCHW image normalized with mean and stddev.", TensorKind::Float, array_view<int64_t const>{1, c, h, w}))
		.Operators().Add(LearningModelOperator(L"Div") // Normalize from 0-255 to 0-1 by dividing by 255
			.SetInput(L"A", L"Input")
			.SetConstant(L"B", TensorFloat::CreateFromArray(std::vector<int64_t>{}, array_view<const float>{ 255.f }))
			.SetOutput(L"C", L"DivOutput"))
		.Operators().Add(LearningModelOperator(L"Reshape")
			.SetConstant(L"data", TensorFloat::CreateFromArray(std::vector<int64_t>{ c }, means))
			.SetConstant(L"shape", TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{ 4 }, std::vector<int64_t>{ 1, c, 1, 1 }))
			.SetOutput(L"reshaped", L"MeansReshaped"))
		.Operators().Add(LearningModelOperator(L"Reshape")
			.SetConstant(L"data", TensorFloat::CreateFromArray(std::vector<int64_t>{ c }, stddev))
			.SetConstant(L"shape", TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{ 4 }, std::vector<int64_t>{ 1, c, 1, 1 }))
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

LearningModel SegmentModel::ReshapeFlatBufferToNCHW(long n, long c, long h, long w)
{
	auto builder = LearningModelBuilder::Create(11)
		// Loading in buffers and reshape
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Input", TensorKind::UInt8, array_view<int64_t const>({ 1, n * c * h * w })))
		.Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output", TensorKind::Float, array_view<int64_t const>{n, c, h, w}))
		.Operators().Add(LearningModelOperator((L"Cast"))
			.SetInput(L"input", L"Input")
			.SetOutput(L"output", L"SliceOutput")
			.SetAttribute(L"to",
				TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{}, std::vector<int64_t>{OnnxDataType::ONNX_FLOAT})))
		.Operators().Add(LearningModelOperator(L"Reshape")
			.SetInput(L"data", L"SliceOutput")
			.SetConstant(L"shape", TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{4}, std::vector<int64_t>{n, h, w, c}))
			.SetOutput(L"reshaped", L"ReshapeOutput"))
		.Operators().Add(LearningModelOperator(L"Transpose")
			.SetInput(L"data", L"ReshapeOutput")
			.SetAttribute(L"perm", TensorInt64Bit::CreateFromArray(std::vector<int64_t>{ 4 }, std::vector<int64_t>{ 0, 3, 1, 2 }))
			.SetOutput(L"transposed", L"Output"))
	;
	return builder.CreateModel();
}

LearningModel SegmentModel::ReshapeFlatBufferToNCHWAndInvert(long n, long c, long h, long w) {
	auto size = std::vector<int64_t>{ 1 };
	//TensorInt64Bit::CreateFromIterable(winrt::param::iterable<int64_t>({ 1,2,3 }), size);
	auto builder = LearningModelBuilder::Create(11)
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Input", TensorKind::UInt8, winrt::array_view<int64_t const>({ 1, n * c * h * w })))
		// Remove the alpha channel
		.Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output", TensorKind::UInt8, winrt::array_view<int64_t const>{n, h, w, c}))
		.Operators().Add(LearningModelOperator((L"Cast"))
			.SetInput(L"input", L"Input")
			.SetOutput(L"output", L"CastOutput")
			.SetAttribute(L"to",
				TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{}, std::vector<int64_t>{OnnxDataType::ONNX_FLOAT})))
		.Operators().Add(LearningModelOperator(L"Reshape")
			.SetInput(L"data", L"CastOutput")
			.SetConstant(L"shape", TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{4}, std::vector<int64_t>{n, h, w, c}))
			.SetOutput(L"reshaped", L"ReshapeOutput"))
		/*.Operators().Add(LearningModelOperator(L"Slice")
			.SetInput(L"data", L"ReshapeOutput")
			.SetConstant(L"starts", TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{ 4 }, std::vector<int64_t>{ 0, 0, 0, 0 }))
			.SetConstant(L"ends", TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{ 4 }, std::vector<int64_t>{ n, h, w, c - 1 }))
			.SetOutput(L"output", L"SliceOutput"))*/
		// Now shape NCHW
		.Operators().Add(LearningModelOperator(L"Mul")
			.SetInput(L"A", L"ReshapeOutput")
			.SetConstant(L"B", TensorFloat::CreateFromIterable(std::vector<int64_t>{1}, std::vector<float>{-1.f}))
			//.SetConstant(L"B", TensorFloat::CreateFromIterable(std::vector<int64_t>{3}, std::vector<float>{0.114f, 0.587f, 0.299f}))
			.SetOutput(L"C", L"MulOutput")
		)
		.Operators().Add(LearningModelOperator(L"Add")
			.SetConstant(L"A", TensorFloat::CreateFromIterable(std::vector<int64_t>{1}, std::vector<float>{255.f}))
			.SetInput(L"B", L"MulOutput")
			.SetOutput(L"C", L"AddOutput")
		)
		.Operators().Add(LearningModelOperator((L"Cast"))
			.SetInput(L"input", L"AddOutput")
			.SetOutput(L"output", L"Output")
			.SetAttribute(L"to",
				TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{}, std::vector<int64_t>{OnnxDataType::ONNX_UINT8})))
		;
	return builder.CreateModel();
}

LearningModelSession SegmentModel::CreateLearningModelSession(const LearningModel& model, bool closeModel) {
	auto device = m_useGPU ? LearningModelDevice(LearningModelDeviceKind::DirectX) : LearningModelDevice(LearningModelDeviceKind::Default); // Todo: Have a toggle between GPU/ CPU? 
	auto options = LearningModelSessionOptions(); 
	options.BatchSizeOverride(0);
	options.CloseModelOnSessionCreation(closeModel);
	auto session = LearningModelSession(model, device);
	return session;
}


void EvaluateInternal(LearningModelSession sess, LearningModelBinding bind, bool wait = false)
{
	auto results = sess.Evaluate(bind, L"");
	/*auto results = sess.EvaluateAsync(bind, L"");
	if (wait) {
		results.GetResults(); // TODO: Will this actually wait?
	}*/
}

LearningModelBinding SegmentModel::Evaluate(LearningModelSession sess,const std::vector<ITensor*>& input, ITensor* output, bool wait) 
{
	auto binding = LearningModelBinding(sess);

	for (int i = 0; i < input.size(); i++)
	{
		hstring inputName = sess.Model().InputFeatures().GetAt(i).Name();
		binding.Bind(inputName, *input[i]);
	}
	//hstring inputName = sess.Model().InputFeatures().GetAt(0).Name();
	hstring outputName = sess.Model().OutputFeatures().GetAt(0).Name();

	auto outputBindProperties = PropertySet();
	outputBindProperties.Insert(L"DisableTensorCpuSync", PropertyValue::CreateBoolean(!wait));
	binding.Bind(outputName, *output, outputBindProperties);
	//EvaluateInternal(sess, binding);

	auto results = sess.Evaluate(binding, L"");
	auto resultTensor = results.Outputs().Lookup(outputName).try_as<TensorFloat>();
	float testPixels[6];
	if (resultTensor) {
		auto resultVector = resultTensor.GetAsVectorView();
		resultVector.GetMany(0, testPixels);
	}

	return binding;
}

