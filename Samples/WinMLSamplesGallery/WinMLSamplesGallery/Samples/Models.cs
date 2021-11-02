using System;
using System.Collections.Generic;
using System.Text;

namespace WinMLSamplesGallery.Samples
{
    public enum OnnxModel
    {
        Unknown = 0,
        MobileNet,
        ResNet,
        SqueezeNet,
        VGG,
        AlexNet,
        GoogleNet,
        CaffeNet,
        RCNN_ILSVRC13,
        DenseNet121,
        Inception_V1,
        Inception_V2,
        ShuffleNet_V1,
        ShuffleNet_V2,
        ZFNet512,
        EfficientNetLite4,
        YoloV3,
        YoloV4,
        YoloV4Repo,
    }

    public sealed class OnnxModelViewModel
    {
        public string Title { get; set; }
        public OnnxModel Tag { get; set; }
    }
}
