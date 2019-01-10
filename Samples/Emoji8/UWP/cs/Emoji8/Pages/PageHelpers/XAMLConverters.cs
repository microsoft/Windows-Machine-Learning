// Copyright (c) Microsoft Corporation. 
// Licensed under the MIT license. 

using System;
using Windows.UI.Xaml.Data;

namespace Emoji8
{
    // Used to help display percent sign for prediction scores on EmotionPage and ResultsPage
    public sealed class StringFormatConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value == null)
                return null;
            if (parameter == null)
                return value;
            return string.Format((string)parameter, value);
        }

        public object ConvertBack(object value, Type targetType, object parameter,
            string language)
        {
            throw new NotImplementedException();
        }
    }

    // Used to enable and disable elements on ResultsPage when a placeholder emoji might need to fill in the spot of a missing videoframe
    public sealed class InvertBoolConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            bool booleanValue = (bool)value;
            return !booleanValue;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            bool booleanValue = (bool)value;
            return !booleanValue;
        }
    }
}
