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
        private DisplayResultSetting [] DisplayResultSettings;
        private int MaxWidth = 500;

        public ResultViewModel(DisplayResultSetting[] settings)
        {
            // Define settings
            this.DisplayResultSettings = settings;
        }

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

        public string ProbabilityValue => string.Format("{0:P1}", this.Probability);

        public Visibility VisibilityState => this.IsVisible == true ? Visibility.Visible : Visibility.Collapsed;

        public int Width => (int)(this.MaxWidth * this.Probability);
                
    }
}
