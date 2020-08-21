# SqueezeNet Rust sample
This is a desktop application that uses SqueezeNet, a pre-trained machine learning model, to detect the predominant object in an image selected by the user from a file.

Note: SqueezeNet was trained to work with image sizes of 224x224, so you must provide an image of size 224X224.

## Prerequisites
- [Install Rustup](https://www.rust-lang.org/tools/install)
- Install cargo-winrt through command prompt. Until Rust 1.46 is released, cargo-winrt should be installed through the winrt-rs git repository.
  - ```cargo install --git https://github.com/microsoft/winrt-rs cargo-winrt```

## Build and Run the sample
1. This project requires Rust 1.46, which is currently in Beta. Rust release dates can be found [here](https://forge.rust-lang.org/). Rust Beta features can be enabled by running the following commands through command prompt in this current project directory after installation of Rustup :
    - ``` rustup install beta ```
    - ``` rustup override set beta ```
2. Install the WinRT nuget dependencies with this command: ``` cargo winrt install ```
3. Build the project by running ```cargo build``` for debug and ```cargo build --release``` for release.
4. Run the sample by running this command through the command prompt. ``` cargo winrt run ```
    - Another option would be to run the executable directly. Should be ```<git enlistment>\Samples\RustSqueezeNet\target\debug\rust_squeezenet.exe```

## Sample output
```
C:\Repos\Windows-Machine-Learning\Samples\RustSqueezeNet> cargo winrt run
    Finished installing WinRT dependencies in 0.47s
    Finished dev [unoptimized + debuginfo] target(s) in 0.12s
     Running `target\debug\rust_squeezenet.exe`
Loading model C:\Repos\Windows-Machine-Learning\RustSqueezeNet\target\debug\Squeezenet.onnx
Creating session
Loading image file C:\Repos\Windows-Machine-Learning\RustSqueezeNet\target\debug\kitten_224.png
Evaluating
Results:
  tabby tabby cat 0.9314611
  Egyptian cat 0.06530659
  tiger cat 0.0029267797
```