#include "pch.h"
#include "SegmentModel.h"
#include <winrt/windows.foundation.collections.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>

#include <iostream>
#include <filesystem>

using winrt::Windows::Foundation::PropertyValue;
using winrt::hstring;
using namespace winrt;
using namespace Windows::Foundation::Collections;

int i = 0;
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

// TODO: Probably don't need to be globals
std::array<float, 3> mean = { 0.485f, 0.456f, 0.406f };
std::array<float, 3> stddev = { 0.229f, 0.224f, 0.225f };
auto outputBindProperties = PropertySet();


SegmentModel::SegmentModel() :
	m_sess(NULL),
	m_sessPreprocess(NULL),
	m_sessFCN(NULL),
	m_sessPostprocess(NULL),
	m_sessStyleTransfer(NULL),
	m_useGPU(true),
	m_bindPreprocess(NULL),
	m_bindFCN(NULL),
	m_bindPostprocess(NULL),
	m_bindStyleTransfer(NULL)
	//bindings(swapChainEntryCount)
{
}

SegmentModel::SegmentModel(UINT32 w, UINT32 h) :
	m_sess(NULL),
	m_sessPreprocess(NULL),
	m_sessFCN(NULL),
	m_sessPostprocess(NULL),
	m_sessStyleTransfer(NULL),
	m_useGPU(true),
	m_bindPreprocess(NULL),
	m_bindFCN(NULL),
	m_bindPostprocess(NULL),
	m_bindStyleTransfer(NULL)
	//bindings(swapChainEntryCount)
{

	SetImageSize(w, h);
	m_sess = nullptr;//CreateLearningModelSession(Invert(1, 3, h, w));
	m_sessStyleTransfer = CreateLearningModelSession(StyleTransfer());
	m_bindStyleTransfer = LearningModelBinding(m_sessStyleTransfer);

	// Initialize segmentation learningmodelsessions
	m_sessPreprocess = CreateLearningModelSession(Normalize0_1ThenZScore(h, w, 3, mean, stddev));
	m_sessFCN = CreateLearningModelSession(FCNResnet());
	m_sessPostprocess = CreateLearningModelSession(PostProcess(1, 3, h, w, 1));

	// Initialize segmentation bindings
	m_bindPreprocess = LearningModelBinding(m_sessPreprocess);
	m_bindFCN = LearningModelBinding(m_sessFCN);
	m_bindPostprocess = LearningModelBinding(m_sessPostprocess);

	// Create set of bindings to cycle through
	/*for (int i = 0; i < swapChainEntryCount; i++) {
		bindings.push_back(std::make_unique<SwapChainEntry>());
		bindings[i]->binding = LearningModelBinding(m_sessStyleTransfer);
		bindings[i]->binding.Bind(L"outputImage",
			VideoFrame(Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8, 720, 720));
	}*/

}

void SegmentModel::SetModels(UINT32 w, UINT32 h) {
	SetImageSize(w, h);
	m_sess = nullptr;//CreateLearningModelSession(Invert(1, 3, h, w));
	m_sessStyleTransfer = CreateLearningModelSession(StyleTransfer());
	m_bindStyleTransfer = LearningModelBinding(m_sessStyleTransfer);

	// Initialize segmentation learningmodelsessions
	m_sessPreprocess = CreateLearningModelSession(Normalize0_1ThenZScore(h, w, 3, mean, stddev));
	m_sessFCN = CreateLearningModelSession(FCNResnet());
	m_sessPostprocess = CreateLearningModelSession(PostProcess(1, 3, h, w, 1));

	// Initialize segmentation bindings
	m_bindPreprocess = LearningModelBinding(m_sessPreprocess);
	m_bindFCN = LearningModelBinding(m_sessFCN);
	m_bindPostprocess = LearningModelBinding(m_sessPostprocess);

	// Create set of bindings to cycle through
	/*for (int i = 0; i < swapChainEntryCount; i++) {
		bindings.push_back(std::make_unique<SwapChainEntry>());
		bindings[i]->binding = LearningModelBinding(m_sessStyleTransfer);
		bindings[i]->binding.Bind(L"outputImage",
			VideoFrame(Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8, 720, 720));
	}*/
 }

void SegmentModel::SetImageSize(UINT32 w, UINT32 h)
{
	m_imageWidthInPixels = w;
	m_imageHeightInPixels = h;
}

