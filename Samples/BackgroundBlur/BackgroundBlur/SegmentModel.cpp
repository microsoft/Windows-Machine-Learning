#include "SegmentModel.h"
#include <wtypes.h>

using winrt::hstring;
using winrt::param::iterable;

enum OnnxDataType : long {
	O_UNDEFINED = 0,
	// Basic types.
	O_FLOAT = 1,
	O_UINT8 = 2,
	O_INT8 = 3,
	O_UINT16 = 4,
	O_INT16 = 5,
	O_INT32 = 6,
	O_INT64 = 7,
	O_STRING = 8,
	O_BOOL = 9,

	// IEEE754 half-precision floating-point format (16 bits wide).
	// This format has 1 sign bit, 5 exponent bits, and 10 mantissa bits.
	O_FLOAT16 = 10,

	O_DOUBLE = 11,
	O_UINT32 = 12,
	O_UINT64 = 13,
	O_COMPLEX64 = 14,     // complex with float32 real and imaginary components
	O_COMPLEX128 = 15,    // complex with float64 real and imaginary components

	// Non-IEEE floating-point format based on IEEE754 single-precision
	// floating-point number truncated to 16 bits.
	// This format has 1 sign bit, 8 exponent bits, and 7 mantissa bits.
	O_BFLOAT16 = 16,
}OnnxDataType;

SegmentModel::SegmentModel(): 
	m_sess(NULL)
{
	//LearningModel grayscaleModel = ConvertToGrayScale(1, 1, 1, 1); // Change to size of the image
	//m_sess = CreateLearningModelSession(grayscaleModel);
}

LearningModel SegmentModel::ConvertToGrayScale(long n, long c, long h, long w) {
	//winrt::array_view<int64_t, 4>  a({ 1, n * c * h * w });
	//winrt::array_view<int64_t const> shape = { 1,n * c * h * w };
	auto builder = LearningModelBuilder::Create(11)
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(hstring(L"Input"), TensorKind::UInt8, winrt::array_view<int64_t const>({1, n*c*h*w})))
		.Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(hstring(L"Output"), TensorKind::Float, winrt::array_view<int64_t const>{1, n, c, h, w}))
		;
	//winrt::hstring i = L"Inputs";

	return builder.CreateModel();
}

LearningModel SegmentModel::ReshapeFlatBufferToNCHW(long n, long c, long h, long w) {
	auto size = std::vector<int64_t>{ 1 };
	TensorInt64Bit::CreateFromIterable(size, size);

	auto builder = LearningModelBuilder::Create(11)
		.Inputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(hstring(L"Input"), TensorKind::UInt8, winrt::array_view<int64_t const>({ 1, n * c * h * w })))
		.Outputs().Add(LearningModelBuilder::CreateTensorFeatureDescriptor(hstring(L"Output"), TensorKind::UInt8, winrt::array_view<int64_t const>{1, n, c, h, w}))
		.Operators().Add(LearningModelOperator(hstring(L"Cast"))
			.SetInput((hstring)L"input", (hstring)L"Input")
			.SetOutput((hstring)L"output", (hstring)L"Output")
			.SetAttribute((hstring)L"to", 
				TensorInt64Bit::CreateFromShapeArrayAndDataArray(winrt::array_view<int64_t const>{}, winrt::array_view<int64_t const>{OnnxDataType::O_FLOAT}))
				);
		);

	return builder.CreateModel();
}

LearningModelSession SegmentModel::CreateLearningModelSession(LearningModel model, bool closeModel) {
	auto device = LearningModelDevice(LearningModelDeviceKind::DirectX); // Todo: Have a toggle between GPU/ CPU? 
	/*auto options = LearningModelSessionOptions() // TODO: Figure out with wifi
	{
		CloseModelOnSessionCreation = closeModel, // Close the model to prevent extra memory usage
		BatchSizeOverride = 0
	};*/
	auto session = LearningModelSession(model, device);
	return session;
}