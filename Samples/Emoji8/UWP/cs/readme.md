# Emoji8

![Emoji8 logo](Emoji8/Assets/emoji8Smaller.png)

## Overview

This sample shows you how you can use [Windows Machine Learning](https://aka.ms/winmldocsfromemoji8oss) to power a fun emotion-detecting application.

Emoji8 is sample UWP application [available in the Store](https://aka.ms/getemoji8fromoss) that evaluates your facial expressions while you imitate a random selection of emojis. The app takes in a video feed from your computer's webcam and evaluates the images with the [FER+ Emotion Recognition model (version 1.2)](https://aka.ms/emoji8emotionmodelfromoss) locally on your machine. You can tweet a gif summarizing your best scoring pics when you have an internet connection and continue playing even when you don't have one!

This app will give you a great end-to-end example of how you can use the Windows ML APIs to create simple yet magical experiences.



## Assumptions
   This sample assumes you have the following installed/established:

1. Windows 10 October 2018 Update [1809]
1. Visual Studio 2017 (version 15.7.4+) with Windows 10 SDK Build 17763
1. A Twitter developer account (if you want to enable the "Share to Twitter" feature)
1. A front-facing camera connected to your computer

## Steps to run the sample

1. Load the `Emoji8.sln` into Visual Studio.

1. _(Optional)_ Enable the "Share to Twitter" feature with the Windows Community Toolkit [Twitter Service](https://aka.ms/twitterserviceredirectfromemoji8oss).
    1. [Register](https://apps.twitter.com/) your local sample as a Twitter app.  
    1. Locate and copy your generated Twitter:
        * Consumer Key
        * Consumer Secret
        * Callback URL
    1. Paste the above credentials in the their corresponding empty Twitter strings in the `Constants.cs` file.
1. _(Optional)_ Remove the Dev Center logging. This non-personal data is only successfully transmitted when playing a Store-published version of the app, so running this OSS code locally will not actually collect any data.    
    1. Uninstall the `Microsoft.Services.Store.Engagement` NuGet package.
    1. Remove Microsoft Engagement Framework from the project References.
    1. Delete the following 2 lines in the `Constants.cs` file:

        ```cs
        using Microsoft.Services.Store.Engagement;
        ```

        ```cs
        public static StoreServicesCustomEventLogger LOGGER = StoreServicesCustomEventLogger.GetDefault();
        ```

    1. Delete the following line in the `MainPage.xaml.cs` file:

        ```cs
        Constants.LOGGER.Log(GameText.LOADER.GetString("LoggerStartButtonClicked"));
        ```

    1. Lastly, delete the following 7 lines in the `ResultsPage.xaml.cs` file:

        ```cs
        Constants.LOGGER.Log(GameText.LOADER.GetString("LoggerPlayAgainClicked"));
        ```

        ```cs
        Constants.LOGGER.Log(GameText.LOADER.GetString("LoggerShareToTwitterClicked"));
        ```

        ```cs
        Constants.LOGGER.Log($"{GameText.LOADER.GetString("LoggerShareToTwitterError")} {ex.Message}");
        ```

        ```cs
        Constants.LOGGER.Log(GameText.LOADER.GetString("LoggerContinuePostingYesClicked"));
        ```

        ```cs
        Constants.LOGGER.Log(GameText.LOADER.GetString("LoggerContinuePostingNoClicked"));
        ```

        ```cs
        Constants.LOGGER.Log(GameText.LOADER.GetString("LoggerPostTweetCancelClicked"));
        ```

        ```cs
        Constants.LOGGER.Log(GameText.LOADER.GetString("LoggerPostTweetClicked"));
        ```

1. Build and run the solution.
1. Enjoy! :)

## 3rd Party OSS Usage

* [WriteableBitmapEx](https://github.com/teichgraf/WriteableBitmapEx/)
   * Licensed under the MIT license.

## Other Legal Stuff

### Data Collection 

The software may collect information about you and your use of the software, and send it to Microsoft. Microsoft may use this information to provide services and improve our products and services. You may turn off the telemetry as described in Step 3 of "Steps to run the sample" above. There are also some features in the software that may enable you and Microsoft to collect data from users of your applications. If you use these features, you must comply with applicable law, including providing appropriate notices to users of your applications together with a copy of Microsoft’s privacy statement. Our privacy statement is located at https://go.microsoft.com/fwlink/?LinkID=824704. You can learn more about data collection and use in the help documentation and our privacy statement. Your use of the software operates as your consent to these practices. 
