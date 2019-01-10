// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT license. 

using System;
using System.Collections.Generic;
using Windows.UI.Xaml.Media.Imaging;

namespace Emoji8
{
    public class Emoji
    {
        public string Name { get; set; }
        //Icon used for static emoji rendering (Main & Results page)
        public string Icon { get; set; }
        //EmojiStyle used for dynamic emoji rendering (Emotion page)
        public EmojiStyle StyleName { get; set; }
        public bool AlreadyUsed { get; set; }
        public SoftwareBitmapSource BestPic { get; set; }
        public WriteableBitmap BestPicWB { get; set; }
        public double BestScore { get; set; }
        public bool ShowOopsIcon { get; set; }

        public Emoji(string name, string icon, EmojiStyle style)
        {
            this.Name = name;
            this.Icon = icon;
            this.StyleName = style;
            this.AlreadyUsed = false;
            this.BestPic = null;
            this.BestPicWB = null;
            this.BestScore = 0;
            this.ShowOopsIcon = true;
        }
    }

    public class EmojiCollection
    {
        public List<Emoji> Emojis { get; set; }

        public EmojiCollection()
        {
            Emojis = new List<Emoji>();
            int numEmojisAdded = 0;

            foreach (EmojiStyle style in Enum.GetValues(typeof(EmojiStyle)))
            {
                if (numEmojisAdded < Constants.POTENTIAL_EMOJI_NAME_LIST.Count)
                {
                    Emojis.Add(new Emoji(Constants.POTENTIAL_EMOJI_NAME_LIST[numEmojisAdded], Constants.POTENTIAL_EMOJI_ICON_LIST[numEmojisAdded], style));
                }
                ++numEmojisAdded;

                //TODO: make sure the Constant lists match so illegally indexing exceptions don't get thrown
            }
        }
    }

    public class CurrentEmojis
    {
        public static EmojiCollection _emojis { get; set; }
        public static Emoji _currentEmoji { get; set; }
        public static int _currentEmojiIndex { get; set; }
    }

    //render dynamic emojis for EmotionsPage, points to glyph values in App.xaml
    public enum EmojiStyle
    {
        NeutralFace,
        HappyFace,
        SurprisedFace,
        SadFace,
        AngryFace,
        DisgustedFace,
        FearfulFace,
        ContemptFace,
        OopsFace
    }
}
