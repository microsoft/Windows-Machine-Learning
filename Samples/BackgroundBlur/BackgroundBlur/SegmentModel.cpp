#include "SegmentModel.h"
#include <winrt/windows.foundation.collections.h>
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


SegmentModel::SegmentModel(): 
	m_sess(NULL)
{
	//m_sess = CreateLearningModelSession(ReshapeFlatBufferToNCHW(1,3,700,1000));
}

SegmentModel::SegmentModel(UINT32 w, UINT32 h) :
	m_sess(CreateLearningModelSession(ReshapeFlatBufferToNCHW(1, 3, h, w)))
{
	SetImageSize(w, h);
}

void SegmentModel::Run(const BYTE** pSrc, BYTE** pDest, DWORD cbImageSize) 
{
	// Right now the single input type I allow is topdown, this should work
	// TODO: Array_View initialized as [x, y) correct? 
	winrt::array_view<const byte> source{*pSrc, *pSrc + cbImageSize}; // TODO: Does this work when topdown vs. bottomup
	//winrt::array_view<byte> dest{ *pDest, *pDest + cbImageSize };
	std::vector<int64_t> shape = { 1, 3, m_imageHeightInPixels, m_imageWidthInPixels };

	auto inputRawTensor = TensorUInt8Bit::CreateFromArray(std::vector<int64_t>{1, cbImageSize}, source);
	auto outputTensor = TensorUInt8Bit::Create(shape);
	//auto outputTensor = TensorUInt8Bit::CreateFromArray(std::vector<int64_t>{1, cbImageSize}, dest);
	auto binding = LearningModelBinding(m_sess);
	hstring inputName = m_sess.Model().InputFeatures().GetAt(0).Name();
	hstring outputName = m_sess.Model().OutputFeatures().GetAt(0).Name();
	binding.Bind(inputName, inputRawTensor);

	//auto outputBindProperties = PropertySet();
	//outputBindProperties::Add(L"DisableTensorCpuSync", PropertyValue::CreateBoolean(true));
	binding.Bind(outputName, outputTensor);

	auto results = m_sess.Evaluate(binding, L"");
	auto resultTensor = results.Outputs().Lookup(L"Output").as<TensorUInt8Bit>();
	auto resultVector = resultTensor.GetAsVectorView();
	auto t = resultVector.GetAt(0);

	auto reference = resultTensor.CreateReference().data();
	
	CopyMemory(*pDest, reference, cbImageSize);
;
}

void SegmentModel::SetImageSize(UINT32 w, UINT32 h) 
{
	m_imageWidthInPixels = w;
	m_imageHeightInPixels = h;
}

LearningModel SegmentModel::ConvertToGrayScale(long n, long c, long h, long w) {
	// Byte order in this buffer is B G R
	auto builder = LearningModelBuilder::Create(11)
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(hstring(L"Input"), TensorKind::UInt8, winrt::array_view<int64_t const>({1, n*c*h*w})))
		.Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(hstring(L"Output"), TensorKind::Float, winrt::array_view<int64_t const>{1, n, c, h, w}))
		;

	return builder.CreateModel();
}

LearningModel SegmentModel::ReshapeFlatBufferToNCHW(long n, long c, long h, long w) {
	auto size = std::vector<int64_t>{ 1 };
	//TensorInt64Bit::CreateFromIterable(winrt::param::iterable<int64_t>({ 1,2,3 }), size);
	auto builder = LearningModelBuilder::Create(11)
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Input", TensorKind::UInt8, winrt::array_view<int64_t const>({ 1, n * c * h * w })))
		//.Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output", TensorKind::UInt8, winrt::array_view<int64_t const>{ 1, n* c* h* w}))
		.Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(L"Output", TensorKind::UInt8, winrt::array_view<int64_t const>{1, n, c, h, w}))
		.Operators().Add(LearningModelOperator((L"Cast"))
			.SetInput(L"input", L"Input")
			.SetOutput(L"output", L"SliceOutput")
			.SetAttribute(L"to",
				TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{}, std::vector<int64_t>{OnnxDataType::ONNX_FLOAT})))
		.Operators().Add(LearningModelOperator(L"Reshape")
			.SetInput(L"data", L"SliceOutput")
			.SetConstant(L"shape", TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{4}, std::vector<int64_t>{n, h, w, c}))
			.SetOutput(L"reshaped", L"ReshapeOutput"))
		/*.Operators().Add(LearningModelOperator(L"Transpose")
			.SetInput(L"data", L"ReshapeOutput")
			.SetOutput(L"transposed", L"TransposeOutput")
			.SetAttribute(L"perm", TensorInt64Bit::CreateFromArray(std::vector<int64_t>{4}, std::vector<int64_t>{0, 3, 1, 2})))*/
		// Now shape NCHW
		.Operators().Add(LearningModelOperator(L"Mul")
			.SetInput(L"A", L"ReshapeOutput")
			.SetConstant(L"B", TensorFloat::CreateFromIterable(std::vector<int64_t>{1}, std::vector<float>{0.333f}))
			//.SetConstant(L"B", TensorFloat::CreateFromIterable(std::vector<int64_t>{3}, std::vector<float>{0.114f, 0.587f, 0.299f}))
			.SetOutput(L"C", L"MulOutput")
		)
		.Operators().Add(LearningModelOperator((L"Cast"))
			.SetInput(L"input", L"MulOutput")
			.SetOutput(L"output", L"Output")
			.SetAttribute(L"to",
				TensorInt64Bit::CreateFromIterable(std::vector<int64_t>{}, std::vector<int64_t>{OnnxDataType::ONNX_UINT8})))
		;
	return builder.CreateModel();
}

LearningModelSession SegmentModel::CreateLearningModelSession(LearningModel model, bool closeModel) {
	auto device = LearningModelDevice(LearningModelDeviceKind::Default); // Todo: Have a toggle between GPU/ CPU? 
	/*auto options = LearningModelSessionOptions() // TODO: Figure out with wifi
	{
		CloseModelOnSessionCreation = closeModel, // Close the model to prevent extra memory usage
		BatchSizeOverride = 0
	};*/
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

LearningModelBinding SegmentModel::Evaluate(ITensor* input, ITensor* output, bool wait) 
{
	auto binding = LearningModelBinding(m_sess);
	hstring inputName = m_sess.Model().InputFeatures().GetAt(0).Name();
	hstring outputName = m_sess.Model().OutputFeatures().GetAt(0).Name();
	binding.Bind(inputName, *input);

	//auto outputBindProperties = PropertySet();
	//outputBindProperties::Add(L"DisableTensorCpuSync", PropertyValue::CreateBoolean(true));
	binding.Bind(outputName, *output);
	//EvaluateInternal(m_sess, binding);

	auto results = m_sess.Evaluate(binding, L"");
	auto resultTensor = results.Outputs().Lookup(L"Output").as<TensorFloat>();
	auto resultVector = resultTensor.GetAsVectorView();

	return binding;
}

