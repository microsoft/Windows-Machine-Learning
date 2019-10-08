
# WinMLRunner tool

The WinMLRunner is a command-line based tool that can run .onnx or .pb models where the input and output variables are tensors or images. It will attempt to load, bind, and evaluate a model and output error messages if these steps were unsuccessful. It will also capture performance measurements on the GPU and/or CPU. If using the performance flag, the GPU, CPU and wall-clock times for loading, binding, and evaluating and the CPU and GPU memory usage during evaluation will print to the command line and to a CSV file.

## Getting the tool

You can either download the x64 executable or build it yourself.

### Download

[Download x64 and x86 Exe](https://github.com/Microsoft/Windows-Machine-Learning/releases)

### Build

#### Prerequisites
- [Visual Studio 2017 Version 15.7.4 or Newer](https://developer.microsoft.com/en-us/windows/downloads)
- [Windows 10 - Build 17763 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewiso)
- [Windows SDK - Build 18362 or higher](https://www.microsoft.com/en-us/software-download/windowsinsiderpreviewSDK)

The easiest way to use these samples without using Git is to download the zip file containing the current version (using the following link or by clicking the "Download ZIP" button on the repo page). You can then unzip the entire archive and use the samples in Visual Studio 2017. Notes: Before you unzip the archive, right-click it, select Properties, and then select Unblock.
Be sure to unzip the entire archive, and not just individual samples. The samples all depend on the SharedContent folder in the archive. In Visual Studio 2017, the platform target defaults to ARM, so be sure to change that to x64 or x86 if you want to test on a non-ARM device. Reminder: If you unzip individual samples, they will not build due to references to other portions of the ZIP file that were not unzipped. 
You must unzip the entire archive if you intend to build the samples.

## Run the tool
 ```
Required command-Line arguments:
-model <path>            : Fully qualified path to a .onnx or .pb model file.
      or
-folder <path>           : Fully qualifed path to a folder with .onnx and/or .pb models, will run all of the models in the folder.

#Optional command-line arguments:
-version: prints the version information for this build of WinMLRunner.exe
-CPU : run model on default CPU
-GPU : run model on default GPU
-GPUHighPerformance : run model on GPU with highest performance
-GPUMinPower : run model on GPU with the least power
-GPUAdapterName <adapter name substring>: run model on GPU specified by its name. NOTE: Please only use this flag on DXCore supported machines. You will need to retarget the solution to at least windows insider build 18936
-CreateDeviceOnClient : create the D3D device on the client and pass it to WinML to create session
-CreateDeviceInWinML : create the device inside WinML
-CPUBoundInput : bind the input to the CPU
-GPUBoundInput : bind the input to the GPU
-RGB : load the input as an RGB image
-BGR : load the input as a BGR image
-Tensor [function] : load the input as a tensor, with optional function for input preprocessing
    Optional function arguments:
        Identity(default) : No input transformations will be performed.
        Normalize <scale> <means> <stddevs> : float scale factor and comma separated per channel means and stddev for normalization.
-Perf [all]: capture performance measurements such as timing and memory usage. Specifying "all" will output all measurements
-Iterations : # times perf measurements will be run/averaged. (maximum: 1024 times)
-Input <path to input file>: binds image or CSV to model
-InputImageFolder <path to directory of images> : specify folder of images to bind to model" << std::endl;
-TopK <number>: print top <number> values in the result. Default to 1
-BaseOutputPath [<fully qualified path>] : base output directory path for results, default to cwd
-PerfOutput [<path>] : fully qualified or relative path including csv filename for perf results
-SavePerIterationPerf : save per iteration performance results to csv file
-PerIterationPath <directory_path> : Relative or fully qualified path for per iteration and save tensor output results.  If not specified a default(timestamped) folder will be created.
-SaveTensorData <saveMode>: saveMode: save first iteration or all iteration output tensor results to csv file [First, All]
-DebugEvaluate: Print evaluation debug output to debug console if debugger is present.
-Terse: Terse Mode (suppresses repetitive console output)
-AutoScale <interpolationMode>: Enable image autoscaling and set the interpolation mode [Nearest, Linear, Cubic, Fant]
-GarbageDataMaxValue <maxValue>: Limit generated garbage data to a maximum value.  Helpful if input data is used as an index.

Concurrency Options:
-ConcurrentLoad: load models concurrently
-NumThreads <number>: number of threads to load a model. By default this will be the number of model files to be executed
-ThreadInterval <milliseconds>: interval time between two thread creations in milliseconds

 ```

Note that -CPU, -GPU, -GPUHighPerformance, -GPUMinPower -BGR, -RGB, -tensor, -CPUBoundInput, -GPUBoundInput are not mutually exclusive (i.e. you can combine as many as you want to run the model with different configurations).

### Examples:
Run a model on the CPU and GPU separately 5 times and output performance data:
> WinMLRunner.exe -model c:\\data\\concat.onnx -iterations 5 -perf

Runs all the models in the data folder, captures performance data 3 times using only the CPU: 
> WinMLRunner.exe -folder c:\\data -perf -iterations 3 -CPU

Run a model on the CPU and GPU separately, and by binding the input to the CPU and the GPU separately (4 total runs):
> WinMLRunner.exe -model c:\\data\\SqueezeNet.onnx -CPU -GPU -CPUBoundInput -GPUBoundInput

Run a model on the CPU with the input bound to the GPU and loaded as an RGB image:
> WinMLRunner.exe -model c:\\data\\SqueezeNet.onnx -CPU -GPUBoundInput -RGB

## Default output

**Running a good model:**
Run the executable as shown below. Make sure to replace the install location with what matches yours:
 ```
.\WinMLRunner.exe -model SqueezeNet.onnx
WinML Runner
GPU: AMD Radeon Pro WX 3100

Loading model (path = SqueezeNet.onnx)...
=================================================================
Name: squeezenet_old
Author: onnx-caffe2
Version: 9223372036854775807
Domain:
Description:
Path: SqueezeNet.onnx
Support FP16: false

Input Feature Info:
Name: data_0
Feature Kind: Float

Output Feature Info:
Name: softmaxout_1
Feature Kind: Float

=================================================================

Binding (device = CPU, iteration = 1, inputBinding = CPU, inputDataType = Tensor)...[SUCCESS]
Evaluating (device = CPU, iteration = 1, inputBinding = CPU, inputDataType = Tensor)...[SUCCESS]
Outputting results..
Feature Name: softmaxout_1
 resultVector[818] has the maximal value of 1


Binding (device = GPU, iteration = 1, inputBinding = CPU, inputDataType = Tensor)...[SUCCESS]
Evaluating (device = GPU, iteration = 1, inputBinding = CPU, inputDataType = Tensor)...[SUCCESS]
Outputting results..
Feature Name: softmaxout_1
 resultVector[818] has the maximal value of 1
 ```
**Running a bad model:**
Here's an example of running a model with incorrect parameters:
 ```
=================================================================
Name: bad_model
Author: 
Version: 0
Domain: onnxml
Description:
Path: C:\bad_model.onnx
=================================================================

Loading model...[SUCCESS]
Binding Model on GPU...[SUCCESS]
Evaluating Model on GPU...[FAILED]
c:\winml\dll\mloperatorauthorimpl.cpp(1081)\Windows.AI.MachineLearning.dll!00007FFEBBF9E19D: (caller: 00007FFEBC308C37) Exception(4) tid(5190) 80070057 The parameter is incorrect.
    [winrt::Windows::AI::MachineLearning::implementation::AbiOpKernel::Compute::<lambda_0245d72c2a362d137084c22c5297e48a>::operator ()(m_operatorFactory->CreateKernel(kernelInfoWrapper.Get(), ret.GetAddressOf()))]
 ```

## Performance output
The following performance measurements will be output to the command-line and .csv file for each load, bind, and evaluate operation:

* **Wall-clock time**: the elapsed real time (in ms) between the beginning and end of an operation.
* **GPU time**: time (in ms) for an operation to be passed from CPU to GPU and execute on the GPU (note: Load() is not executed on the GPU).
* **CPU time**: time (in ms) for an operation to execute on the CPU. 
 ```
 // The timer class is used to capture high-resolution timing information.
 // TimerHelper.h Line 18
class Timer
{
public:
    void Start()
    {
        LARGE_INTEGER t;
        QueryPerformanceCounter(&t);
        m_startTime = static_cast<double>(t.QuadPart);
    }

    double Stop()
    {
        LARGE_INTEGER stopTime;
        QueryPerformanceCounter(&stopTime);
        double t = static_cast<double>(stopTime.QuadPart) - m_startTime;
        LARGE_INTEGER tps;
        QueryPerformanceFrequency(&tps);
        return t / static_cast<double>(tps.QuadPart) * 1000;
    }

private:
    double m_startTime;
};
 ```
 * **Memory usage (evaluate)**: Average kernel and user-level memory usage (in MB) during evaluate on the CPU or GPU.

 Run the executable as shown below. Make sure to replace the install location with what matches yours:
 ```
WinMLRunner.exe -model C:\Repos\Windows-Machine-Learning\SharedContent\models\SqueezeNet.onnx -perf

If capturing performance measurements, the program will create a CSV titled "WinML Model Run [Today's date]" in the same folder as the .exe file. 
 ```
Working Set Memory (MB) - The amount of DRAM memory that the process on the CPU required during evaluation.
Dedicated Memory (MB) - The amount of memory that was used on the VRAM of the dedicated GPU.
Shared Memory (MB) -  The amount of memory that was used on the DRAM by the GPU.
 ### Sample performance output:
 ```
.\WinMLRunner.exe -model SqueezeNet.onnx -perf
WinML Runner
Printing available GPUs with DXGI..
Index: 0, Description: AMD Radeon Pro WX 3100

Loading model (path = .\SqueezeNet.onnx)...
=================================================================
Name: squeezenet_old
Author: onnx-caffe2
Version: 9223372036854775807
Domain:
Description:
Path: .\SqueezeNet.onnx
Support FP16: false

Input Feature Info:
Name: data_0
Feature Kind: Float

Output Feature Info:
Name: softmaxout_1
Feature Kind: Float

=================================================================


Creating Session with CPU device
Binding (device = CPU, iteration = 1, inputBinding = CPU, inputDataType = Tensor, deviceCreationLocation = WinML)...[SUCCESS]
Evaluating (device = CPU, iteration = 1, inputBinding = CPU, inputDataType = Tensor, deviceCreationLocation = WinML)...[SUCCESS]


Results (device = CPU, numIterations = 1, inputBinding = CPU, inputDataType = Tensor, deviceCreationLocation = WinML):

First Iteration Performance (load, bind, session creation, and evaluate):
  Load: 436.598 ms
  Bind: 0.8575 ms
  Session Creation: 120.181 ms
  Evaluate: 177.233 ms

  Working Set Memory usage (evaluate): 9.95313 MB
  Working Set Memory usage (load, bind, session creation, and evaluate): 45.6289 MB
  Peak Working Set Memory Difference (load, bind, session creation, and evaluate): 46.5625 MB

  Dedicated Memory usage (evaluate): 0 MB
  Dedicated Memory usage (load, bind, session creation, and evaluate): 0 MB

  Shared Memory usage (evaluate): 0 MB
  Shared Memory usage (load, bind, session creation, and evaluate): 0 MB




Creating Session with GPU: AMD Radeon Pro WX 3100
Binding (device = GPU, iteration = 1, inputBinding = CPU, inputDataType = Tensor, deviceCreationLocation = WinML)...[SUCCESS]
Evaluating (device = GPU, iteration = 1, inputBinding = CPU, inputDataType = Tensor, deviceCreationLocation = WinML)...[SUCCESS]


Results (device = GPU, numIterations = 1, inputBinding = CPU, inputDataType = Tensor, deviceCreationLocation = WinML):

First Iteration Performance (load, bind, session creation, and evaluate):
  Load: 436.598 ms
  Bind: 5.1858 ms
  Session Creation: 285.041 ms
  Evaluate: 25.7202 ms

  Working Set Memory usage (evaluate): 1.21484 MB
  Working Set Memory usage (load, bind, session creation, and evaluate): 42.8047 MB
  Peak Working Set Memory Difference (load, bind, session creation, and evaluate): 44.1152 MB

  Dedicated Memory usage (evaluate): 10.082 MB
  Dedicated Memory usage (load, bind, session creation, and evaluate): 15.418 MB

  Shared Memory usage (evaluate): 1 MB
  Shared Memory usage (load, bind, session creation, and evaluate): 6.04688 MB
 ```
 
 ## Capturing Trace Logs
 If you want to capture trace logs using the tool, you can use logman commands in conjunction with the debug flag:
 ```
logman start winml -ets -o winmllog.etl -nb 128 640 -bs 128
logman update trace  winml -p {BCAD6AEE-C08D-4F66-828C-4C43461A033D} 0xffffffffffffffff 0xff -ets         
WinMLRunner.exe -model C:\Repos\Windows-Machine-Learning\SharedContent\models\SqueezeNet.onnx -debug
logman stop winml -ets
 ```
The winmllog.etl file will appear in the same directory as the WinMLRunner.exe. 
 
 ## Reading the Trace Logs
1. Using the traceprt.exe
  * Run the following command from the command-line and open the logdump.csv file:
 ```
tracerpt.exe winmllog.etl -o logdump.csv -of CSV
 ```

2. Windows Performance Analyzer (from Visual Studio)
 * Launch Windows Performance Analyzer and open the winmllog.etl.

## Dynamic DLL Loading

If you want to run WinMLRunner with another version of WinML (e.g. comparing the performance with an older version or testing a newer version), simply place the `windows.ai.machinelearning.dll` and `directml.dll` files in the same folder as WinMLRunner.exe. WinMLRunner will look for for these DLLs first and fall back to `C:/Windows/System32` if it doesn't find them.

## Known issues

- Sequence/Map inputs are not supported yet (the model is just skipped, so it doesn't block other models in a folder);
- We cannot reliably run multiple models with the `-folder` argument with real data. Since we can only specify 1 input, the size of the input would mismatch with most of the models. Right now, using the `-folder` argument only works well with garbage data;
- Generating garbage input as Gray or YUV is not currently supported. Ideally, WinMLRunner's garbage data pipeline should support all inputs types that we can give to winml.

## License
MIT. See [LICENSE file](https://github.com/Microsoft/Windows-Machine-Learning/blob/master/LICENSE).
