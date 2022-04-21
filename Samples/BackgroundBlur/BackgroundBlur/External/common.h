#pragma once
#include "logging.h"
#include "logmediatype.h"
#include <wil/cppwinrt.h> // must be before the first C++ WinRT header
#include <wil/result.h>
#include <winrt/Windows.Foundation.h>
#include <wil/resource.h>

using winrt::com_ptr;

DEFINE_GUID(TransformAsync_MFSampleExtension_Marker,
    0x1f620607, 0xa7ff, 0x4b94, 0x82, 0xf4, 0x99, 0x3f, 0x2e, 0x17, 0xb4, 0x97);
// Common macros
#define CHECK_HR(hr) if (FAILED(hr)) { goto done; }

// SAFE_RELEASE template.
// Releases a COM pointer if the pointer is not NULL, and sets the pointer to NULL.
#ifndef SAFE_RELEASE
template <class T>
inline void SAFE_RELEASE(T*& p)
{
    if (p)
    {
        p->Release();
        p = NULL;
    }
}
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(x){ delete x; x = NULL; }
#endif

// ARRAY_SIZE macro.
// Returns the size of an array (on the stack only)
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]) )
#endif


namespace MainWindow {
    inline void _SetStatusText(const WCHAR* szStatus);
}