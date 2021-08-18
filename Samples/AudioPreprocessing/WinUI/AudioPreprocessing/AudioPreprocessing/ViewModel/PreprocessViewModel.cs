using AudioPreprocessing.Model;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Windows.Input;
using Windows.Graphics.Imaging;
using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.Storage.Streams;

namespace AudioPreprocessing.ViewModel
{
    public class PreprocessViewModel : INotifyPropertyChanged

    {
        private string audioPath;
        private string imagePath;
        private MelSpectrogramModel preprocessModel;
        private SoftwareBitmap melSpectrogramImage;
        private string results;
        private IReadOnlyList<float> resultVector;
        

        public PreprocessViewModel()
        {
            preprocessModel = new MelSpectrogramModel(new ModelSetting());
            audioPath = preprocessModel.AudioPath;
            imagePath = preprocessModel.MelSpecImagePath;
        }

        public async Task<SoftwareBitmap> GetSampleBitmap()
        {

            FileOpenPicker fileOpenPicker = new FileOpenPicker();
            fileOpenPicker.SuggestedStartLocation = PickerLocationId.PicturesLibrary;
            fileOpenPicker.FileTypeFilter.Add(".jpg");
            fileOpenPicker.ViewMode = PickerViewMode.Thumbnail;

            var inputFile = await fileOpenPicker.PickSingleFileAsync();

            if (inputFile == null)
            {
                // The user cancelled the picking operation
                return null;
            }
            using (IRandomAccessStream stream = await inputFile.OpenAsync(FileAccessMode.Read))
            {
                // Create the decoder from the stream
                BitmapDecoder decoder = await BitmapDecoder.CreateAsync(stream);

                // Get the SoftwareBitmap representation of the file
                return await decoder.GetSoftwareBitmapAsync();
            }
        }

        public string AudioPath
        {
            get { return audioPath; }
            set { audioPath = value; OnPropertyChanged(); }
        }
        public string ImagePath
        {
            get { return imagePath; }
            set { imagePath = value; OnPropertyChanged(); }
        }
        public SoftwareBitmap MelSpectrogramImage
        {
            get { return melSpectrogramImage; }
            set { melSpectrogramImage = value; OnPropertyChanged(); }
        }
        public string Results
        {
            get { return results; }
            set { results = value; OnPropertyChanged(); }
        }

        public void GenerateAndEvalAudio(string wavPath, ModelSetting settings)
        {
            bool colorize = settings.Colored;
            preprocessModel = new MelSpectrogramModel(settings);
            MelSpectrogramImage = preprocessModel.GenerateMelSpectrogram(wavPath);
            resultVector = preprocessModel.ClassifySpectrogram(MelSpectrogramImage);
            GetLabelledResults(resultVector);
            if (colorize)
            {
                // Use computational graph to colorize image
                //MelSpectrogramImage = PreprocessModel.ColorizeWithComputationalGraph(MelSpectrogramImage);
                // Use bitmap editing, pixel by pixel, to colorize image
                MelSpectrogramModel.ColorizeWithBitmapEditing(MelSpectrogramImage);
            }
            AudioPath = wavPath;

            //Image control only accepts BGRA8 encoding and Premultiplied/no alpha channel. This checks and converts
            //the SoftwareBitmap we want to bind.
            if (MelSpectrogramImage.BitmapPixelFormat != BitmapPixelFormat.Bgra8 ||
                MelSpectrogramImage.BitmapAlphaMode != BitmapAlphaMode.Premultiplied)
            {
                MelSpectrogramImage = SoftwareBitmap.Convert(MelSpectrogramImage, BitmapPixelFormat.Bgra8, BitmapAlphaMode.Premultiplied);
            }

        }

        private void GetLabelledResults(IReadOnlyList<float> resultVector)
        {
            var labels = new string[]{"backward", "bed", "bird", "cat", "dog", "down", "eight", "five", "follow",
                     "forward", "four", "go", "happy", "house", "learn", "left", "marvin",
                     "nine", "no", "off", "on", "one", "right", "seven", "sheila",
                     "six", "stop", "three", "tree", "two", "up", "visual", "wow", "yes", "zero" };

            List<(int index, float probability)> indexedResults = new List<(int, float)>();

            for (int i = 0; i < resultVector.Count; i++)
            {
                indexedResults.Add((index: i, probability: resultVector[i]));
            }
            indexedResults.Sort((a, b) =>
            {
                if (a.probability < b.probability)
                {
                    return 1;
                }
                else if (a.probability > b.probability)
                {
                    return -1;
                }
                else
                {
                    return 0;
                }
            });

            var topResults = new List<string>();
            for (int i = 0; i < 3; i++)
            {
                topResults.Add($"\"{ labels[indexedResults[i].index]}\" with confidence of { indexedResults[i].probability}");
            }
            Results = string.Join('\n', topResults);
        }

        public ICommand SaveFileCommand => new AsyncRelayCommand(p => SaveFile());

        private async Task SaveFile()
        {
            FileSavePicker fileSavePicker = new FileSavePicker();
            fileSavePicker.SuggestedStartLocation = PickerLocationId.PicturesLibrary;
            fileSavePicker.FileTypeChoices.Add("JPEG files", new List<string>() { ".jpg" });
            fileSavePicker.SuggestedFileName = "image";

            var outputFile = await fileSavePicker.PickSaveFileAsync();

            if (outputFile == null)
            {
                // The user cancelled the picking operation
                return;
            }
            else { SaveSoftwareBitmapToFile(melSpectrogramImage, outputFile); }
        }

        private async void SaveSoftwareBitmapToFile(SoftwareBitmap softwareBitmap, StorageFile outputFile)
        {
            using (IRandomAccessStream stream = await outputFile.OpenAsync(FileAccessMode.ReadWrite))
            {
                // Create an encoder with the desired format
                BitmapEncoder encoder = await BitmapEncoder.CreateAsync(BitmapEncoder.JpegEncoderId, stream);

                // Set the software bitmap
                encoder.SetSoftwareBitmap(softwareBitmap);

                try
                {
                    await encoder.FlushAsync();
                }
                catch (Exception err)
                {
                    const int WINCODEC_ERR_UNSUPPORTEDOPERATION = unchecked((int)0x88982F81);
                    switch (err.HResult)
                    {
                        case WINCODEC_ERR_UNSUPPORTEDOPERATION:
                            // If the encoder does not support writing a thumbnail, then try again
                            // but disable thumbnail generation.
                            encoder.IsThumbnailGenerated = false;
                            break;
                        default:
                            throw;
                    }
                }

                if (encoder.IsThumbnailGenerated == false)
                {
                    await encoder.FlushAsync();
                }
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged([CallerMemberName] string name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

    }
}

