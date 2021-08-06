using System;
using System.Linq;
using System.Threading.Tasks;
using Windows.AI.MachineLearning;
using Windows.Storage;

namespace Iris_Data_Analysis
{
    class IrisModel
    {
        private LearningModel _learning_model;
        private LearningModelSession _session;
        private String[] _labels = { "Iris-setosa", "Iris-versicolor", "Iris-virginica"};

        public async Task Initialize()
        {
            // Load and create the model and session
            var modelFile = await StorageFile.GetFileFromApplicationUriAsync(new Uri($"ms-appx:///Assets//Regression.onnx"));
            _learning_model = await LearningModel.LoadFromStorageFileAsync(modelFile);
            _session = new LearningModelSession(_learning_model);
        }

        private float _sepal_length = 1.0f;
        public float Sepal_Length
        {
            get
            {
                return _sepal_length;
            }
            set
            {
                // validate range [1,10]
                if (value >= 1 && value <= 10)
                {
                    _sepal_length = value;
                }
            }
        }

        private float _sepal_width = 1.0f;
        public float Sepal_Width
        {
            get
            {
                return _sepal_width;
            }
            set
            {
                // validate range [1, 8]
                if (value >= 1 && value <= 8)
                {
                    _sepal_width = value;
                }
            }
        }

        private float _petal_length = 0.5f;
        public float Petal_Length
        {
            get
            {
                return _petal_length;
            }
            set
            {
                // validate range [0.5, 10]
                if (value >= 0.5 && value <= 10)
                {
                    _petal_length = value;
                }
            }
        }

        private float _petal_width = 0.1f;
        public float Petal_Width
        {
            get
            {
                return _petal_width;
            }
            set
            {
                // validate range [0.1, 5]
                if (value >= 0.1 && value <= 5)
                {
                    _petal_width = value;
                }
            }
        }

        internal String Evaluate()
        {
            // input tensor shape is [1x4]
            long[] shape = new long[2];
            shape[0] = 1;
            shape[1] = 4;

            // set up the input tensor
            float[] input_data = new float[4];
            input_data[0] = _sepal_length;
            input_data[1] = _sepal_width;
            input_data[2] = _petal_length;
            input_data[3] = _petal_width;
            TensorFloat tensor_float = TensorFloat.CreateFromArray(shape, input_data);

            // bind the tensor to "input"
            var binding = new LearningModelBinding(_session);
            binding.Bind("input", tensor_float);

            // evaluate
            var results = _session.Evaluate(binding, "");

            // get the results
            TensorFloat prediction = (TensorFloat)results.Outputs.First().Value;
            var prediction_data = prediction.GetAsVectorView();

            // find the highest predicted value
            int max_index = 0;
            float max_value = 0;
            for (int i = 0; i < prediction_data.Count; i++)
            {
                var val = prediction_data.ElementAt(i);
                if (val > max_value)
                {
                    max_value = val;
                    max_index = i;
                }
            }

            // return the label corresponding to the highest predicted value
            return _labels.ElementAt(max_index);
        }
    }
}
