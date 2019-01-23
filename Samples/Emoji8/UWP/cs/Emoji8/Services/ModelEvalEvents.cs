// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT license. 

using System;
using Windows.Graphics.Imaging;

namespace Emoji8.Services
{

    //used to update success gauge on EmotionPage
    public class EmotionPageGaugeScoreEventArgs : EventArgs
    {
        public double Score { get; set; }
    }

    //used to select imitated emoji on MainPage
    public class ClassifiedEmojiEventArgs : EventArgs
    {
        public ClassifiedEmojiEventArgs(Emoji emoji)
        {
            this.ClassifiedEmoji = emoji;
        }

        public Emoji ClassifiedEmoji { get; set; }
    }

    //used to pass image from camera to model evaluation
    public class SoftwareBitmapEventArgs : EventArgs
    {
        public SoftwareBitmapEventArgs(SoftwareBitmap softwareBitmap)
        {
            this.SoftwareBitmap = softwareBitmap;
        }

        public SoftwareBitmap SoftwareBitmap { get; set; }
    }
}
