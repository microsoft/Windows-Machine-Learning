using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace Iris_Data_Analysis
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        private IrisModel _iris_model;

        public MainPage()
        {
            this.InitializeComponent();
            _iris_model = new IrisModel();
#pragma warning disable CS4014 // Because this call is not awaited, execution of the current method continues before the call is completed
            _iris_model.Initialize();
#pragma warning restore CS4014 // Because this call is not awaited, execution of the current method continues before the call is completed
        }

        private void sepal_length_input_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            if (_iris_model != null)
            {
                _iris_model.Sepal_Length = (float)sepal_length_input.Value / 10.0f;
                model_output.Text = _iris_model.Evaluate();
            }
        }

        private void sepal_width_input_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            if (_iris_model != null)
            {
                _iris_model.Sepal_Width = (float)sepal_width_input.Value / 10.0f;
                model_output.Text = _iris_model.Evaluate();
            }
        }

        private void petal_length_input_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            if (_iris_model != null)
            {
                _iris_model.Petal_Length = (float)petal_length_input.Value / 10.0f;
                model_output.Text = _iris_model.Evaluate();
            }
        }

        private void petal_width_input_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            if (_iris_model != null)
            {
                _iris_model.Petal_Width = (float)petal_width_input.Value / 10.0f;
                model_output.Text = _iris_model.Evaluate();
            }
        }
    }
}
