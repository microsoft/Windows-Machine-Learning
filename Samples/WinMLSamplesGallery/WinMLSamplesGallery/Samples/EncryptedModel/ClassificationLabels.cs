using System;
using System.Collections.Generic;
using Windows.Storage;

namespace WinMLSamplesGallery.Samples
{
    public static class ClassificationLabels
    {
        private static Dictionary<long, string> _imagenetLabels = null;
        public static Dictionary<long, string> ImageNet {
            get {
                if (_imagenetLabels == null)
                {
                    _imagenetLabels = LoadLabels("ms-appx:///InputData/sysnet.txt");
                }
                return _imagenetLabels;
            }
        }

        private static Dictionary<long, string> _ilsvrc2013Labels = null;
        public static Dictionary<long, string> ILSVRC2013
        {
            get
            {
                if (_ilsvrc2013Labels == null)
                {
                    _ilsvrc2013Labels = LoadLabels("ms-appx:///InputData/ilsvrc2013.txt");
                }
                return _ilsvrc2013Labels;
            }
        }

        private static Dictionary<long, string> LoadLabels(string csvFile)
        {
            var file = StorageFile.GetFileFromApplicationUriAsync(new Uri(csvFile)).GetAwaiter().GetResult();
            var text = Windows.Storage.FileIO.ReadTextAsync(file).GetAwaiter().GetResult();
            var labels = new Dictionary<long, string>();
            var records = text.Split(Environment.NewLine);
            foreach (var record in records)
            {
                var fields = record.Split(",", 2);
                if (fields.Length == 2)
                {
                    var index = long.Parse(fields[0]);
                    labels[index] = fields[1];
                }
            }
            return labels;
        }
    }
}
