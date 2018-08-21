# Introduction

The concept of neural networks is decades old. However, only in recent years we have used them extensively.

One of the reasons that neural networks did not become popular earlier was processing power limitation. The limited resources meant only shallow architectures were possible, which could only solve simple problems. Deeper networks with many more parameters are able to discern much more complex problems.

## How good?

One example of the power of neural networks in computer vision when compared to classical vision pipelines is the ImageNet challenge. ImageNet is a large dataset of labeled images. The ImageNet challenge is a competition among researchers to build the best algorithms for image classification, object detection and other image and video related tasks.

The following image shows labelled images from the dataset (image followed by its label) and some sample predictions given by classifiers underneath. The first row shows correct top-1 classification, while the second row shows incorrect classifications.

![imagenet-fig4l.png](https://lh6.googleusercontent.com/_r67cUOFV5xbWryMY3-MjBEAWv5jz6cizxC68YGMTVQjaonfNKyMUqqZ_58e-Srtsy3DbkS3D2dWGIRXukMSHIlz4y033izj4TLMCUmtxlv9vIZSkWtfevEonQImLyPQo1fDA7PtJFk)

Around 2011, the error rate in image classification was around 25%. In 2012, the first submission using a convolutional neural network (AlexNet) had an error rate of 16%. In the next years, teams got down to error rates of a few percent.

![ImageNet error rate](https://cdn-images-1.medium.com/max/800/1*kOb39xf47de-Bqr9KcK9hw.png)

*[Image source](https://medium.com/global-silicon-valley/machine-learning-yesterday-today-tomorrow-3d3023c7b519)*

## Why neural?

Neural networks have a structure similar to the way we believe the brain works.

![Nature of Code Image](https://lh5.googleusercontent.com/4-5NWlioP9GWN9p2Wj7tLUARJzNgkZVVQclVvMrYGDeOzQN6Gp1YM8Bf4_j3aZZPoq2XLlwFJBVHCdlq8UM2XlhvAX8XODDGwC0cjPKVc5mXrwvlErVYJm9xn-OZOrDOvKlieb86F4U)

A single neuron can receive multiple impulses through its dendrites. These impulses come from many other neurons. Then, it sums these electrical impulses (through a weighted sum) and generates an output through its axon. The axon then connects to the next neurons.

## How it works?

A neural network can be used in supervised or unsupervised learning. We will focus on supervised learning applications.

Supervised learning means a machine learns from labeled data. A large set of data and corresponding labels is given to **train** the network. Then it can be run on new, unseen samples and should hopefully generalize to these new samples.

A simple neural network structure (**Fully Connected**) looks like this:

![FC Network](https://lh4.googleusercontent.com/pDrX_bIyuUdmoCXYrp3v0Q7K24tkdJCEo_FG3KNUdNcnSFwtpuGZuEvA4RCEx2091u82QiUgFTJRwPWpsfQ_97qozQRbnDIupvRaIfJV2Zpc1BMESfIdrMfmPWGNGnCBvisRWc_udz8)

*Image source: PyImageSearch*

First, inputs are fed to the network. These inputs can be, for example, RGB pixel values. The network has internal (hidden) layers doing transformations on the data (effectively applying a function), and generates outputs. Each node is connected to all the nodes in the previous layer.

These transformations are weighted sums of the inputs. A weighted sum is a linear function, where each node output is a linear combination of its inputs. However, a linear function of linear functions can be simplified to a single linear function, meaning that additional layers don't make the network any more powerful. We are missing one ingredient to make the hidden layers work: **activation functions**.

An activation function is applied to the node output and adds nonlinearities to the model. **Sigmoidal** activation functions are often used, such as the logistic or hyperbolic tangent functions:

![Logistic](https://lh6.googleusercontent.com/pkZ-7mJXKMXjyJT7R_t7PSEnsK9xsS1RjNShfvazEABj_B-s4Rnx0ScZXnAKIRRIo4I5rcjWQpwZEKhd6G81jpknf5a4bzplB-oMtZdhqTBLJ7qGhRl1iF7rdpSZjAkF6AZjL9TjiTI)

*Logistic function*

![tanh](https://lh6.googleusercontent.com/NAQUv4WCHCp8z6xa-hHKuAnj4ewT9n7UVG9qxvd6XbKIuJubItXV5r9zksniYuILGeBacqQYpWY_8LWlKZi6PcSGqSHYPp68rmTQnen2yHae1Q36C9jIy8HaY1jYsvG0lyYHc-xQ2HI)

 *Hyperbolic tangent*

Sigmoidal functions have a very steep change around zero and a very small change away from it. In practice, nodes with this activation act similarly to linear classifiers: if the weighted sum is above a given value, its output is very close to a maximum value (high); else, if under a certain value, it's very close to a minimum (low).

Every node also has a bias weight, which is added to the weighted sum. It shifts the sigmoid function; without a bias, all the weighted sums would cross the origin (0, 0, ..., 0).

---

Some simple functions can be approximated by a single artificial neuron. For example, logic functions `and` and `or`:

![img](https://lh4.googleusercontent.com/85gxnmqtMpfuBS1Ogisa-RNoMtXU3yVH-9DZ05KjU4fm7rTydVehdp-QsYu3G3c9MXhNY-_Ypemx5dnqw4TsdWNioRRChPNrLKiAFnnY6A81zSAnpzHv0yE08PlIJ6OyGbRKuivCaZk)

In the `and` function, the `-n` bias will keep the output low unless all the inputs are high. In the `or` function, the output will be high if any input is high.

More complicated functions can't be approximated by a single linear classifier, but can be by a series of layers. For example, the `xor` function:

![maxresdefault.jpg](https://lh5.googleusercontent.com/vNLK0S9EQmYR-CEYIbKadpsbWyyh4R20WR2Na467NcpC9mVVj4bCUV2UM92E78qSN9_nctyW6FTH-ANccEH6KGZSGAgn_tjo-4A9bfLURtVw_VnSLwSLqXQqn0EAofXyQTnqNDvlVXI)

A graphical representation of `xor` is shown on the left: its output is high if either `x1` or `x2` is high, and low otherwise. A single line can't separate the low and high outputs. However, a two layer network can approximate the `xor` function. `h1`  is an `or` function, triggering whenever `x1` or `x2` is high. `h2` is a `nand` function, triggering unless `x1` and `x2` are high. `y` is an `and` function. The final output will be high if any input is high, but not if both inputs are high, mimicking the `xor` function. In general, adding more layers to a network allows it to solve less linear and more complex problems.

## Training

Once you have a network architecture, you will want to "teach" it how to solve your problem. This process is called **training**.

The training process involves running the network with the labelled data, comparing its outputs to the ground truth labels and scoring it, and adjusting the weights according to the score.

* The process of running inference in the network is called **forward propagation**. Data is fed to the inputs, and a network generates an output.
* To score how good the prediction is, a **loss function** is used. The function tells how bad the prediction is.
* The process of adjusting weights is called **backwards propagation** of errors. The weights are adjusted depending on the gradient of the error, which is calculated based on the loss (if the loss is large, a large adjustment is applied) and the derivative of the activation function (the function must have a first order derivative).

The training process is an iterative optimization problem: at each iteration, the network can get a bit better, and lots of iterations and training time are required to mimic the desired function.

Many details have been left out of this explanation; for a throughout explanation with all the math, you can search for more articles online, [such as this one](https://medium.com/@erikhallstrm/backpropagation-from-the-beginning-77356edf427d).

## Overfitting

Overfitting means your network fits the training data very well, but doesn't generalize to new samples. It means the network learned features specific to the data it was trained on, memorizing the answers for that data.

Larger networks, with more parameters, are more prone to overfitting. There are a few ways to detect and avoid overfitting.

Before training, the dataset is usually split in at least two sets: the training and the validation set. The training set is used in training, while the validation set is never given to the network to backpropagate on. Every now and then, the network is run on the validation set and the average loss is calculated.

Plotting the loss curves and tracking how the loss evolves in the training and validation sets is a simple way to detect overfitting. If the validation loss starts increasing while the training loss decreases, the network is overfitting to the training dataset:

![Loss graph](Loss_graph.png)

## Inference

*[Windows ML overview](https://docs.microsoft.com/en-us/windows/uwp/machine-learning/overview)*

Inference is much cheaper than training. A trained model can be imported on a variety of devices, including less powerful computers such as smartphones. Windows ML is an inference library, running models locally on Windows systems. It abstracts the underlying hardware, making it easy to run models on GPU and CPU.

Windows ML uses [ONNX (Open Neural Network Exchange format)](https://onnx.ai/) formatted models.

When compared with other libraries, such as TensorFlow, Windows ML has less operators. However, Windows ML focuses on deployment scenarios in a variety of platforms, while TensorFlow focuses on flexibility, research and faster development.

## The broader scenario

This document only discusses a subset of applications of neural networks in image processing. There are many more topics not being covered; just to give an idea, [A mostly complete chart of neural networks](https://cdn-images-1.medium.com/max/2000/1*cuTSPlTq0a_327iTPJyD-Q.png) shows lots of architectures not being mentioned here.