using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Media;

using WinMLExplorer.Models;

namespace WinMLExplorer.ViewModels
{
    public class ResultViewModel : PropertyModel
    {
        /// <summary>
        /// Display result settings
        /// </summary>
        private DisplayResultSetting [] DisplayResultSettings;

        /// <summary>
        /// Max width of the view model
        /// </summary>
        private int MaxWidth = 500;

        public ResultViewModel(DisplayResultSetting[] settings)
        {
            // Define settings
            this.DisplayResultSettings = settings;
        }

        /// <summary>
        /// Color of the result based on display result setting
        /// </summary>
        public Brush Color
        {
            get
            {
                DisplayResultSetting setting = null;

                if (string.IsNullOrEmpty(Label) == false)
                {
                    setting =
                        this.DisplayResultSettings.FirstOrDefault(s => this.Label == s.Name && this.Probability >= s.ProbabilityRange.Item1 && this.Probability <= s.ProbabilityRange.Item2);
                }

                if (setting == null)
                {
                    setting =
                        this.DisplayResultSettings.FirstOrDefault(s => this.Probability >= s.ProbabilityRange.Item1 && this.Probability <= s.ProbabilityRange.Item2);
                }

                if (setting == null)
                {
                    return new SolidColorBrush(Colors.DarkBlue);
                }

                return new SolidColorBrush(setting.Color);
            }
        }

        private bool isVisible = false;

        /// <summary>
        /// Indicate if the view model is visible
        /// </summary>
        public bool IsVisible
        {
            get
            {
                return this.isVisible;
            }
            set
            {
                this.isVisible = value;
                this.RaisePropertyChanged("IsVisible");
                this.RaisePropertyChanged("VisibilityState");
            }
        }

        private string label = string.Empty;

        /// <summary>
        /// Label of the view model
        /// </summary>
        public string Label
        {
            get
            {
                return this.label;
            }
            set
            {
                if (this.label == value)
                {
                    return;
                }

                this.label = value;
                this.RaisePropertyChanged("Label");
            }
        }

        private float probability = 0f;

        /// <summary>
        /// Probability value of the view model
        /// </summary>
        public float Probability
        {
            get
            {
                return this.probability;
            }
            set
            {
                if (this.probability == value)
                {
                    return;
                }

                this.probability = value;

                this.RaisePropertyChanged("Probability");
                this.RaisePropertyChanged("Color");
                this.RaisePropertyChanged("ProbabilityValue");
                this.RaisePropertyChanged("Width");
            }
        }

        /// <summary>
        /// String value of the probability
        /// </summary>
        public string ProbabilityValue => string.Format("{0:P1}", this.Probability);

        /// <summary>
        /// Visibility state of the view model
        /// </summary>
        public Visibility VisibilityState => this.IsVisible == true ? Visibility.Visible : Visibility.Collapsed;

        /// <summary>
        /// Calculated width of the view model based on probability
        /// </summary>
        public int Width => (int)(this.MaxWidth * this.Probability);
                
    }
}