void SegmentModel::Run(IDirect3DSurface src, IDirect3DSurface dest, IDirect3DDevice device)
{
	OutputDebugString(L"\n [ Starting runTest | "); i++;

	// 1. Get input buffer as a VideoFrame
	VideoFrame input = VideoFrame::CreateWithDirect3D11Surface(src);
	VideoFrame output = VideoFrame::CreateWithDirect3D11Surface(dest);

	auto desc = input.Direct3DSurface().Description();
	auto descOut = output.Direct3DSurface().Description();
	VideoFrame input2 = VideoFrame::CreateAsDirect3D11SurfaceBacked(desc.Format, desc.Width, desc.Height, device);
	VideoFrame output2 = VideoFrame::CreateAsDirect3D11SurfaceBacked(descOut.Format, descOut.Width, descOut.Height, device);

	input.CopyToAsync(input2).get(); // TODO: I'm guessing it's this copy that's causing issues... 
	output.CopyToAsync(output2).get(); 
	std::vector<int64_t> shape = { 1, 3, m_imageHeightInPixels, m_imageWidthInPixels };
	// TODO: Make sure input surface still has the same shape as m_imageHeight and m_imageWidth? 

	// 2. Preprocessing: z-score normalization 
	ITensor intermediateTensor = TensorFloat::Create(shape);
	hstring inputName = m_sessPreprocess.Model().InputFeatures().GetAt(0).Name();
	hstring outputName = m_sessPreprocess.Model().OutputFeatures().GetAt(0).Name();

	m_bindPreprocess.Bind(inputName, input2);
	outputBindProperties.Insert(L"DisableTensorCpuSync", PropertyValue::CreateBoolean(true));
	m_bindPreprocess.Bind(outputName, intermediateTensor, outputBindProperties);
	m_sessPreprocess.EvaluateAsync(m_bindPreprocess, L"");

	// 3. Run through actual model
	std::vector<int64_t> FCNResnetOutputShape = { 1, 21, m_imageHeightInPixels, m_imageWidthInPixels };
	ITensor FCNResnetOutput = TensorFloat::Create(FCNResnetOutputShape);

	m_bindFCN.Bind(m_sessFCN.Model().InputFeatures().GetAt(0).Name(), intermediateTensor);
	m_bindFCN.Bind(m_sessFCN.Model().OutputFeatures().GetAt(0).Name(), FCNResnetOutput, outputBindProperties);
	m_sessFCN.EvaluateAsync(m_bindFCN, L"");

	// 4.Postprocessing: extract labels from FCN scores and use to compose background-blurred image
	ITensor rawLabels = TensorFloat::Create({1, 1, m_imageHeightInPixels, m_imageWidthInPixels});
	outputBindProperties.Insert(L"DisableTensorCpuSync", PropertyValue::CreateBoolean(false));
	m_bindPostprocess.Bind(m_sessPostprocess.Model().InputFeatures().GetAt(0).Name(), input2); // InputImage
	m_bindPostprocess.Bind(m_sessPostprocess.Model().InputFeatures().GetAt(1).Name(), FCNResnetOutput); // InputScores
	m_bindPostprocess.Bind(m_sessPostprocess.Model().OutputFeatures().GetAt(0).Name(), output2); // TODO: DisableTensorCPUSync to false now? 
	// Retrieve final output
	m_sessPostprocess.EvaluateAsync(m_bindPostprocess, L"").get(); 

	// Copy back to the correct surface for MFT
	output2.CopyToAsync(output).get(); 

	// Clean up bindings before returning
	m_bindPreprocess.Clear();
	m_bindFCN.Clear();
	m_bindPostprocess.Clear();
	input.Close();
	input2.Close();
	output2.Close();
	output.Close();
	OutputDebugString(L" Ending runTest ]");
}

