// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT license. 

using Emoji8.Services;
using Microsoft.Toolkit.Uwp.Helpers;
using Microsoft.Toolkit.Uwp.UI.Animations;
using Microsoft.Toolkit.Uwp.UI.Controls;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading.Tasks;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace Emoji8
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class EmotionPage : Page
    {
        private static List<Emoji> _resultsEmojis { get; set; }
        private DispatcherTimer EmojiTimer;
        private int countdownNum;
        private int numEmojisLeftToPerform = Constants.EMOJI_NUMBER_TO_DISPLAY;
        private double nextPercentage;
        private double smoothDelta = 0.01;
        private bool _isInitialized = false;
        private bool debugMode = false;
        private bool _isPaused = false;
        private DateTime _pageNavigatedToTime;

        private const double BAD_COLOR_THRESHOLD = 0.33;
        private const double OKAY_COLOR_THRESHOLD = 0.66;
        //the "good" performance threshold is derived from the other two

        public EmotionPage()
        {
            this.InitializeComponent();
            LoadingMessage.Text = GameText.LOADER.GetString("SecondScreenInstructions");
        }

        private async void Current_Resuming(object sender, object e)
        {
            //reinitialize camera & resume timer
            if (!_isInitialized)
            {
                await InitializeScreenAsync();
                if (!_isPaused) InitializeTimer();
            }
            _isPaused = false;
            EmojiTimer.Start();           
        }

        private async void Current_Suspending(object sender, Windows.ApplicationModel.SuspendingEventArgs e)
        {
            //pause timer & clean up camera
            var deferral = e.SuspendingOperation.GetDeferral();

            //on resuming make sure that the results information & timer progress is not lost
            _isPaused = true;

            if (EmojiTimer != null ) EmojiTimer.Stop();
            if (_isInitialized)
            {
                Debug.WriteLine($"Calling CleanUpCamera, {DateTime.Now}");
                await CleanUpCameraAsync();
                Debug.WriteLine($"CleanUpCamera Completed, {DateTime.Now}");
            }
            deferral.Complete();
        }

        protected override async void OnNavigatedTo(NavigationEventArgs e)
        {
            Application.Current.Suspending += Current_Suspending;
            Application.Current.Resuming += Current_Resuming;
            _pageNavigatedToTime = DateTime.Now;
            if (_isInitialized)
            {
                await CleanUpCameraAsync();
            }
            await InitializeScreenAsync();
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
            IntelligenceService.Current.ScoreUpdated += UpdatePredictionRadialGaugeValue;

            //need dummy current emoji placeholder to start
            if (!_isPaused) CurrentEmojis._currentEmoji = null;

            //maintain results emojis if app is suspended and resumed
            if (!_isPaused) _resultsEmojis = new List<Emoji>();

            //to avoid initial interval delay, make first UI update here
            await UpdateNonPredictionBasedUIAsync();

            _isInitialized = true;
        }

        public void UpdatePredictionRadialGaugeValue(object sender, EmotionPageGaugeScoreEventArgs e)
        {
            nextPercentage = (double)e.Score;
        }

        private void InitializeTimer()
        {
            countdownNum = Constants.EMOJI_DISPLAY_DURATION_IN_SECONDS * Constants.DIVISIBLE_FACTOR;
            if (EmojiTimer == null) EmojiTimer = new DispatcherTimer();
            EmojiTimer.Interval = TimeSpan.FromMilliseconds(Constants.EMOJI_COUNTDOWN_INTERVAL);
            EmojiTimer.Tick += DispatcherTimer_Tick;
        }

        private async void DispatcherTimer_Tick(object sender, object e)
        {
            await UpdateNonPredictionBasedUIAsync();
            UpdatePredictionRadialGaugeAesthetics();
        }

        private async Task UpdateNonPredictionBasedUIAsync()
        {
            if (countdownNum == 0)
            {
                await UpdateGameplayScreenAsync();
            }
            else
            {
                UpdateCountdownRing();
            }

            if (countdownNum % Constants.DIVISIBLE_FACTOR == 0)
            {
                //display pattern 5...4...3...2...1...5... [...] ...2...1...0
                if ((numEmojisLeftToPerform > -1 && countdownNum != 0) || 
                    (numEmojisLeftToPerform == 0 && countdownNum == 0) )
                {
                    UpdateCountdownNumber();   
                }
            }
        }

        private void UpdatePredictionRadialGaugeAesthetics()
        {
            //smooth transition between new prediction values
            var dif = RadialGauge.Value - nextPercentage;
            var delta = Math.Abs(dif) / 8;
            delta = Math.Max(delta, smoothDelta);
            if (dif > 0)
            {
                RadialGauge.Value -= Math.Min(delta, Math.Abs(dif));
            }
            else if (dif < 0)
            {
                RadialGauge.Value += Math.Min(delta, Math.Abs(dif));
            }

            //update color of radial gauge depending on prediction percentage
            if (RadialGauge.Value <= BAD_COLOR_THRESHOLD)
            {
                RadialGauge.TrailBrush = new SolidColorBrush(Colors.Red);
            }
            else if (RadialGauge.Value <= OKAY_COLOR_THRESHOLD)
            {
                RadialGauge.TrailBrush = new SolidColorBrush(Colors.Yellow);
            }
            else
            {
                RadialGauge.TrailBrush = new SolidColorBrush(Colors.Green);
            }
        }

        private void UpdateCountdownRing()
        {
            --countdownNum;
            CountdownProgressBarControl.Value = countdownNum;
            Debug.WriteLine("countdown: " + countdownNum);
        }

        private void UpdateCountdownNumber()
        {
            CountdownDisplay.Text = (countdownNum / Constants.DIVISIBLE_FACTOR).ToString();

            //position countdown number before it is assigned a physical height & width
            float centerX = CountdownDisplay.ActualWidth == 0 ? 14 : (float)CountdownDisplay.ActualWidth / 2f;
            float centerY = CountdownDisplay.ActualHeight == 0 ? 34 : (float)CountdownDisplay.ActualHeight / 2f;

            CountdownDisplay.Fade(value: 1f).Scale(1, 1, centerX, centerY).SetDuration(0).Then()
                            .Fade(0.2f).Scale(0.5f, 0.5f, centerX, centerY).SetDuration(2000).Start();
        }

        private Emoji GetRandomEmoji()
        {
            Random r = new Random();
            int index = -1;

            //do not allow repeated emojis to be chosen for this game session 
            while (index == -1 || CurrentEmojis._emojis.Emojis[index].AlreadyUsed)
            {
                index = r.Next(CurrentEmojis._emojis.Emojis.Count);
            }
            CurrentEmojis._emojis.Emojis[index].AlreadyUsed = true;
            CurrentEmojis._currentEmojiIndex = index;
            return CurrentEmojis._emojis.Emojis[index];
        }

        private async Task UpdateGameplayScreenAsync()
        {
            await EmojiName.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, async () =>
            {
                if (numEmojisLeftToPerform == 0)
                {
                    await GetReadyToTransitionPageAsync();
                }
                else
                {

                    if (LoadingGameScreen.Visibility == Visibility.Visible)
                    {
                        //wait 5 seconds before displaying emojis so user has a chance to read game instructions
                        while (DateTime.Now.Second - _pageNavigatedToTime.Second < 5)
                        {
                            int secondsElapsed = DateTime.Now.Second - _pageNavigatedToTime.Second;
                            Debug.WriteLine("Duration of EmotionPage message (in seconds):" + secondsElapsed);
                            await Task.Delay(1000);
                        }

                        LoadingGameScreen.Visibility = Visibility.Collapsed;
                        GameplayScreen.Visibility = Visibility.Visible;

                        InitializeTimer();
                        EmojiTimer.Start();
                    }

                    await RenderEmojiAsync();

                }
                _resultsEmojis.Add(CurrentEmojis._currentEmoji);
                numEmojisLeftToPerform--;
            });

            //reset countdown clock for new emoji
            if (numEmojisLeftToPerform != 0) countdownNum = Constants.EMOJI_DISPLAY_DURATION_IN_SECONDS * Constants.DIVISIBLE_FACTOR;
        }

        private async Task RenderEmojiAsync()
        {
            //position emojis before they get assigned a physical height & width
            float centerX = (float)EmojiIcon.ActualWidth == 0 ? 32 : (float)EmojiIcon.ActualWidth / 2f;
            float centerY = (float)EmojiIcon.ActualHeight == 0 ? 30 : (float)EmojiIcon.ActualHeight / 2f;

            //load and transition emojis
            await EmojiIcon.Fade(0.2f).Scale(0.0f, 0.0f, centerX, centerY).SetDuration(100).StartAsync();
            CurrentEmojis._currentEmoji = GetRandomEmoji();
            EmojiName.Text = CurrentEmojis._currentEmoji.Name;
            EmojiIcon.Style = App.Current.Resources[CurrentEmojis._currentEmoji.StyleName.ToString()] as Style;
            await EmojiIcon.Fade(value: 1f).Scale(1, 1, centerX, centerY).SetDuration(300).StartAsync();
        }

        private async Task GetReadyToTransitionPageAsync()
        {
            EmojiName.Text = GameText.LOADER.GetString("DoneText");
            if (EmojiTimer != null) EmojiTimer.Stop();

            Application.Current.Suspending -= Current_Suspending;
            Application.Current.Resuming -= Current_Resuming;

            if (!debugMode)
            {
                CleanUpTimer();
                await CleanUpCameraAsync();
                Frame.Navigate(typeof(ResultsPage), _resultsEmojis);
            }
        }

        private async Task CleanUpCameraAsync()
        {
            _isInitialized = false;
            await CameraService.Current.CleanUpAsync();
            IntelligenceService.Current.ScoreUpdated -= UpdatePredictionRadialGaugeValue;
            IntelligenceService.Current.CleanUp();
        }

        private void CleanUpTimer()
        {
            if (EmojiTimer != null)
            {
                EmojiTimer.Tick -= DispatcherTimer_Tick;
                EmojiTimer = null;
            }
        }
    }
}
