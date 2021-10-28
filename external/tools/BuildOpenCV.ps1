Param
(
    # Build architecture.
    [ValidateSet(
        'x64',
        'x86',
        'ARM64')]
    [string]$Architecture = 'x64',

    # Build configuration.
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',

    # Location to generate build files.
    [string]$CMakeBuildDirectory = "$PSScriptRoot\..\..\build\external\opencv\cmake_config\$Architecture\",

    # Cleans build files before proceeding.
    [switch]$Clean
)

if ($Clean) {
  Remove-Item -Recurse -Force $CMakeBuildDirectory
}

& $PSScriptRoot\CMakeConfigureOpenCV.ps1 -Architecture $Architecture

# Build OpenCV
$solution = $CMakeBuildDirectory + "OpenCV.sln"
msbuild /p:Configuration=$Configuration /t:Build /p:LinkIncremental=false /p:DebugSymbols=false /p:DebugType=None $solution

# Install
$installProject = $CMakeBuildDirectory + "INSTALL.vcxproj"
msbuild /p:Configuration=$Configuration /p:LinkIncremental=false /p:DebugSymbols=false /p:DebugType=None $installProject