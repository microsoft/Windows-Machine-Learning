echo on
setlocal
set BASE_TOOL_SRC_PATH=%1
set OUTPUT_PATH=%2

if not defined BASE_TOOL_SRC_PATH (
    @echo BASE_TOOL_SRC_PATH not specified.
    goto :USAGE_AND_EXIT
)
if not defined OUTPUT_PATH (
    @echo OUTPUT_PATH not specified.
    goto :USAGE_AND_EXIT
)

set VERSION_STRINGS_PATH=%OUTPUT_PATH%\GeneratedVersionStrings.h
set VERSION_INT=1,0,0,0
set VERSION_STRING=1.0.0.0

for %%P in (%%) do (
    for /F "usebackq tokens=* delims=" %%A in (`git log -1 --pretty^=format:%%Pcd^.%%Ph --date^=format:%%Py%%Pm%%Pd -- %BASE_TOOL_SRC_PATH%`) do (
        set PRODUCT_VERSION_STRING=%%A
    )
)

echo //------------------------------------------------------   > %VERSION_STRINGS_PATH%
echo // *DO NOT EDIT*                                          >> %VERSION_STRINGS_PATH%
echo // This is a Generated File                               >> %VERSION_STRINGS_PATH%
echo // Change src/GeneratedVersionStrings.cmd instead         >> %VERSION_STRINGS_PATH%
echo // *DO NOT EDIT*                                          >> %VERSION_STRINGS_PATH%
echo //------------------------------------------------------  >> %VERSION_STRINGS_PATH%
echo #define PRODUCT_VERSION_STRING "%PRODUCT_VERSION_STRING%" >> %VERSION_STRINGS_PATH%
echo #define FILE_VERSION_STRING    "%VERSION_STRING%"         >> %VERSION_STRINGS_PATH%
echo #define PRODUCT_VERSION_INT    %VERSION_INT%              >> %VERSION_STRINGS_PATH%
echo #define FILE_VERSION_INT       %VERSION_INT%              >> %VERSION_STRINGS_PATH%

goto :eof

:USAGE_AND_EXIT
    @echo ###################
    @echo Usage:
    @echo     GenerateVersionString.cmd ^<BASE_TOOL_SRC_PATH^> ^<OUTPUT_PATH^>
    @echo Arguments Received:
    @echo BASE_TOOL_SRC_PATH=%1
    @echo OUTPUT_PATH=%2
    @echo ###################

endlocal
echo on