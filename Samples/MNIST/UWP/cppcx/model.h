#include <ppltasks.h>
#include <collection.h>

ref class coreml_MNISTInput sealed
{
public:
    property ::Windows::AI::MachineLearning::ImageFeatureValue^ image; // shape(-1,1,28,28)
};

ref class coreml_MNISTOutput sealed
{
public:
    property ::Windows::AI::MachineLearning::TensorInt64Bit^ classLabel; // shape(-1,1)
    property ::Windows::Foundation::Collections::IVector<::Windows::Foundation::Collections::IMap<int64_t, float>^>^ prediction;
};

ref class coreml_MNISTModel sealed
{
private:
    ::Windows::AI::MachineLearning::LearningModel^ m_model;
    ::Windows::AI::MachineLearning::LearningModelSession^ m_session;
    ::Windows::AI::MachineLearning::LearningModelBinding^ m_binding;
public:
    property ::Windows::AI::MachineLearning::LearningModel^ LearningModel { ::Windows::AI::MachineLearning::LearningModel^ get() { return m_model; } }
    property ::Windows::AI::MachineLearning::LearningModelSession^ LearningModelSession { ::Windows::AI::MachineLearning::LearningModelSession^ get() { return m_session; } }
    property ::Windows::AI::MachineLearning::LearningModelBinding^ LearningModelBinding { ::Windows::AI::MachineLearning::LearningModelBinding^ get() { return m_binding; } }

    static ::Windows::Foundation::IAsyncOperation<coreml_MNISTModel^>^ CreateFromStreamAsync(::Windows::Storage::Streams::IRandomAccessStreamReference^ stream, ::Windows::AI::MachineLearning::LearningModelDeviceKind deviceKind = ::Windows::AI::MachineLearning::LearningModelDeviceKind::Cpu)
    {
        return ::concurrency::create_async([stream, deviceKind] {
            return ::concurrency::create_task(::Windows::AI::MachineLearning::LearningModel::LoadFromStreamAsync(stream))
                .then([deviceKind](::Windows::AI::MachineLearning::LearningModel^ learningModel) {
                    coreml_MNISTModel^ model = ref new coreml_MNISTModel();
                    model->m_model = learningModel;
                    ::Windows::AI::MachineLearning::LearningModelDevice^ device = ref new ::Windows::AI::MachineLearning::LearningModelDevice(deviceKind);
                    model->m_session = ref new ::Windows::AI::MachineLearning::LearningModelSession(model->m_model, device);
                    model->m_binding = ref new ::Windows::AI::MachineLearning::LearningModelBinding(model->m_session);
                    return model;
            });
        });
    }

    ::Windows::Foundation::IAsyncOperation<coreml_MNISTOutput^>^ EvaluateAsync(coreml_MNISTInput^ input)
    {
        if (this->m_model == nullptr) throw ref new Platform::InvalidArgumentException();
        return ::concurrency::create_async([this, input] {
            return ::concurrency::create_task([this, input]() {
                m_binding->Bind("image", input->image);
                return ::concurrency::create_task(this->m_session->EvaluateAsync(m_binding, L""));
            }).then([](::Windows::AI::MachineLearning::LearningModelEvaluationResult^ evalResult) {
                    coreml_MNISTOutput^ output = ref new coreml_MNISTOutput();
                    output->classLabel = static_cast<::Windows::AI::MachineLearning::TensorInt64Bit^>(evalResult->Outputs->Lookup("classLabel"));
                    output->prediction = static_cast<::Windows::Foundation::Collections::IVector<::Windows::Foundation::Collections::IMap<int64_t, float>^>^>(evalResult->Outputs->Lookup("prediction"));
                    return output;
            });
        });
    }
};