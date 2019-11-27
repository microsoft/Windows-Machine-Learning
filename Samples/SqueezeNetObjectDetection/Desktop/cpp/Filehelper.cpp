#include "pch.h"
#include "Filehelper.h"
#include <libloaderapi.h>
#include <stdlib.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace FileHelper
{
    std::string GetModulePath()
    {
        std::string val;
        char modulePath[MAX_PATH] = {};
        GetModuleFileNameA(NULL, modulePath, ARRAYSIZE(modulePath));
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char filename[_MAX_FNAME];
        char ext[_MAX_EXT];
        _splitpath_s(modulePath, drive, _MAX_DRIVE, dir, _MAX_DIR, filename, _MAX_FNAME, ext, _MAX_EXT);

        val = drive;
        val += dir;
        return val;
    }
} // namespace FileHelper
