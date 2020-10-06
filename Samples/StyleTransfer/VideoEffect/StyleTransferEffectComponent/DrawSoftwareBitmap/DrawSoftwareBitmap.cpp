// DrawSoftwareBitmap.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "unknwn.h"
#include <iostream>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Foundation.h>
#include "Memorybuffer.h"
#include <winrt/Windows.Storage.h>
#include "CImg.h"
#include <locale>
#include <string>
#include "DrawSoftwareBitmap.h"
using namespace cimg_library;
using namespace std;

using namespace winrt;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Graphics::Imaging;

void SaveOutputToDisk(
    SoftwareBitmap bitmap,
    hstring outputDataImageFileName)
{
    // save the output to disk
    StorageFolder currentfolder = StorageFolder::GetFolderFromPathAsync(L"C:\\Users\\Ryan\\source\\repos\\DrawSoftwareBitmap\\x64\\Debug\\").get();
    StorageFile outimagefile = currentfolder.CreateFileAsync(outputDataImageFileName, CreationCollisionOption::ReplaceExisting).get();
    IRandomAccessStream writestream = outimagefile.OpenAsync(FileAccessMode::ReadWrite).get();
    BitmapEncoder encoder = BitmapEncoder::CreateAsync(BitmapEncoder::JpegEncoderId(), writestream).get();
    encoder.SetSoftwareBitmap(bitmap);
    encoder.FlushAsync().get();
}

void WriteSoftwareBitmap(SoftwareBitmap& bitmap, string& text)
{
    BitmapBuffer buffer = bitmap.LockBuffer(BitmapBufferAccessMode::Write);
    winrt::Windows::Foundation::IMemoryBufferReference reference = buffer.CreateReference();
    auto spByteAccess = reference.as<::Windows::Foundation::IMemoryBufferByteAccess>();
    BYTE* dataInBytes;
    UINT32 capacity;
    spByteAccess->GetBuffer(&dataInBytes, &capacity);
    // fill in bgra plane
    BitmapPlaneDescription bufferLayout = buffer.GetPlaneDescription(0);

    // Create 224 x 224 RGB image
    CImg<unsigned char> image(224, 224, 1, 4);

    // Fill with magenta
    cimg_forXY(image, x, y) {
        image(x, y, 0, 0) = 0;
        image(x, y, 0, 1) = 255;
        image(x, y, 0, 2) = 255;
        image(x, y, 0, 3) = 255;
    }

    // Make some colours
    unsigned char cyan[] = { 0,   255, 255 };
    unsigned char black[] = { 0,   0,   0 };
    // Draw black text on cyan
    image.draw_text(3, 20, text.c_str(), black, cyan, 1, 12);
    BYTE arr[224][224][4];
    cimg_forXY(image, x, y) {
        arr[y][x][0] = image(x, y, 0, 0);
        arr[y][x][1] = image(x, y, 0, 1);
        arr[y][x][2] = image(x, y, 0, 2);
        arr[y][x][3] = image(x, y, 0, 3);
    }
    for (int i = 0; i < bufferLayout.Height; i++)
    {
        for (int j = 0; j < bufferLayout.Width; j++)
        {

            //BYTE value = (BYTE)((float)j / bufferLayout.Width * 255);
            dataInBytes[bufferLayout.StartIndex + bufferLayout.Stride * i + 4 * j + 0] = arr[i][j][0];
            dataInBytes[bufferLayout.StartIndex + bufferLayout.Stride * i + 4 * j + 1] = arr[i][j][1];
            dataInBytes[bufferLayout.StartIndex + bufferLayout.Stride * i + 4 * j + 2] = arr[i][j][2];
            dataInBytes[bufferLayout.StartIndex + bufferLayout.Stride * i + 4 * j + 3] = arr[i][j][3];
        }
    }
}

int main()
{

    std::cout << "Hello World!\n";
    SoftwareBitmap bitmap = SoftwareBitmap(BitmapPixelFormat::Rgba8, 224, 224);
    string str = "Tinca tinca with a confidence\n of 0.7345\nTinca tinca with a confidence\n of 0.7345\nTinca tinca with a confidence\n of 0.7345\n";
    WriteSoftwareBitmap(bitmap, str);
    SaveOutputToDisk(bitmap, L"test.jpg");
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
