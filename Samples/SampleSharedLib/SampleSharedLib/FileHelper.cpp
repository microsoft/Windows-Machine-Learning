#include "pch.h"
#include "FileHelper.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

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
}