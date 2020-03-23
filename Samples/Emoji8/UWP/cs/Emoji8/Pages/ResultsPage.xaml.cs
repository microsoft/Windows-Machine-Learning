// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT license. 

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using System.Timers;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;
using Microsoft.Toolkit.Uwp.UI.Animations;
using Emoji8.Services;
using Emoji8.Pages.PageHelpers;
using Microsoft.Toolkit.Services.Twitter;
using System.IO;
using System.Diagnostics;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace Emoji8
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class ResultsPage : Page
    {
        private Timer messageTimer;
        public ObservableCollection<Emoji> ResultsEmojis { get; private set; }
        public static Stream GifStream;
        private TwitterUser user;

        public ResultsPage()
        {
            this.DataContext = this;
            this.InitializeComponent();
            inkCanvas.InkPresenter.InputDeviceTypes = Windows.UI.Core.CoreInputDeviceTypes.Mouse | 
                Windows.UI.Core.CoreInputDeviceTypes.Pen | Windows.UI.Core.CoreInputDeviceTypes.Touch;
        }

        protected override async void OnNavigatedTo(NavigationEventArgs e)
        {
            ResultsEmojis = new ObservableCollection<Emoji>(e.Parameter as List<Emoji>);
            await LoadGIFsAsync();
            ShowTimedMessage();
        }

        private void ShowTimedMessage()
        {
            messageTimer = new Timer(Constants.MESSAGE_BEFORE_RESULTS_DURATION_IN_MILLISECONDS);
            messageTimer.Elapsed += (s_, e_) => ShowResultsAsync();
            messageTimer.Start();
        }

        private async Task LoadGIFsAsync()
        {
            await Windows.ApplicationModel.Core.CoreApplication.MainView.CoreWindow.Dispatcher.RunAsync(CoreDispatcherPriority.Normal,
            async () =>
                {
                    //create gif that is displayed on ResultsPage
                    await CreateGifAsync(false);

                    //create branded gif that is displayed in Tweet content
                    await CreateGifAsync(true);
                }
            );
        }

        private async void ShowResultsAsync()
        {
            await Windows.ApplicationModel.Core.CoreApplication.MainView.CoreWindow.Dispatcher.RunAsync(CoreDispatcherPriority.Normal,
            async () =>
                {
                    //wait for animated gifs to generate before displaying complete results page
                    SetBackgroundToBestScorePic();
                    while (Gif.Source == null && brandedGif.Source == null && OopsEmojiGifReplacement.Visibility == Visibility.Collapsed)
                    {
                        await Task.Delay(20);
                    }
                    LoadingResultsScreen.Visibility = Visibility.Collapsed;
                    ResultsPageBackground.Visibility = Visibility.Visible;
                    ResultsLoadedScreen.Visibility = Visibility.Visible;
                    messageTimer.Stop();
                }
            );
        }

        private void SetBackgroundToBestScorePic()
        {
            Emoji em = null;
            for (int i = 0; i < ResultsEmojis.Count; i++)
            {
                if (i == 0 || ResultsEmojis[i].BestScore > em.BestScore)
                {
                    em = ResultsEmojis[i];
                }
            }

            if (em != null)
            BestPicBackground.Source = em.BestPic;
        }

        private async void PlayAgain(object sender, RoutedEventArgs e)
        {
            //hide results screen before it gets wiped
            await ResultsLoadedScreen.Fade(value: 0.0f, duration: 250, delay: 0).StartAsync();

            //dispose of Emoji objects
            foreach (Emoji emoji in ResultsEmojis)
            {
                emoji.AlreadyUsed = false;
                if (emoji.BestPic != null)
                {
                    emoji.BestPic.Dispose();
                    emoji.BestPic = null;
                }
                emoji.BestPicWB = null;
                emoji.BestScore = 0;
                emoji.ShowOopsIcon = true;
            }

            //clean up gifs
            Gif.Source = null;
            brandedGif.Source = null;

            Debug.WriteLine(GameText.LOADER.GetString("LoggerPlayAgainClicked"));
            Frame.Navigate(typeof(EmotionPage));
        }

        private async void ShareButtonClicked(object sender, RoutedEventArgs e)
        {
            try
            {
                await EstablishTwitterUserAsync();
                Debug.WriteLine(GameText.LOADER.GetString("LoggerShareToTwitterClicked"));
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"{GameText.LOADER.GetString("LoggerShareToTwitterError")} {ex.Message}");
                await MessageDialogService.Current.WriteMessage(GameText.LOADER.GetString("TwitterServiceUnavailable"));
                await ReturnScreenToNormalAfterPopupAsync(PostTweetBorder);
                return;
            }
        }

        private void ContinueAsLoggedInUserClickedYes(object sender, RoutedEventArgs e)
        {
            ClosePopup(ContinueQuestionPopup);

            //confirmed user can now edit Tweet content
            CreateTweet();
            Debug.WriteLine(GameText.LOADER.GetString("LoggerContinuePostingYesClicked"));
        }

        private async void ContinueAsLoggedInUserClickedNo(object sender, RoutedEventArgs e)
        {
            ClosePopup(ContinueQuestionPopup);

            //log out user and prompt then to log in again
            TwitterService.Instance.Logout();
            await EstablishTwitterUserAsync();
            Debug.WriteLine(GameText.LOADER.GetString("LoggerContinuePostingNoClicked"));
        }

        private async void PostTweetClickedCancel(object sender, RoutedEventArgs e)
        {
            await ReturnScreenToNormalAfterPopupAsync(PostTweetBorder);
            Debug.WriteLine(GameText.LOADER.GetString("LoggerPostTweetCancelClicked"));
        }

        private async void PostTweetClickedShare(object sender, RoutedEventArgs e)
        {
            try
            {
                //post tweet with gif and text
                if (GifStream != null && user != null && TwitterService.Instance != null)
                    await TwitterService.Instance.TweetStatusAsync(TweetText.Text, GifStream);
            }
            catch (Exception)
            {
                await MessageDialogService.Current.WriteMessage(GameText.LOADER.GetString("TwitterSharingFailed"));
                await ReturnScreenToNormalAfterPopupAsync(PostTweetBorder);
                return;
            }
            
            await ReturnScreenToNormalAfterPopupAsync(PostTweetBorder);
            Debug.WriteLine(GameText.LOADER.GetString("LoggerPostTweetClicked"));
        }

        private async Task ReturnScreenToNormalAfterPopupAsync(UIElement p)
        {
            ClosePopup(p);
            await FadeOutDarkBackgroundTintAsync();
            EnableResultsPageButtons();
        }

        private async Task EstablishTwitterUserAsync()
        {

            //ensure that Twitter Service credentials are not empty
            if (String.IsNullOrEmpty(Constants.TwitterConsumerKey) || String.IsNullOrEmpty(Constants.TwitterConsumerSecret)
                || String.IsNullOrEmpty(Constants.TwitterCallbackURI))
            {
                await MessageDialogService.Current.WriteMessage(GameText.LOADER.GetString("TwitterCredentialsEmpty"));
                return;
            }

            try
            {
                //initialize service
                TwitterService.Instance.Initialize(Constants.TwitterConsumerKey, Constants.TwitterConsumerSecret, Constants.TwitterCallbackURI);
            } catch (Exception ex)
            {
                await MessageDialogService.Current.WriteMessage(GameText.LOADER.GetString("TwitterInitializationErrorPart1") + ex.ToString() + GameText.LOADER.GetString("TwitterInitializationErrorPart2"));
                return;
            }

            //temporarily fade background to focus on popup(s)
            await FadeInDarkBackgroundTintAsync();

            user = await TwitterService.Instance.GetUserAsync();

            //if user is already logged in, ask if they want to post again
            if (user != null)
            {
                //get user profile image & username to display in popup
                InitializeContinueContent();               

                OpenPopup(ContinueQuestionPopup);
                DisableResultsPageButtons();
            }
            else //log in new user to post content
            {
                DisableResultsPageButtons();

                if (!await TwitterService.Instance.LoginAsync())
                {
                    await MessageDialogService.Current.WriteMessage(GameText.LOADER.GetString("TwitterLoginFailed"));
                    await FadeOutDarkBackgroundTintAsync();
                    EnableResultsPageButtons();
                    return;
                }

                EnableResultsPageButtons();
                user = await TwitterService.Instance.GetUserAsync();
                CreateTweet();
            }
        }

        private async Task FadeInDarkBackgroundTintAsync()
        {
            DarkPopupBackground.Visibility = Visibility.Visible;
            await DarkPopupBackground.Fade(value: 0.75f, duration: 750, delay: 0).StartAsync();
        }

        private async Task FadeOutDarkBackgroundTintAsync()
        {
            await DarkPopupBackground.Fade(value: 0.0f, duration: 750, delay: 0).StartAsync();
            DarkPopupBackground.Visibility = Visibility.Collapsed;
        }

        private void InitializeContinueContent()
        {
            var bitmapImage = new BitmapImage
            {
                UriSource = new Uri(user.ProfileImageUrl)
            };
            TwitterProfilePic.ImageSource = bitmapImage;
            ContinueText.Text = GameText.LOADER.GetString("TwitterContinuePostingPart1") + user.ScreenName + GameText.LOADER.GetString("TwitterContinuePostingPart2");
        }

        private void CreateTweet()
        {
            OpenPopup(PostTweetBorder);
        }

        private void ClosePopup(UIElement p)
        {
            if (p.Visibility == Visibility.Visible) { p.Visibility = Visibility.Collapsed; }
        }

        private void OpenPopup(UIElement p)
        {
            if (p.Visibility != Visibility.Visible) { p.Visibility = Visibility.Visible; }
        }

        private void EnableResultsPageButtons()
        {
            ShareButton.IsEnabled = true;
            PlayAgainButton.IsEnabled = true;
        }

        private void DisableResultsPageButtons()
        {
            ShareButton.IsEnabled = false;
            PlayAgainButton.IsEnabled = false;
        }

        private async Task CreateGifAsync(bool isBranded)
        {
            BitmapImage bitmap;
            var images = await GifHelpers.LoadImagesAsync(isBranded, ResultsEmojis);

            //create the gif
            if (images.Count > 0)
            {
                bitmap = await GifHelpers.CreateGifBitmapImageAsync(isBranded, images, ResultsEmojis);

                if (isBranded)
                {
                    brandedGif.Source = bitmap;
                } else
                {
                    Gif.Source = bitmap;
                }
            }
            else //no gif was created due to lack of recorded images
            {
                //hide what should have been included with the gif
                if (isBranded)
                {
                    brandedGif.Visibility = Visibility.Collapsed;
                }
                else
                {
                    Gif.Visibility = Visibility.Collapsed;
                    OopsEmojiGifReplacement.Visibility = Visibility.Visible;
                    BestMomentsText.Visibility = Visibility.Collapsed;
                    Emoji8Logo.Visibility = Visibility.Collapsed;
                    ShareButton.IsEnabled = false;
                }
            }
        }
    }
}
