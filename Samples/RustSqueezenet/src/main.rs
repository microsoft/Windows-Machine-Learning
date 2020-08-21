include!(concat!(env!("OUT_DIR"), "/winrt.rs"));
fn main() -> winrt::Result<()> {
    use microsoft::ai::machine_learning::*;
    use winrt::ComInterface;

    let model_path = get_current_dir()? + "\\Squeezenet.onnx";
    println!("Loading model {}", model_path);
    let learning_model = LearningModel::load_from_file_path(model_path)?;

    let device = LearningModelDevice::create(LearningModelDeviceKind::Cpu)?;

    println!("Creating session");
    let session = LearningModelSession::create_from_model_on_device(learning_model, device)?;

    let image_file_path = get_current_dir()? + "\\fish.png";
    println!("Loading image file {}", image_file_path);
    let input_image_videoframe = load_image_file(image_file_path)?;
    let input_image_feature_value = ImageFeatureValue::create_from_video_frame(input_image_videoframe)?;
    let binding = LearningModelBinding::create_from_session(&session)?;
    binding.bind("data_0", input_image_feature_value)?;

    println!("Evaluating");
    let results = LearningModelSession::evaluate(&session,binding, "RunId")?;

    let result_lookup = results.outputs()?.lookup("softmaxout_1")?;
    let result_itensor_float : ITensorFloat = result_lookup.try_query()?;
    let result_vector_view = result_itensor_float.get_as_vector_view()?;
    print_results(result_vector_view)?;
    Ok(())
}

fn print_results(results: windows::foundation::collections::IVectorView<f32>) -> winrt::Result<()> {
    let labels = load_labels()?;
    let mut sorted_results : std::vec::Vec<(f32,u32)> = Vec::new();
    for i in 0..results.size()? {
        let result = (results.get_at(i)?, i);
        sorted_results.push(result);
    }
    sorted_results.sort_by(|a, b| b.0.partial_cmp(&a.0).unwrap());
    
    // Display the result
    for i in 0..3 {
        println!("{} {}", labels[sorted_results[i].1 as usize], sorted_results[i].0)
    }
    Ok(())
}

fn get_current_dir() -> winrt::Result<String> {
    use std::io::Error;
    use std::env;
    let current_exe = match env::current_exe() {
        Ok(val) => val,
        Err(_err) => return Err(winrt::Error::new(winrt::ErrorCode(Error::last_os_error().raw_os_error().unwrap() as u32), "Failed to get current working directory of executable.")),
    };
    let current_dir = current_exe.parent().unwrap();
    Ok(current_dir.display().to_string())
}

fn load_labels() -> winrt::Result<std::vec::Vec<String>> {
    use std::fs::File;
    use std::io::{prelude::*, BufReader};
    use std::io::Error;

    let mut labels : std::vec::Vec<String> = Vec::new();
    let labels_file_path = get_current_dir()? + "\\Labels.txt";
    let file = match File::open(labels_file_path) {
        Ok(val) => val,
        Err(_err) => return Err(winrt::Error::new(winrt::ErrorCode(Error::last_os_error().raw_os_error().unwrap() as u32), "Failed to load labels.")),
    };
    let reader = BufReader::new(file);
    for line in reader.lines() {
        let line_str = match line {
                Ok(val)=> val,
                Err(_err) => return Err(winrt::Error::new(winrt::ErrorCode(Error::last_os_error().raw_os_error().unwrap() as u32), "Failed to read lines.")),
        };
        let mut tokenized_line: Vec<&str> = line_str.split(',').collect();
        let index = tokenized_line[0].parse::<usize>().unwrap();
        labels.resize(index+1, "".to_string());
        tokenized_line.remove(0);
        labels[index] = tokenized_line.join("");
    }
    Ok(labels)
}

fn load_image_file(image_file_path: String) -> winrt::Result<windows::media::VideoFrame> {
    use windows::graphics::imaging::*;
    use windows::media::*;
    use windows::storage::*;

    let file = StorageFile::get_file_from_path_async(image_file_path)?.get()?;
    let stream = file.open_async(FileAccessMode::Read)?.get()?;
    let decoder = BitmapDecoder::create_async(&stream)?.get()?;
    let software_bitmap = decoder.get_software_bitmap_async()?.get()?;
    let image_videoframe = VideoFrame::create_with_software_bitmap(software_bitmap)?;
    Ok(image_videoframe)
}