//void SegmentModel::SubmitEval(VideoFrame input, VideoFrame output) {
//	auto currentBinding = bindings[0].get();
//	if (currentBinding->activetask == nullptr
//		|| currentBinding->activetask.Status() != Windows::Foundation::AsyncStatus::Started)
//	{
//		auto now = std::chrono::high_resolution_clock::now();
//		OutputDebugString(L"PF Start new Eval ");
//		OutputDebugString(std::to_wstring(swapChainIndex).c_str());
//		OutputDebugString(L" | ");
//		// submit an eval and wait for it to finish submitting work
//
//		{
//			std::lock_guard<std::mutex> guard{ Processing };
//			currentBinding->binding.Bind(L"inputImage", input);
//		}
//		std::rotate(bindings.begin(), bindings.begin() + 1, bindings.end());
//		finishedFrameIndex = (finishedFrameIndex - 1 + swapChainEntryCount) % swapChainEntryCount;
//		currentBinding->activetask = m_sessStyleTransfer.EvaluateAsync(
//			currentBinding->binding,
//			std::to_wstring(swapChainIndex).c_str());
//		currentBinding->activetask.Completed([&, currentBinding, now](auto&& asyncInfo, winrt::Windows::Foundation::AsyncStatus const) {
//			OutputDebugString(L"PF Eval completed |");
//			VideoFrame evalOutput = asyncInfo.GetResults()
//				.Outputs()
//				.Lookup(L"outputImage")
//				.try_as<VideoFrame>();
//			int bindingIdx;
//			bool finishedFrameUpdated;
//			{
//				std::lock_guard<std::mutex> guard{ Processing };
//				auto binding = std::find_if(bindings.begin(),
//					bindings.end(),
//					[currentBinding](const auto& b)
//					{
//						return b.get() == currentBinding;
//					});
//				bindingIdx = std::distance(bindings.begin(), binding);
//				finishedFrameUpdated = bindingIdx >= finishedFrameIndex;
//				finishedFrameIndex = finishedFrameUpdated ? bindingIdx : finishedFrameIndex;
//			}
//			if (finishedFrameUpdated)
//			{
//				OutputDebugString(L"PF Copy | ");
//				evalOutput.CopyToAsync(currentBinding->outputCache);
//			}
//
//			auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - now);
//			// Convert to FPS: milli to seconds, invert
//			OutputDebugString(L"PF End ");
//			});
//	}
//	if (bindings[finishedFrameIndex]->outputCache != nullptr) {
//		OutputDebugString(L"\nStart CopyAsync ");
//		OutputDebugString(std::to_wstring(finishedFrameIndex).c_str());
//		{
//			// Lock so that don't have multiple sources copying to output at once
//			std::lock_guard<std::mutex> guard{ Processing };
//			bindings[finishedFrameIndex]->outputCache.CopyToAsync(output).get();
//		}
//		OutputDebugString(L" | Stop CopyAsync\n");
//	}
//	// return without waiting for the submit to finish, setup the completion handler
//}

void SegmentModel::RunStyleTransfer(IDirect3DSurface src, IDirect3DSurface dest, IDirect3DDevice device) 
{
	VideoFrame input = VideoFrame::CreateWithDirect3D11Surface(src);
	VideoFrame output = VideoFrame::CreateWithDirect3D11Surface(dest);

	auto desc = input.Direct3DSurface().Description();
	auto descOut = output.Direct3DSurface().Description();

	VideoFrame output2 = VideoFrame::CreateAsDirect3D11SurfaceBacked(descOut.Format, 720, 720, device);
	VideoFrame input2 = VideoFrame::CreateAsDirect3D11SurfaceBacked(desc.Format, 720,720, device);
	input.CopyToAsync(input2).get(); // TODO: Can input stay the same if NV12? 
	output.CopyToAsync(output2).get();
	desc = input2.Direct3DSurface().Description();

	//TODO: If want to use swapchain comment out these two lines. 
	/*SubmitEval(input2, output);
	swapChainIndex = (++swapChainIndex) % swapChainEntryCount;*/

	hstring inputName = m_sessStyleTransfer.Model().InputFeatures().GetAt(0).Name();
	m_bindStyleTransfer.Bind(inputName, input2);
	hstring outputName = m_sessStyleTransfer.Model().OutputFeatures().GetAt(0).Name();

	auto outputBindProperties = PropertySet();
	m_bindStyleTransfer.Bind(outputName, output2); // TODO: See if can bind videoframe from MFT
	auto results = m_sessStyleTransfer.Evaluate(m_bindStyleTransfer, L"");

	output2.CopyToAsync(output).get(); // Should put onto the correct surface now? Make sure, can return the surface instead later
	m_bindStyleTransfer.Clear();
	input.Close();
	input2.Close();
	output2.Close();
	output.Close();
}


