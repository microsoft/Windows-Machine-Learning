# Modern operations

This guide lists modern operations in the field of computer vision using neural networks. These operations build on top of the basic convolution concepts and make smaller but powerful models possible. The guide is meant to be concise; it lists recommended articles and papers containing further information.

Pictures are taken from linked articles and papers. Text in quote blocks,

> such as this one,

is copied from the source papers.

# Operations and concepts

Here, recent operations and concepts are discussed. These are often reused in building blocks of modern neural networks.

## Convolutions

Some strategies can make convolutions faster, with lower parameter count, while not making it too less powerful.

### 1. 1x1 convolutions

1x1 convolutions are a convolution on a single pixel in the input images. The outcome is a function of the same pixel in all the image planes:

![1x1 Convolution](https://image.slidesharecdn.com/mcv-m52016lecture9-deepconvnetsforglobalrecognition-160330230446/95/deep-convnets-for-global-recognition-master-in-computer-vision-barcelona-2016-62-638.jpg?cb=1483804418)

1x1 convolutions are a kind of feature pooling across all images. It has learnable parameters (in the image above, c2 is the number of weights for each of the c3 filters). It can be used in:

- Dimensionality reduction: The number output of images is equal to the number of filters in the convolution, which can be lower than the number of input channels
- Separable convolutions
- Bottlenecks

### 2. Separable convolutions

*[Xception: Deep Learning with Depthwise Separable Convolutions](https://arxiv.org/abs/1610.02357)*

A convolutional layer adds a decent amount of parameters to the model. Each filter add `kernel height * kernel width * number of input image planes` weights.

In a standard convolution, a sliding window covers both the spatial dimension (sliding along X and Y) and the depthwise dimension (covering all input channels).

A separable convolution does two separate, but faster, convolutions. Instead of doing a pass through all channels, first filters will sweep only the spatial dimension, then the results will be pooled with a 1x1 convolution.

In more detail, a depthwise separable convolution:

- Receives `c` channels
- Each channel is independently convoluted with different `k`x`k` kernels. The number of kernels for each channel is called the depth multiplier or channel multiplier. 1 is the most common value.
- `c * depth_multiplier` channels go through 1x1 convolutions

Using numeric values, if we had 32 input channels, 64 output channels and 3x3 kernels:

- A standard convolution has `64 * 32 * 3 * 3 = 18432` parameters
- A depthwise separable convolution with depth multiplier 1 has `32 * 3 * 3 = 288` parameters for the spatial convolutions, followed by `64 * 32 * 1 * 1 = 2048` parameters for the 1x1 convolution (2336 parameters total)

### 3. Atrous/dilated convolution

*[Rethinking Atrous Convolution for Semantic Image Segmentation](https://arxiv.org/pdf/1706.05587.pdf)*

Small kernels have a very narrow view of the input channels. A dilated convolution introduces spacing between the values in the kernel when reading an input channel, thus enlarging the seen area of the input image:

![Dilated Convolution](https://cdn-images-1.medium.com/max/600/1*SVkgHoFoiMZkjy54zM_SUw.gif)

A dilated convolution has a larger receptive field, while maintaining the number of parameters and the computational cost. It can be used when a larger receptive field is desired, but the introduction of larger kernels or series of convolutions is too expensive. It's frequently found in segmentation networks.

### 4. Transposed convolution

*More visualizations at [Convolution arithmetic](https://github.com/vdumoulin/conv_arithmetic)*

Transposed convolutions are a convolution that upsamples the image. They are often used in decoders in encoder-decoder architectures.

The transposed convolution will undo the changes in dimension of a convolution:

![Convolution](https://cdn-images-1.medium.com/max/600/1*BMngs93_rm2_BpJFH2mS0Q.gif)

> 2D convolution with no padding, stride of 2 and kernel of 3

![Transposed convolution](https://cdn-images-1.medium.com/max/600/1*Lpn4nag_KRMfGkx1k6bV-g.gif)

> Transposed 2D convolution with no padding, stride of 2 and kernel of 3

The transposed convolution with no padding will actually pad the input to return to the original dimensions, and the stride is applied as spacing in the input.

*Transposed convolutions are sometimes called deconvolutions. However, it is **not** the inverse operation of a convolution: it applies the same operation, only reverting the dimension changes of the convolution.*

### 5. Grouped convolutions

*Recommended article: [A Tutorial on Filter Groups Grouped Convolution (Grouped Convolution)](https://blog.yani.io/filter-group-tutorial/)*

Grouped convolutions split the input channels in two distinct groups, with two distinct set of kernels:

![Grouped convolutions](https://blog.yani.io/assets/images/posts/2017-08-10-filter-group-tutorial/filtergroups2.svg)

Grouped convolutions save parameters (in the image above, if `g = 2` (two groups), there are `2 * h1 * w1 * c1/2 * c2/2` instead of `h1 * w1 * c1 * c2` weights) and are computationally cheaper. However, as noted in the article referenced above, it can also lead to better model performance: models can learn correlated filters in the same group, yielding better representations. For example, one group can specialize its `c2/2` filters in black and white filters (analyzing structures such as borders) while another specializes in colored filters.

## Building block structure

These changes are often found in modern building blocks of neural networks:

### 1. Residual networks

*Recommended article: [Understand Deep Residual Networks — a simple, modular learning framework that has redefined state-of-the-art](https://blog.waya.ai/deep-residual-learning-9610bb62c355)*

*[Deep Residual Learning for Image Recognition](https://arxiv.org/abs/1512.03385)*

As networks become deeper, accuracy gets saturated and then degrades. There is a *degradation* problem; adding more layers leads to higher training error, because more information from the previous layer is being distorted/lost.

Residual networks add identity connections between input and output of its building blocks:

![Blah](https://cdn-images-1.medium.com/max/800/1*pUyst_ciesOz_LUg0HocYg.png)

> Instead of hoping each stack of layers directly fits a desired underlying mapping, we explicitly let these layers fit a residual mapping. The original mapping is recast into F(x)+x. We hypothesize that it is easier to optimize the residual mapping than to optimize the original, unreferenced mapping. To the extreme, if an identity mapping were optimal, it would be easier to push the residual to zero than to fit an identity mapping by a stack of nonlinear layers.

It's worth noting that residual networks don't add learnable parameters to the model, maintaining the model parameter count.

In encoder-decoder models, symmetric **skip connections** pass details from higher resolution layers closer to the input, and thus less distorted. It is often used in models requiring per pixel precision, such as restoration and segmentation models. From [Image Restoration Using Convolutional Auto-encoders with Symmetric Skip Connections](https://arxiv.org/abs/1606.08921):

> First, they allow the signal to be back-propagated to bottom layers directly, and thus tackles the problem of gradient vanishing, making training deep networks easier and achieving restoration performance gains consequently. Second, these skip connections pass image details from convolutional layers to deconvolutional layers, which is beneficial in recovering the clean image.

![Encoder/decoder skip connections](https://pbs.twimg.com/media/DFKK-dOXYAAhEhS.jpg)

### 2. Bottlenecks

*[Deep Residual Learning for Image Recognition](https://arxiv.org/abs/1512.03385)*

A *bottleneck* design involves a layer with reduced number of neurons, preceded and followed by layers with more neurons, forming a bottleneck. The bottleneck has a reduced number of features, thus expensive operations (such as convolutions) have less input and output channels and are faster. The bottleneck is built with 1x1 convolutions to change the number of channels.

Benchmarks show few loss of generality when using bottlenecks. The input features are often correlated and the 1x1 convolutions can learn how to properly combine different channels. After the next block, features are again recombined and expanded using 1x1 convolutions.

### 3. Cardinality

*[Going Deeper with Convolutions](https://www.cs.unc.edu/~wliu/papers/GoogLeNet.pdf)*

*Cardinality* is a new dimension introduced in neural networks. It was first introduced in paper [Network in network](https://arxiv.org/abs/1312.4400), which proposes the use of a micro network in each convolutional layer instead of doing a simple convolution.

It has been shown that having simple functions operating on the same input followed by merging them can be as effective as a more complex function, but cheaper and with less parameters.

# Applications

Common uses of neural networks in image processing are in classification, detection and segmentation tasks.

### 10. Classification

Image classification networks receive an image and output which class it belongs to. Traditionally, it was built by appending fully connected layers and a softmax after the convolutional layers.

Newer models use global average pooling which are directly fed into a softmax. From *[Network in Network](https://arxiv.org/abs/1312.4400)*:

> However, the fully connected layers are prone to overfitting [...].
>
> In this paper, we propose another strategy called global average pooling to replace the traditional fully connected layers in CNN. The idea is to generate one feature map for each corresponding category of the classification task in the last mlpconv layer. Instead of adding fully connected layers on top of the feature maps, we take the average of each feature map, and the resulting vector is fed directly into the softmax layer. One advantage of global average pooling over the fully connected layers is that it is more native to the convolution structure by enforcing correspondences between feature maps and categories. Thus the feature maps can be easily interpreted as categories confidence maps. Another advantage is that there is no parameter to optimize in the global average pooling thus overfitting is avoided at this layer. Futhermore, global average pooling sums out the spatial information, thus it is more robust to spatial translations of the input.

### 11. Detection

*[You Only Look Once: Unified, Real-Time Object Detection](https://arxiv.org/abs/1506.02640)*

*[YOLO9000: Better, Faster, Stronger](https://arxiv.org/pdf/1612.08242.pdf)*

*[YOLOv3: An Incremental Improvement](https://pjreddie.com/media/files/papers/YOLOv3.pdf)*

*[SSD: Single Shot MultiBox Detector](https://arxiv.org/abs/1512.02325)*

Detection networks can detect objects in a scene (give them a class and location). In a very simplistic explanation, each of the outputs in the final convolutional layer is used as a classifier (giving confidence scores of each class) and a bounding box predictor (giving x/y location and width/height of the object).

In YOLO, for example, the last layer is an `SxS` grid, with `B * 5 + C` channels, where B is the max number of predicted boxes centered in that grid cell, and C is the number of classes.

To better fit the objects in the scene, **anchors** (also called **priors**) are used. A set of anchor boxes is defined before training, covering the most common object sizes. Instead of predicting the real width and height of objects, the networks is trained to predict an offset from the anchor boxes. The bounding boxes generated using anchors fit the objects better, because the range of values to be output by the network is smaller and centered around zero.

Detection networks will often have multiple cells outputting the same object. To remove duplicate detections, non-maximum suppression (NMS) is run on the results. If two bounding boxes have a high probability prediction of the same class and a high IoU (intersection over union), only the higher probability one is kept.

For good performance in multiple scales, detection neural networks usually have shortcut connections from earlier layers. For example, in SSD:

![SSD](https://cdn-images-1.medium.com/max/1110/1*pPxrkm4Urz04Ez65mwWE9Q.png)

YOLOv3 has a detection block with small anchor boxes early in the network, while in parallel more convolutions are done before a detection block with medium and then large anchor boxes ([taken from the model configuration](https://github.com/pjreddie/darknet/blob/481d9d98abc8ef1225feac45d04a9514935832bf/cfg/yolov3.cfg)):

```ini
[...]
# Downsample
[convolutional]
batch_normalize=1
filters=64
size=3
stride=2
pad=1
activation=leaky

[convolutional]
batch_normalize=1
filters=32
size=1
stride=1
pad=1
activation=leaky

[...]

[convolutional]
size=1
stride=1
pad=1
filters=255
activation=linear


[yolo]
mask = 6,7,8  # Uses anchor boxes 6, 7 and 8 (largest detections)
anchors = 10,13,  16,30,  33,23,  30,61,  62,45,  59,119,  116,90,  156,198,  373,326
classes=80
[...]

[convolutional]
size=1
stride=1
pad=1
filters=255
activation=linear


[yolo]
mask = 3,4,5  # Uses anchor boxes 3, 4 and 5 (medium detections)
anchors = 10,13,  16,30,  33,23,  30,61,  62,45,  59,119,  116,90,  156,198,  373,326
classes=80
[...]

[convolutional]
size=1
stride=1
pad=1
filters=255
activation=linear

[yolo]
mask = 0,1,2  # Uses anchor boxes 0, 1 and 2 (small detections)
anchors = 10,13,  16,30,  33,23,  30,61,  62,45,  59,119,  116,90,  156,198,  373,326
classes=80
```

### 12. Encoder-decoder networks

As seen in the Introduction to Convolutions document, convolutional layers can encode features, learning higher level features at each layer, generating more feature channels at a lower resolution.

Encoder-decoder architectures are used in image colorization, restoration, segmentation, contour detection and other fields where it's desired to input an image and output some processed form of the image.

In an encoder-decoder architecture, the first half (the encoder) has a series of convolutions detecting interesting features of the input image, outputting wider features (more channels) in lower resolution. Each set of convolutions will learn higher level features (contours, shapes, objects...).

In the second phase, a decoder block upsamples the image and reduces the number of channels using transposed convolutions. The transposed convolutions will learn how to merge the high level features to each pixel output.

![Encoder-decoder](https://ai2-s2-public.s3.amazonaws.com/figures/2017-08-08/1779b6a17ee68afafb6801477b165f19901689b2/3-Figure2-1.png)