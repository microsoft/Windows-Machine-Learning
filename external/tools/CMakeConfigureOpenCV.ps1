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
        'Visual Studio 16 2019',
        'Visual Studio 17 2022')]
    [string]$Generator='Visual Studio 17 2022',

    # Location to generate build files.
    [string]$BuildDirectory = "$PSScriptRoot\..\..\build\external\opencv\cmake_config\$Architecture",

    # Cleans build files before proceeding.
    [switch]$Clean
)


$Command = New-Object Collections.Generic.List[String]
$Command.Add("cmake")
if ($Architecture -eq 'x86') {
  $Command.Add("-A Win32")
}
else {
  $Command.Add("-A " + $Architecture)
}

$Command.Add("-G '$Generator'")
$Command.Add("-DCMAKE_SYSTEM_NAME=Windows")
$Command.Add("-DCMAKE_SYSTEM_VERSION=10.0")
$Command.Add("-DWITH_OPENCL=OFF")
$Command.Add("-DWITH_FFMPEG=OFF")
$Command.Add("-DWITH_CUDA=OFF")
$Command.Add("-DBUILD_EXAMPLES=OFF")
$Command.Add("-DBUILD_TESTS=OFF")
$Command.Add("-DBUILD_opencv_apps=OFF")
$Command.Add("-DBUILD_DOCS=OFF")
$Command.Add("-DBUILD_PERF_TESTS=OFF")
$Command.Add("-DBUILD_opencv_world=ON")

if ($Architecture -eq 'x64') {
  $Command.Add("-DCMAKE_SYSTEM_PROCESSOR=AMD64")
}
else {
  $Command.Add("-DCMAKE_SYSTEM_PROCESSOR=" + $Architecture)
}

if ($Clean) {
    $Command.Add("--clean")	
}

$Command.Add("-B '$BuildDirectory'")
$Command.Add("'$PSScriptRoot\..\opencv'")

$CommandStr = ($Command -join " ")
$CommandStr
Invoke-Expression $CommandStr
