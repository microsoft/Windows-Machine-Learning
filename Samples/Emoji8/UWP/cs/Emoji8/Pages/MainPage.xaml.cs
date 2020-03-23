// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT license. 

using Emoji8.Services;
using Microsoft.Toolkit.Uwp.Helpers;
using Microsoft.Toolkit.Uwp.UI.Animations;
using System;
using System.Diagnostics;
using System.Threading.Tasks;
using Windows.ApplicationModel;
using Windows.ApplicationModel.Core;
using Windows.Foundation;
using Windows.UI.ViewManagement;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace Emoji8
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        private bool _isInitialized = false;

        public MainPage()
        {
            CurrentEmojis._emojis = new EmojiCollection();
            this.InitializeComponent();
            CarouselControl.ItemsSource = CurrentEmojis._emojis.Emojis;
            CurrentEmojis._currentEmoji = null;

            ApplicationView.GetForCurrentView().SetPreferredMinSize(new Size(500,500));
            Application.Current.Suspending += Current_Suspending;
            Application.Current.Resuming += Current_Resuming;

            instructions.Text = GameText.LOADER.GetString("FirstScreenInstructionsBeforeInitialization");
        }

        private async void Current_Suspending(object sender, SuspendingEventArgs e)
        {
            Debug.WriteLine("Current_Suspending");
            var deferral = e.SuspendingOperation.GetDeferral();
            StartButton.IsEnabled = false;

            if (_isInitialized)
            {
                Debug.WriteLine($"Calling CleanUp, {DateTime.Now}");
                await CleanUpAsync();
                Debug.WriteLine($"CleanUp Completed, {DateTime.Now}");
            }

            deferral.Complete();
            Debug.WriteLine("Deferral completed");

        }

        private async void Start_Click(object sender, RoutedEventArgs e)
        {
            if (_isInitialized)
            {
                Application.Current.Resuming -= Current_Resuming;
                Application.Current.Suspending -= Current_Suspending;

                StartButton.IsEnabled = false;
                Debug.WriteLine("About to clean up MainPage since Start was pressed");
                await CleanUpAsync();
                Debug.WriteLine("Finished cleaning up MainPage, ready to move onto EmotionPage");
            }

            Debug.WriteLine(GameText.LOADER.GetString("LoggerStartButtonClicked"));
            Frame.Navigate(typeof(EmotionPage));
        }

        protected override async void OnNavigatedTo(NavigationEventArgs e)
        {
            StartButton.IsEnabled = false;
            if (_isInitialized)
            {
                await CleanUpAsync();
            }
            await InitializeScreenAsync();
        }

        private async void Current_Resuming(object sender, object e)
        {
            if (!_isInitialized)
            {
                await InitializeScreenAsync();
            }
        }

        private async Task InitializeScreenAsync()
        { 
            CameraHelperResult result = await CameraService.Current.InitializeAsync();
            if (result != CameraHelperResult.Success)
            {
                await MessageDialogService.Current.WriteMessage(result.ToString() + GameText.LOADER.GetString("CameraHelperResultFailed"));
                return;
            }

            bool isModelLoaded = await IntelligenceService.Current.InitializeAsync();
            if (!isModelLoaded)
            {
                await MessageDialogService.Current.WriteMessage(GameText.LOADER.GetString("ModelLoadedFailed"));
                return;
            }
            IntelligenceService.Current.IntelligenceServiceEmotionClassified += Current_IntelligenceServiceEmotionClassified;

            await instructions.Fade(value: 0.0f, duration: 1000, delay: 0).StartAsync();
            instructions.Text = GameText.LOADER.GetString("FirstScreenInstructionsAfterInitialization");
            await instructions.Fade(value: 1.0f, duration: 250, delay: 0).StartAsync();

            _isInitialized = true;
            StartButton.IsEnabled = true;
        }

        public void Current_IntelligenceServiceEmotionClassified(object sender, ClassifiedEmojiEventArgs e)
        {
            CurrentEmojis._currentEmoji = e.ClassifiedEmoji;
            UpdateCarouselAsync();
        }

        public async void UpdateCarouselAsync()
        {
            await CoreApplication.MainView.CoreWindow.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                CarouselControl.SelectedIndex = CurrentEmojis._emojis.Emojis.IndexOf(CurrentEmojis._currentEmoji);
            });
        }

        private async Task CleanUpAsync()
        {
            _isInitialized = false;

            await CameraService.Current.CleanUpAsync();
            IntelligenceService.Current.CleanUp();
                        
            IntelligenceService.Current.IntelligenceServiceEmotionClassified -= Current_IntelligenceServiceEmotionClassified;
                        
            CurrentEmojis._currentEmoji = null;
        }

    }
}