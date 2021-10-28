Param
(
    # Build architecture.
    [ValidateSet(
        'x64',
        'x86',
        'ARM64')]
    [string]$Architecture = 'x64',

    # CMake generator.
    [ValidateSet(
        'Visual Studio 15 2017', 
        'Visual Studio 16 2019')]
    [string]$Generator='Visual Studio 16 2019',

    # Location to generate build files.
    [string]$BuildDirectory = "$PSScriptRoot\..\..\build\external\opencv\cmake_config\$Architecture",

    # Cleans build files before proceeding.
    [switch]$Clean
)


$Args = New-Object Collections.Generic.List[String]

if ($Architecture -eq 'x86') {
  $Args.Add("-A Win32")
}
else {
  $Args.Add("-A " + $Architecture)
}

$Args.Add("-G " + $Generator)
$Args.Add("-DCMAKE_SYSTEM_NAME=Windows")
$Args.Add("-DCMAKE_SYSTEM_VERSION=10.0")
$Args.Add("-DWITH_OPENCL=OFF")
$Args.Add("-DWITH_FFMPEG=OFF")
$Args.Add("-DWITH_CUDA=OFF")
$Args.Add("-DBUILD_EXAMPLES=OFF")
$Args.Add("-DBUILD_TESTS=OFF")
$Args.Add("-DBUILD_opencv_world=ON")
$Args.Add("-DCMAKE_SYSTEM_PROCESSOR=" + $Architecture)

if ($Clean) {
    $Args.Add("--clean")	
}

$Args.Add("-B " + $BuildDirectory)
cmake $Args "$PSScriptRoot\..\opencv"
