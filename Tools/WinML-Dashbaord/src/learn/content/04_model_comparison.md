# Comparative study between image networks

In this document, modern networks are discussed and compared.

## Building blocks

Some building blocks of modern networks are discussed here, covering:

* Inception
* ResNet
* ResNeXt
* ShuffleNet
* MobileNet

### Inception

*[Short history of the Inception deep learning architecture](https://nicolovaligi.com/history-inception-deep-learning-architecture.html)*

*[Inception-v4, Inception-ResNet and the Impact of Residual Connections on Learning](https://arxiv.org/abs/1602.07261)*

In Inception, 1x1, 3x3 and 5x5 convolutions and 3x3 max pooling are applied to the input and concatenated.

To reduce the number of input channels in the expensive 3x3 and 5x5 convolutions, and to reduce the number of outputs after max pooling, 1x1 convolutions are applied, in a bottleneck style (*note: this graph goes from bottom to top*):

![Inception](https://raw.githubusercontent.com/iamaaditya/iamaaditya.github.io/master/images/inception_1x1.png)

> [Aggregated Residual Transformations for Deep Neural Networks](https://arxiv.org/pdf/1611.05431.pdf) observes that an important common property of Inception family models is a *split-transform-merge* strategy. In an Inception module, the input is split into a few lower — dimensional embeddings (by 1×1 convolutions), transformed by a set of specialized filters (3×3, 5×5, *etc*…), and merged by concatenation. It can be shown that the solution space of this architecture is a strict subspace of the solution space of a single large layer (*e.g*., 5×5) operating on a high-dimensional embedding. The split-transform-merge behavior of Inception modules is expected to approach the representational power of large and dense layers, but at a considerably lower computational complexity.

Newer versions of Inception include residual connections and higher cardinality blocks.

### ResNet

*[Deep Residual Learning for Image Recognition](https://arxiv.org/abs/1512.03385)*

ResNet uses residual connections. It also uses bottlenecks in the deeper architectures, replacing its two layer block with a three layer bottleneck:

![ResNet](https://i.stack.imgur.com/kbiIG.png)

### ResNeXt

*[Aggregated Residual Transformations for Deep Neural Networks](https://arxiv.org/pdf/1611.05431.pdf)*

 In ResNeXt, blocks have high cardinality (32 paths), use a residual connection, and use a bottleneck architecture (the 256 channels are first reduced to 4 channels using 1x1 convolutions). Following is a comparison of ResNet's and ResNeXt's building blocks:

![ResNeXt](https://cdn-images-1.medium.com/max/1600/1*mdiQTfovOXKnqzfj727b9Q.png)

The building block of ResNeXt is equivalent to a grouped convolution, with 32 groups (each group with 4 input and output channels):

![ResNeXt](https://cdn-images-1.medium.com/max/2000/1*ydf_35H_2ESd9dd3zCHT3A.png)

### ShuffleNet

*[ShuffleNet: An Extremely Efficient Convolutional Neural Network for Mobile Devices](https://arxiv.org/abs/1707.01083)*

ShuffleNet is focused on efficiency. It uses grouped convolutions. However, it notes that multiple group convolutions have a downside: outputs from a channel are only derived from a small fraction of input channels. The reduced flow between channel groups weakens the model.

In ShuffleNet, the channels are shuffled among groups after each grouped convolution:

![ShuffleNet](https://cdn-images-1.medium.com/max/1200/0*Bjh1Kxs9hQ76tDDW.png)

Its building block uses depthwise separable convolutions and grouped convolutions. For dimensionality reduction, a stride of 2 is used:

![ShuffleNet building block](ShuffleNet_block.png)

### MobileNet

*[MobileNets: Efficient Convolutional Neural Networks for Mobile Vision Applications](https://arxiv.org/abs/1704.04861)*

*[MobileNets: Open-Source Models for Efficient On-Device Vision](https://ai.googleblog.com/2017/06/mobilenets-open-source-models-for.html)*

MobileNet uses depthwise separable convolutions to reduce its runtime. Convolutions with stride 2 are used to reduce the resolution of  higher level feature maps. In the end, a fully connected layer and a softmax function are used to classify the output:

![MobileNet](MobileNet.png)

MobileNet also introduces two parameters to easily trade model accuracy for runtime performance, to mak is suitable for less powerful systems. `α` is a width multiplier for the network; if `α = 0.5`, for example, all the layer will use half their nominal width (half the number of filters). `ρ` is a resolution multiplier, multiplying the resolution of the input and all the internal layers. Changing these parameters require retraining the network. The paper has performance benchmarks of the model with varying parameters, showing mean average precision, operation count and parameter count.

### MobileNet v2

*[MobileNetV2: Inverted Residuals and Linear Bottlenecks](https://arxiv.org/pdf/1801.04381.pdf)*

*[MobileNetV2: The Next Generation of On-Device Computer Vision Networks](http://ai.googleblog.com/2018/04/mobilenetv2-next-generation-of-on.html)*

MobileNetV2 uses depthwise separable convolutions, like the original MobileNet. It adds bottlenecks and shortcut connections between bottlenecks:

![MobileNet v2](https://raw.githubusercontent.com/joshua19881228/my_blogs/master/Computer_Vision/Reading_Note/figures/Reading_Note_20180307_InvertedResiduals.png)

> The intuition is that the bottlenecks encode the model’s intermediate inputs and outputs while the inner layer encapsulates the model’s ability to transform from lower-level concepts such as pixels to higher level descriptors such as image categories. Finally, as with traditional residual connections, shortcuts enable faster training and better accuracy.
>
> [...]
>
> Overall, the MobileNetV2 models are faster for the same accuracy across the entire latency spectrum. In particular, the new models use 2x fewer operations, need 30% fewer parameters and are about 30-40% faster on a Google Pixel phone than MobileNetV1 models, all while achieving higher accuracy.

### SqueezeNet

*[SqueezeNet: AlexNet-level accuracy with 50x fewer parameters and <0.5MB model size](https://arxiv.org/abs/1602.07360)*

*[Graph of the full model](https://raw.githubusercontent.com/cmasch/squeezenet/master/images/model-1_1.png)*

SqueezeNet is based on a building block that squeezes its input (reduces the channel count using 1x1 convolutions), trying to compress the features, runs 1x1 and 3x3 convolutions, and concatenates the outputs for the next block. It follows the same concept of a bottleneck design. Maxpooling is added between some blocks to reduce the resolution of higher level features.

![SqueezeNet](https://cdn-images-1.medium.com/max/1600/1*xji5NAhX6m3Nk7BmR_9GFw.png)

## Models

Models are summarized and compared in the table below. Classification and detection are common tasks related to images and were chosen for this comparison.

### Classification

The aforementioned models can be used for classification. Their papers append either fully connected layers and a softmax for classification, or use global average pooling followed by softmax.

### Detection

The following object detection models are considered state-of-art:

* [SSD](https://arxiv.org/pdf/1512.02325.pdf) variations: SSD (Single Shot MultiBox Detector) is based on the VGG-16 architecture. Instead of fully connected layers, it has a detection layer followed by NMS (non-maximum suppression). It also has skip connections from higher resolution layers. It is very popular and many optimizations were later applied to it. Some variations are (sorted by release date):

  * [DSSD : Deconvolutional Single Shot Detector](https://arxiv.org/abs/1701.06659)
  * [Feature-Fused SSD: Fast Detection for Small Objects](https://arxiv.org/abs/1709.05054)
  * [FSSD: Feature Fusion Single Shot Multibox Detector](https://arxiv.org/abs/1712.00960)
  * [MDSSD: Multi-scale Deconvolutional Single Shot Detector for small objects](https://arxiv.org/abs/1805.07009)

* [YOLOv3: An Incremental Improvement](https://arxiv.org/abs/1804.02767): Based on the Darknet-53 architecture (53 convolutions with residual connections and strided convolutions for downsampling):

  ![Darknet](https://www.groundai.com/media/arxiv_projects/195592/x3.png)

  After the convolutions, instead of average pooling, fully connected and softmax, a YOLO (You Only Look Once) layer is used, which does object detection using anchor boxes.

### Semantic segmentation

[A 2017 Guide to Semantic Segmentation with Deep Learning](http://blog.qure.ai/notes/semantic-segmentation-deep-learning-review)  has short descriptions of a few models. [DeepLabv3](https://arxiv.org/abs/1706.05587) is considered current state of art.

[really-awesome-semantic-segmentation](https://github.com/nightrome/really-awesome-semantic-segmentation) has a list of papers, which [can be found here](https://docs.google.com/spreadsheets/d/1r1PNqpcNyo3E8enQdBz-zze7nMiOUd4lb890WPh7aII/). Performance evaluation of many state of art models can be found in [PASCAL VOC Challenge](http://host.robots.ox.ac.uk:8080/leaderboard/displaylb.php?challengeid=11&compid=6#KEY_DeepLabv3).

## Comparison

This table summarizes results from some top performing image classification networks. Data was taken from papers or extracted from models (using [Netscope](https://tiagoshibata.github.io/netscope/) or TensorFlow to extract model information). Some of the papers didn't include the top-5 error in ImageNet-1K, in which case it was taken from other internet sources.

[This fork of Netscope](https://github.com/tiagoshibata/netscope) supports separable convolutions and shuffle layers ([live version](https://tiagoshibata.github.io/netscope/)).

### Classification

| Model               | Runtime | % Top-5 error in ImageNet-1K                               | Parameter count | [Multiply-Accumulate](https://en.wikipedia.org/wiki/Multiply%E2%80%93accumulate_operation) (MAC) count | Comparison count | Operator count * | Windows ML support                   |
| ------------------- | ------- | ---------------------------------------------------------- | --------------- | ------------------------------------------------------------ | ---------------- | ---------------- | ------------------------------------ |
| Inception v4        | Large   | 3.08                                                       | 42.71M          | 12.27G                                                       | 21.87M           | 638              | Yes                                  |
| ResNet-152          | Large   | 5.7                                                        | 60.19M          | 11.3G                                                        | 22.33M           | 671              | Yes                                  |
| ResNet-50           | Medium  | 6.7                                                        | 25.56M          | 3.87G                                                        | 10.89M           | 229              | Yes                                  |
| ResNeXt-101 (32x4d) | Large   | 5.6                                                        | 44.18M          | 7.99G                                                        | 21.53M           | 452              | Yes                                  |
| ResNeXt-50          | Medium  | 6.6                                                        | 25.03M          | 4.25G                                                        | 14.7M            | 231              | Yes                                  |
| MobileNetV2         | Small   | < 9.5                                                      | 3.51M           | 438.04M                                                      | 8.2M             | 209              | Yes                                  |
| ShuffleNet x1       | Smaller | [< 15](https://github.com/farmingyard/ShuffleNet/issues/4) | 2.12M           | 170.34M                                                      | 3.25M            | 218              | No (missing group shuffle operation) |
| SqueezeNet v1.1     | Smaller | < 19.7                                                     | 1.24M           | 387.75M                                                      | 6.02M            | 67               | Yes                                  |

* &ast; "Operator count" counts layers, including internal operators (e.g. a Fire module counts as a 3 convolutions, 3 ReLU, and a concat operators). It was taken from [Netscope](https://tiagoshibata.github.io/netscope/).

### Detection

A current (April/2018) comparison of mean average precision and inference time in the COCO dataset, normalized to the same GPU, can be found in [YOLOv3: An Incremental Improvement](https://arxiv.org/abs/1804.02767). [https://github.com/lzx1413/PytorchSSD](https://github.com/lzx1413/PytorchSSD) has mAP and FPS benchmarks of some models in VOC2007 and COCO.

## Compressing networks

These references have information on how to analyze a model and reduce its size:

[Pruning Filters for Efficient ConvNets](https://arxiv.org/abs/1608.08710)
[Compressing deep neural nets](http://machinethink.net/blog/compressing-deep-neural-nets/) and [Compressing MobileNet on ImageNet](https://gist.github.com/hollance/d15c0cc6004a5479c00ac26bce61ac8d)
