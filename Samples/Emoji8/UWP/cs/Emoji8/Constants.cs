// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT license. 

using Microsoft.Services.Store.Engagement;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace Emoji8
{
    class Constants
    {
        public const string MODEL_PATH = "ms-appx:///Models//model.onnx";

        public const float CLASSIFICATION_CERTAINTY_THRESHOLD = 0.2f;
        public const int CLASSIFICATION_SENSITIVITY_IN_SECONDS = 0;

        public const int EMOJI_DISPLAY_DURATION_IN_SECONDS = 5;
        public const int DIVISIBLE_FACTOR = 25;
        public const int EMOJI_COUNTDOWN_INTERVAL = 1;
        public const int EMOJI_TICK_INTERVAL_IN_MILLISECONDS = 100;
        public const int EMOJI_NUMBER_TO_DISPLAY = 4;
        public const int MESSAGE_BEFORE_RESULTS_DURATION_IN_MILLISECONDS = 4000;

        public static readonly IList<String> POTENTIAL_EMOJI_NAME_LIST = new ReadOnlyCollection<string>
            (new List<string> {
                GameText.LOADER.GetString("EmotionNeutral"),
                GameText.LOADER.GetString("EmotionHappiness"),
                GameText.LOADER.GetString("EmotionSurprise"),
                GameText.LOADER.GetString("EmotionSadness"),
                GameText.LOADER.GetString("EmotionAnger"),
                GameText.LOADER.GetString("EmotionDisgust"),
                GameText.LOADER.GetString("EmotionFear"),
                GameText.LOADER.GetString("EmotionContempt") });

        public static readonly IList<String> POTENTIAL_EMOJI_ICON_LIST = new ReadOnlyCollection<string>
            (new List<string> { "😐", "😄", "😮", "😭", "😠", "🤢", "😨", "🙄" });

        public static readonly string TwitterConsumerKey = "";
        public static readonly string TwitterConsumerSecret = "";
        public static readonly string TwitterCallbackURI = "";

        public static StoreServicesCustomEventLogger LOGGER = StoreServicesCustomEventLogger.GetDefault();
    }
}
