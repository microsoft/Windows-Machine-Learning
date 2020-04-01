#include "pch.h"
#include "FileHelper.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

using namespace winrt;
using namespace winrt::Windows::Media;
using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Storage;


namespace FileHelper {
  std::wstring GetModulePath() {
    std::wstring val;
    wchar_t modulePath[MAX_PATH] = { 0 };
    GetModuleFileNameW((HINSTANCE)&__ImageBase, modulePath, _countof(modulePath));
    wchar_t drive[_MAX_DRIVE];
    wchar_t dir[_MAX_DIR];
    wchar_t filename[_MAX_FNAME];
    wchar_t ext[_MAX_EXT];
    _wsplitpath_s(modulePath, drive, _MAX_DRIVE, dir, _MAX_DIR, filename,
      _MAX_FNAME, ext, _MAX_EXT);

    val = drive;
    val += dir;

    return val;
  }

  VideoFrame LoadImageFile(hstring filePath) {
    VideoFrame inputImage = nullptr;
    try {
      // open the file
      StorageFile file = StorageFile::GetFileFromPathAsync(filePath).get();
      // get a stream on it
      auto stream = file.OpenAsync(FileAccessMode::Read).get();
      // Create the decoder from the stream
      BitmapDecoder decoder = BitmapDecoder::CreateAsync(stream).get();
      // get the bitmap
      SoftwareBitmap softwareBitmap = decoder.GetSoftwareBitmapAsync().get();
      // load a videoframe from it
      inputImage = VideoFrame::CreateWithSoftwareBitmap(softwareBitmap);
    }
    catch (...) {
      printf("failed to load the image file, make sure you are using fully "
        "qualified paths\r\n");
      exit(EXIT_FAILURE);
    }
    // all done
    return inputImage;
  }

  std::vector<std::string> LoadLabels(std::string labelsFilePath) {
    // Parse labels from labels file.  We know the file's entries are already
    // sorted in order.
    std::vector<std::string> labels;
    std::ifstream labelFile{ labelsFilePath, std::ifstream::in };
    if (labelFile.fail()) {
      printf("failed to load the %s file.  Make sure it exists in the same "
        "folder as the app\r\n",
        labelsFilePath.c_str());
      exit(EXIT_FAILURE);
    }

    std::string s;
    while (std::getline(labelFile, s, ',')) {
      int labelValue = atoi(s.c_str());
      if (labelValue >= labels.size()) {
        labels.resize(labelValue + 1);
      }
      std::getline(labelFile, s);
      labels[labelValue] = s;
    }

    return labels;
  }
}