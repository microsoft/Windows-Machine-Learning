using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Input;
using AudioPreprocessing.Model;
using Microsoft.UI.Xaml.Media.Imaging;
using Windows.Graphics.Imaging;
using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.Storage.Streams;
using Windows.UI.Core;

namespace AudioPreprocessing.ViewModel
{
    // Implements INotifyPropertyChanged interface to support bindings
    public class PreprocessViewModel : INotifyPropertyChanged

    {
        private string audioPath;
        private string imagePath;
        private string test;
        private SoftwareBitmapSource melSpectrogramImageSource;
        private SoftwareBitmap melSpectrogramImage;

        public string Test
        {
            get { return test; }
            set { test = value; OnPropertyChanged(); }
        }
        public PreprocessViewModel()
        {
            PreprocessModel preprocessModel = new PreprocessModel();
            audioPath = preprocessModel.AudioPath;
            imagePath = preprocessModel.MelSpecImagePath;
            test = "Hello World";
            //melSpectrogramImage = GetSampleBitmap().Result;
            MelSpectrogramImageSource = new SoftwareBitmapSource();
            //MelSpectrogramImageSource.SetBitmapAsync(melSpectrogramImage).GetResults();
        }

        public async Task<SoftwareBitmap> GetSampleBitmap()
        {
            SoftwareBitmap softwareBitmap;
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
                softwareBitmap = await decoder.GetSoftwareBitmapAsync();
            }
            return softwareBitmap;
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

        public SoftwareBitmapSource MelSpectrogramImageSource
        {
            get { return melSpectrogramImageSource; }
            set { melSpectrogramImageSource = value; OnPropertyChanged(); }
        }


        //public ICommand OpenFileCommand => new AsyncRelayCommand(p => OpenFile());

        private async Task<string> OpenFile()
        {
            var openFileDialog = new OpenFileDialog();
            openFileDialog.Title = "Choose an Audio File";
            openFileDialog.Filter = "sound files (*.wav)|*.wav|All files (*.*)|*.*";

            if (openFileDialog.ShowDialog() == DialogResult.OK)
            {
                AudioPath = openFileDialog.FileName;
            }
            return AudioPath;
        }

        public SoftwareBitmap ProcessFile()
        {
            PreprocessModel melSpec = new PreprocessModel();
            var openFileDialog = new OpenFileDialog();
            openFileDialog.Title = "Choose an Audio File";
            openFileDialog.Filter = "sound files (*.wav)|*.wav|All files (*.*)|*.*";

            if (openFileDialog.ShowDialog() == DialogResult.OK)
            {
                AudioPath = openFileDialog.FileName;
                melSpectrogramImage = melSpec.GenerateMelSpectrogram(AudioPath);
            }
            return melSpectrogramImage;
        }

        public SoftwareBitmap ProcessFile2()
        {
            PreprocessModel melSpec = new PreprocessModel();
            var openFileDialog = new OpenFileDialog();
            openFileDialog.Title = "Choose an Audio File";
            openFileDialog.Filter = "sound files (*.wav)|*.wav|All files (*.*)|*.*";

            if (openFileDialog.ShowDialog() == DialogResult.OK)
            {
                AudioPath = openFileDialog.FileName;
                melSpectrogramImage = melSpec.GenerateMelSpectrogram(AudioPath);
            }
            return melSpectrogramImage;
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

                // Set additional encoding parameters, if needed
                encoder.BitmapTransform.ScaledWidth = 320;
                encoder.BitmapTransform.ScaledHeight = 240;
                encoder.BitmapTransform.Rotation = Windows.Graphics.Imaging.BitmapRotation.Clockwise90Degrees;
                encoder.BitmapTransform.InterpolationMode = BitmapInterpolationMode.Fant;
                encoder.IsThumbnailGenerated = true;

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

