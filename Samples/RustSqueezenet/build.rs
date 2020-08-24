macro_rules! copy_file {
    ($file:expr, $destination:expr) => {
        match fs::copy($file,
        $destination) {
            Ok(file) => file,
            Err(error) => panic!("Problem copying the file {} to {}: {:?}", $file, $destination, error),
        };
    }
}

fn copy_resources() {
    use std::fs;
    let profile = std::env::var("PROFILE").unwrap();
    if profile == "debug" {
        copy_file!("..\\..\\SharedContent\\media\\fish.png",".\\target\\debug\\fish.png");
        copy_file!("..\\..\\SharedContent\\media\\fish.png",".\\target\\debug\\kitten_224.png");
        copy_file!("..\\..\\SharedContent\\models\\SqueezeNet.onnx",".\\target\\debug\\SqueezeNet.onnx");
        copy_file!("..\\SqueezeNetObjectDetection\\Desktop\\cpp\\Labels.txt",".\\target\\debug\\Labels.txt");
    }
    else if profile == "release" {
        copy_file!("..\\..\\SharedContent\\media\\fish.png",".\\target\\release\\fish.png");
        copy_file!("..\\..\\SharedContent\\media\\fish.png",".\\target\\release\\kitten_224.png");
        copy_file!("..\\..\\SharedContent\\models\\SqueezeNet.onnx",".\\target\\release\\SqueezeNet.onnx");
        copy_file!("..\\SqueezeNetObjectDetection\\Desktop\\cpp\\Labels.txt",".\\target\\release\\Labels.txt");
    }
}

fn main() {
    winrt::build!(
        types
            microsoft::ai::machine_learning::*
            windows::graphics::imaging::*
    );
    copy_resources();
}
