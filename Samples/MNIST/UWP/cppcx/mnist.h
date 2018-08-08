#include <ppltasks.h>
#include <collection.h>
namespace mnist_cppcx
{

	ref class mnistInput sealed
	{
	public:
		property ::Windows::AI::MachineLearning::ImageFeatureValue^ Input3; // BitmapPixelFormat: Gray8, BitmapAlphaMode: Premultiplied, width: 28, height: 28
	};

	ref class mnistOutput sealed
	{
	public:
		property ::Windows::AI::MachineLearning::TensorFloat^ Plus214_Output_0; // shape(1,10)
	};

	ref class mnistModel sealed
	{
	private:
		::Windows::AI::MachineLearning::LearningModel^ m_model;
		::Windows::AI::MachineLearning::LearningModelSession^ m_session;
		::Windows::AI::MachineLearning::LearningModelBinding^ m_binding;
	public:
		property ::Windows::AI::MachineLearning::LearningModel^ LearningModel { ::Windows::AI::MachineLearning::LearningModel^ get() { return m_model; } }
		property ::Windows::AI::MachineLearning::LearningModelSession^ LearningModelSession { ::Windows::AI::MachineLearning::LearningModelSession^ get() { return m_session; } }
		property ::Windows::AI::MachineLearning::LearningModelBinding^ LearningModelBinding { ::Windows::AI::MachineLearning::LearningModelBinding^ get() { return m_binding; } }
		static ::Windows::Foundation::IAsyncOperation<mnistModel^>^ CreateFromStreamAsync(::Windows::Storage::Streams::IRandomAccessStreamReference^ stream, ::Windows::AI::MachineLearning::LearningModelDeviceKind deviceKind = ::Windows::AI::MachineLearning::LearningModelDeviceKind::Default)
		{
			return ::concurrency::create_async([stream, deviceKind] {
				return ::concurrency::create_task(::Windows::AI::MachineLearning::LearningModel::LoadFromStreamAsync(stream))
					.then([deviceKind](::Windows::AI::MachineLearning::LearningModel^ learningModel) {
					mnistModel^ model = ref new mnistModel();
					model->m_model = learningModel;
					::Windows::AI::MachineLearning::LearningModelDevice^ device = ref new ::Windows::AI::MachineLearning::LearningModelDevice(deviceKind);
					model->m_session = ref new ::Windows::AI::MachineLearning::LearningModelSession(model->m_model, device);
					model->m_binding = ref new ::Windows::AI::MachineLearning::LearningModelBinding(model->m_session);
					return model;
				});
			});
		}
		::Windows::Foundation::IAsyncOperation<mnistOutput^>^ EvaluateAsync(mnistInput^ input)
		{
			if (this->m_model == nullptr) throw ref new Platform::InvalidArgumentException();
			return ::concurrency::create_async([this, input] {
				return ::concurrency::create_task([this, input]() {
					m_binding->Bind("Input3", input->Input3);
					return ::concurrency::create_task(this->m_session->EvaluateAsync(m_binding, L""));
				}).then([](::Windows::AI::MachineLearning::LearningModelEvaluationResult^ evalResult) {
					mnistOutput^ output = ref new mnistOutput();
					output->Plus214_Output_0 = static_cast<::Windows::AI::MachineLearning::TensorFloat^>(evalResult->Outputs->Lookup("Plus214_Output_0"));
					return output;
				});
			});
		}
	};
}
