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
    [switch]$Clean,

    [switch]$SetupDevEnv = $False
)

if ($Clean) {
  Remove-Item -Recurse -Force $CMakeBuildDirectory
}

& $PSScriptRoot\CMakeConfigureOpenCV.ps1 -Architecture $Architecture

$msbuild = "msbuild"

if ($SetupDevEnv)
{
  Install-Module VSSetup -Scope CurrentUser -Force
  $latestVS = Get-VSSetupInstance -All -Prerelease | Sort-Object -Property InstallationVersion -Descending | Select-Object -First 1
  if ($latestVS.InstallationVersion -like "15.*") {
    $msbuild = "$($latestVS.InstallationPath)\MSBuild\15.0\Bin\msbuild.exe"
  } else {
    $msbuild = "$($latestVS.InstallationPath)\MSBuild\Current\Bin\msbuild.exe"
  }
}

# Build OpenCV
$solution = $CMakeBuildDirectory + "OpenCV.sln"
& $msbuild /p:Configuration=$Configuration /t:Build /p:LinkIncremental=false /p:DebugSymbols=false /p:DebugType=None $solution

# Install
$installProject = $CMakeBuildDirectory + "INSTALL.vcxproj"
& $msbuild /p:Configuration=$Configuration /p:LinkIncremental=false /p:DebugSymbols=false /p:DebugType=None $installProject