void SegmentModel::RunTestDXGI(IDirect3DSurface src, IDirect3DSurface dest, IDirect3DDevice device)
{

	OutputDebugString(L"\n [ Starting runTest | "); i++;
	printf("[ Starting runtest %d| ", i);
	VideoFrame input = VideoFrame::CreateWithDirect3D11Surface(src);
	VideoFrame output = VideoFrame::CreateWithDirect3D11Surface(dest);
	
	auto desc = input.Direct3DSurface().Description(); 
	auto descOut = output.Direct3DSurface().Description();

	//  TODO: Use a specific device to create so not piling up on resources? 
	VideoFrame output2 = VideoFrame::CreateAsDirect3D11SurfaceBacked(descOut.Format, descOut.Width, descOut.Height, device);
	VideoFrame input2 = VideoFrame::CreateAsDirect3D11SurfaceBacked(desc.Format, desc.Width, desc.Height, device);
	input.CopyToAsync(input2).get(); // TODO: Can input stay the same if NV12? 
	output.CopyToAsync(output2).get();
	desc = input2.Direct3DSurface().Description();

	auto binding = LearningModelBinding(m_sess);
	
	hstring inputName = m_sess.Model().InputFeatures().GetAt(0).Name();
	binding.Bind(inputName, input2);		
	hstring outputName = m_sess.Model().OutputFeatures().GetAt(0).Name();

	auto outputBindProperties = PropertySet();
	binding.Bind(outputName, output2); // TODO: See if can bind videoframe from MFT
	auto results = m_sess.Evaluate(binding, L"");

	output2.CopyToAsync(output).get(); // Should put onto the correct surface now? Make sure, can return the surface instead later

	binding.Clear();
	input.Close();
	input2.Close();
	output2.Close();
	output.Close();
	OutputDebugString(L" Ending runTest ]");
	//printf(" Ending runtest %d]", i);

}

LearningModel SegmentModel::Invert(long n, long c, long h, long w)
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

LearningModel SegmentModel::PostProcess(long n, long c, long h, long w, long axis)
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

LearningModel SegmentModel::FCNResnet()
{
	// TODO: Add FCN-resnet to LargeModels if want to use for published streaming sample, or remove. 
	return LearningModel::LoadFromFilePath(L"C:\\Windows-Machine-Learning\\Samples\\BackgroundBlur\\BackgroundBlur\\Assets\\fcn-resnet50-11.onnx");
}


LearningModel SegmentModel::StyleTransfer()
{
	auto rel = std::filesystem::path(modelPath.c_str());
	rel.append("mosaic.onnx");
	return LearningModel::LoadFromFilePath(rel + L"");
}

LearningModel SegmentModel::Normalize0_1ThenZScore(long h, long w, long c, const std::array<float, 3>& means, const std::array<float, 3>& stddev)
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

LearningModel SegmentModel::ReshapeFlatBufferToNCHW(long n, long c, long h, long w)
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

LearningModelSession SegmentModel::CreateLearningModelSession(const LearningModel& model, bool closeModel) {
	auto device = m_useGPU ? LearningModelDevice(LearningModelDeviceKind::DirectX) : LearningModelDevice(LearningModelDeviceKind::Default); // Todo: Have a toggle between GPU/ CPU? 
	auto options = LearningModelSessionOptions(); 
	options.BatchSizeOverride(0);
	options.CloseModelOnSessionCreation(closeModel);
	auto session = LearningModelSession(model, device);
	return session;
}

void SegmentModel::EvaluateInternal(LearningModelSession sess, LearningModelBinding bind, bool wait)
{
	auto results = sess.Evaluate(bind, L"");
	/*auto results = sess.EvaluateAsync(bind, L"");
	if (wait) {
		results.GetResults(); // TODO: Will this actually wait?
	}*/
}

LearningModelBinding SegmentModel::Evaluate(LearningModelSession& sess,const std::vector<ITensor*>& input, ITensor* output, bool wait) 
{
	auto binding = LearningModelBinding(sess);

	for (int i = 0; i < input.size(); i++)
	{
		hstring inputName = sess.Model().InputFeatures().GetAt(i).Name();
		binding.Bind(inputName, *input[i]);
	}
	hstring outputName = sess.Model().OutputFeatures().GetAt(0).Name();

	auto outputBindProperties = PropertySet();
	outputBindProperties.Insert(L"DisableTensorCpuSync", PropertyValue::CreateBoolean(!wait));
	binding.Bind(outputName, *output, outputBindProperties);
	//EvaluateInternal(sess, binding);

	/*auto results = sess.Evaluate(binding, L"");
	auto resultTensor = results.Outputs().Lookup(outputName).try_as<TensorFloat>();
	float testPixels[6];
	if (resultTensor) {
		auto resultVector = resultTensor.GetAsVectorView();
		resultVector.GetMany(0, testPixels);
	}*/

	return binding;
}

