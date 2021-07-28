//Specify all the using statements which give us the access to all the APIs that we'll need
using System;
using System.Threading.Tasks;
using Windows.AI.MachineLearning;
using Windows.Graphics.Imaging;
using Windows.Media;
using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.Storage.Streams;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media.Imaging;


namespace classifierPyTorch
{
    public sealed partial class MainPage : Page
    {
        // All the required fields declaration
        private ImageClassifierModel modelGen;
        private ImageClassifierInput image = new ImageClassifierInput();
        private ImageClassifierOutput results;
        private StorageFile selectedStorageFile;
        private string label = "";
        private float probability = 0;
        private Helper helper = new Helper();

        public enum Labels
        {            
            plane,
            car,
            bird,
            cat,
            deer,
            dog,
            frog,
            horse,
            ship,
            truck
        }

        // The main page to initialize and execute the model.
        public MainPage()
        {
            this.InitializeComponent();
            loadModel();
        }

        // A method to load a machine learning model.
        private async Task loadModel()
        {
            // Get an access the ONNX model and save it in memory. 
            StorageFile modelFile = await StorageFile.GetFileFromApplicationUriAsync(new Uri($"ms-appx:///Assets/ImageClassifier.onnx"));
            // Instantiate the model. 
            modelGen = await ImageClassifierModel.CreateFromStreamAsync(modelFile);
        }

        // Waiting for a click event to select a file 
        private async void OpenFileButton_Click(object sender, RoutedEventArgs e)
        {
            if (!await getImage())
            {
                return;
            }
            // After the click event happened and an input selected, we begin the model execution. 
            // Bind the model input
            await imageBind();
            // Model evaluation
            await evaluate();
            // Extract the results
            extractResult();
            // Display the results  
            await displayResult();
        }

        // A method to select an input image file
        private async Task<bool> getImage()
        {
            try
            {
                // Trigger file picker to select an image file
                FileOpenPicker fileOpenPicker = new FileOpenPicker();
                fileOpenPicker.SuggestedStartLocation = PickerLocationId.PicturesLibrary;
                fileOpenPicker.FileTypeFilter.Add(".jpg");
                fileOpenPicker.FileTypeFilter.Add(".png");
                fileOpenPicker.ViewMode = PickerViewMode.Thumbnail;
                selectedStorageFile = await fileOpenPicker.PickSingleFileAsync();
                if (selectedStorageFile == null)
                {
                    return false;
                }
            }
            catch (Exception)
            {
                return false;
            }
            return true;
        }

        // A method to convert and bind the input image.   
        private async Task imageBind()
        {
            UIPreviewImage.Source = null;

            try
            {
                SoftwareBitmap softwareBitmap;

                using (IRandomAccessStream stream = await selectedStorageFile.OpenAsync(FileAccessMode.Read))
                {
                    
                    // Create the decoder from the stream 
                    BitmapDecoder decoder = await BitmapDecoder.CreateAsync(stream);

                    // Get the SoftwareBitmap representation of the file in BGRA8 format
                    softwareBitmap = await decoder.GetSoftwareBitmapAsync();
                    softwareBitmap = SoftwareBitmap.Convert(softwareBitmap, BitmapPixelFormat.Bgra8, BitmapAlphaMode.Premultiplied);
                }
                
                // Display the image
                SoftwareBitmapSource imageSource = new SoftwareBitmapSource();
                await imageSource.SetBitmapAsync(softwareBitmap);
                UIPreviewImage.Source = imageSource;

                // Encapsulate the image within a VideoFrame to be bound and evaluated
                VideoFrame inputImage = VideoFrame.CreateWithSoftwareBitmap(softwareBitmap);
                // Resize the image size to 32x32 using the Helper we have defined earlier.
                inputImage=await helper.CropAndDisplayInputImageAsync(inputImage);
                // Bind the model input
                ImageFeatureValue imageTensor = ImageFeatureValue.CreateFromVideoFrame(inputImage);
                image.modelInput = imageTensor;
            }
            catch (Exception e)
            {
            }
        }

        // A method to evaluate the model
        private async Task evaluate()
        {
            results = await modelGen.EvaluateAsync(image);
        }

        // A method to extract the output from the the model 
        private void extractResult()
        {
            // Retrieve the results of evaluation
            var mResult = results.modelOutput as TensorFloat;
            // Convert the result to vector format
            var resultVector = mResult.GetAsVectorView();

            probability = 0;
            int index = 0;
            // find the maximum "energy" of the label
            for (int i = 0; i < resultVector.Count; i++)
            {
                var elementProbability = resultVector[i];

                if (elementProbability > probability)
                {
                    index = i;
                    probability = elementProbability;
                }
                System.Diagnostics.Debug.WriteLine(i+" "+ elementProbability);
            }
            label = ((Labels)index).ToString();
        }

        // A method to display the result
        private async Task displayResult()
        {
            displayOutput.Text = label;
        }
    }

}
