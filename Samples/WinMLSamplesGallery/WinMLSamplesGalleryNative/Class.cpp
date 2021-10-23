#include "pch.h"
#include "Class.h"
#include "Class.g.cpp"

#include <Windows.h>

namespace winrt::WinMLSamplesGalleryNative::implementation
{
    int32_t Class::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void Class::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void Class::MessageBoxFromWin32()
    {
        MessageBox(
            NULL,
            (LPCWSTR)L"Resource not available\nDo you want to try again?",
            (LPCWSTR)L"Account Details",
            MB_ICONWARNING | MB_CANCELTRYCONTINUE | MB_DEFBUTTON2
        );

    }
}
