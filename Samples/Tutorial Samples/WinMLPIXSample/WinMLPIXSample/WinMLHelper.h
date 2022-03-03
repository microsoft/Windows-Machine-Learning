#pragma once
#include "pch.h"

#ifndef WINMLHELPER_H
#define WINMLHELPER_H

using namespace winrt;
using namespace winrt::Windows::AI::MachineLearning;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Media;
using namespace winrt::Windows::Storage;
using namespace std;

// Global variables
static hstring modelPath = L".\\Assets\\SqueezeNet.onnx";
static string deviceName = "default";
static hstring imagePath = L"\\Assets\\kitten_224.png";
static LearningModel model = nullptr;
static LearningModelSession session = nullptr;
static LearningModelBinding binding = nullptr;
static VideoFrame imageFrame = nullptr;
static string labelsFilePath = ".\\Assets\\Labels.txt";
static vector<string> labels;

// Forward declarations
void LoadModel();
VideoFrame LoadImageFile(hstring filePath);
void CreateSession(ID3D12CommandQueue* commandQueue);
void BindModel(ID3D12CommandQueue* commandQueue);
void EvaluateModel();
void PrintResults(IVectorView<float> results);
void LoadLabels();

#endif