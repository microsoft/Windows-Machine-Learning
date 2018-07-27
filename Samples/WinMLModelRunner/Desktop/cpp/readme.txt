The WinMLModelRunner program can run .onnx or .pb models where the input and output variables
are tensors. It allows you to run WinML on the GPU or CPU, and if neither are specified will 
run the test multiple times to generate separate GPU and CPU performance measurements. The GPU, 
CPU and wall-clock times for loading, binding, and evaluating and the CPU and GPU memory usage during
evaluate will print to the command line and to a CSV file. 

If no csv file name is specified, the program will create csv titled
"WinML Model Run [Today's date].csv" in the same folder as the .exe file. 

Command-Line Options:
---------------------------------------------------------------------------------------
Required command-Line arguments:
-model <path>     : Path to a .onnx model file.
		
-folder <path>    : Path to a folder with .onnx models, will run all of the models in the folder.

Optional command-line arguments:
-iterations <int> : Number of times to evaluate the model.
-CPU              : Will create a session on the CPU.
-GPU              : Will create a session on the GPU.
-csv <file name>   : Will create a CSV file and output the performance measurements to it. 

Examples:
---------------------------------------------------------------------------------------
Run 'concat' operator on the CPU and GPU separately 5 times:
> WinMLModelRunner.exe -model c:\\data\\concat.onnx -iterations 5 

Run all the models in the data folder 3 times using only the CPU: 
> WinMLModelRunner .exe -folder c:\\data -iterations 3 -CPU

Run all of the models in the data folder on the GPU and CPU once and output the 
performance data to benchmarkdata.csv:
> WinMLModelRunner.exe -folder c:\\data -csv benchmarkdata.csv
