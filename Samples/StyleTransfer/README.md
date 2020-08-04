# Real-Time Style Transfer Sample

This UWP application uses the [Microsoft.AI.MachineLearning](https://www.nuget.org/packages/Microsoft.AI.MachineLearning/) Nuget pacakge to perform style transfer on user-provided input images or web camera streams. 
The VideoEffect/StyleTranfserEffectCpp project implements the (IBasicVideoEffect)[https://docs.microsoft.com/en-us/uwp/api/windows.media.effects.ibasicvideoeffect?view=winrt-19041] interface in order to create a video effect that can be plugged in to the media streaming pipeline.

For how-tos, tutorials and additional information, see the [Windows ML documentation](https://docs.microsoft.com/windows/ai/).
To learn more about creating custom video effects, see [this walkthrough](https://docs.microsoft.com/en-us/windows/uwp/audio-video-camera/custom-video-effects). 

## Building the Sample
1. If you download the samples ZIP, be sure to unzip the entire archive, not just the folder with
   the sample you want to build.
2. Start Microsoft Visual Studio 2017 and select **File** \> **Open** \> **Project/Solution**.
### Build the Video Effect
3. Starting in the folder where you unzipped the samples, go to the Samples subfolder, then the
   subfolder for this specific sample (eg. StyleTranfer). Navigate to the VideoEffect/StyleTranferEffectCpp and open the .sln file. 
4. Confirm that you are set for the right configuration and platform (eg. Debug, x64).
5. Build the solution (Ctrl+Shift+B).
6. Take note of where the .winmd file is built to (eg. StyleTransferEffectCpp/x64/Debug/StyleTransferEffectCpp.winmd).
### Build the Sample App
7. Open a new Visual Studio window and select **File** \> **Open** \> **Project/Solution**.
8. Starting in the folder where you unzipped the samples, go to the Samples suboflder, then the subfolder for this specific example (eg. StyleTransfer).
    Open the Visual Studio (.sln) file.
9. In the Solution Explorer, right-click on the References tab and select "Add Reference..." 
10. Click Browse and navigate to the .winmd file you built in the previous section (eg. StyleTransferEffectCpp/x64/Debug/StyleTransferEffectCpp.winmd). Click OK. 
11. You should now be able to run the sample app!

## Requirements

- [Visual Studio 2017 - 15.4 or higher](https://developer.microsoft.com/en-us/windows/downloads)
- [Windows 10 - Build 17763 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewiso)
- [Windows SDK - Build 17763 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)
- [Microsoft.AI.MachineLearning Nuget package- version 1.4.0 or higher](https://www.nuget.org/packages/Microsoft.AI.MachineLearning/)

## Contributing

We're always looking for your help to fix bugs and improve the samples. Create a pull request, and we'll be happy to take a look.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## License

MIT. See [LICENSE file](https://github.com/Microsoft/Windows-Machine-Learning/blob/master/LICENSE).

The machine learning models in this sample are used with permission from Justin Johnson.
For additional information on these models, refer to:
- [Fast-neural-style GitHub repo](https://github.com/jcjohnson/fast-neural-style)
- [Perceptual Losses for Real-Time Style Transfer and Super-Resolution](https://cs.stanford.edu/people/jcjohns/papers/eccv16/JohnsonECCV16.pdf)
