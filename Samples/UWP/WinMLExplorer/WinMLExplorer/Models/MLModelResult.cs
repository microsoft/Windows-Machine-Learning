using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using Windows.UI;

namespace WinMLExplorer.Models
{
    [DataContract]
    public class MLModelResult
    {
        [DataMember(Name = "correlationId")]
        public string CorrelationId { get; set; }

        [DataMember(Name = "durationInMilliSeconds")]
        public double DurationInMilliSeconds { get; set; }

        [DataMember(Name = "outputFeatures")]
        public MLModelOutputFeature[] OutputFeatures { get; set; }
    }

    [DataContract]
    public class MLModelOutputFeature
    {
        [DataMember(Name = "label")]
        public string Label { get; set; }

        [DataMember(Name = "probability")]
        public float Probability { get; set; }
    }

    [DataContract]
    public class DisplayResultSetting
    {
        [DataMember(Name = "Name")]
        public string Name { get; set; }

        [DataMember(Name = "color")]
        public Color Color { get; set; }

        [DataMember(Name = "probabilityRange")]
        public Tuple<float, float> ProbabilityRange { get; set; }
    }
}
