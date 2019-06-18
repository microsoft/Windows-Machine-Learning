#pragma once
#include <string>
#include <Windows.h>
namespace FileHelper
{
    std::wstring GetModulePath();
    std::wstring GetAbsolutePath(std::wstring relativePath);
}