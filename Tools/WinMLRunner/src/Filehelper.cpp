#include "Filehelper.h"
#include <libloaderapi.h>
#include <stdlib.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace FileHelper
{
    std::wstring GetModulePath()
    {
        std::wstring val;
        wchar_t modulePath[MAX_PATH] = { 0 };
        GetModuleFileNameW((HINSTANCE)&__ImageBase, modulePath, _countof(modulePath));
        wchar_t drive[_MAX_DRIVE];
        wchar_t dir[_MAX_DIR];
        wchar_t filename[_MAX_FNAME];
        wchar_t ext[_MAX_EXT];
        errno_t err = _wsplitpath_s(modulePath, drive, _MAX_DRIVE, dir, _MAX_DIR, filename, _MAX_FNAME, ext, _MAX_EXT);

        val = drive;
        val += dir;

        return val;
    }
    std::wstring GetAbsolutePath(std::wstring relativePath)
    {
        TCHAR** lppPart = { NULL };
        wchar_t absolutePath[MAX_PATH] = { 0 };
        errno_t err = GetFullPathName(relativePath.c_str(), MAX_PATH, absolutePath, lppPart);
        if (err == 0)
        {
            throw HRESULT_FROM_WIN32(GetLastError());
        }
        return absolutePath;
    }
} // namespace FileHelper
