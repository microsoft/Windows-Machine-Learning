#pragma once
#include <unknwn.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <wil/cppwinrt.h> // must be before the first C++ WinRT header
#include <wil/result.h>
#include <wil/resource.h>

using winrt::com_ptr;

// SAFE_RELEASE template.
// Releases a COM pointer if the pointer is not NULL, and sets the pointer to NULL.
//#ifndef SAFE_RELEASE
//template <class T>
//inline void SAFE_RELEASE(T*& p)
//{
//    if (p)
//    {
//        p->Release();
//        p = NULL;
//    }
//}
//#endif
