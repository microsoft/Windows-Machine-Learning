// The main message window and UI for the streaming sample. 
#include "pch.h"
#include "Capture.h"
#include "resource.h"
//#include "External/common.h"
//#include "External/trace.h"
#include <shlobj.h>
#include <Shlwapi.h>
#include <powrprof.h>
#include <KnownFolders.h>

#define USE_LOGGING

CaptureManager* g_engine = NULL;

INT_PTR CALLBACK ChooseDeviceDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#include <filesystem>
#include <shlobj.h>

static std::wstring GetLatestWinPixGpuCapturerPath_Cpp17()
{
    LPWSTR programFilesPath = nullptr;
    SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

    std::filesystem::path pixInstallationPath = programFilesPath;
    pixInstallationPath /= "Microsoft PIX";

    std::wstring newestVersionFound;

    for (auto const& directory_entry : std::filesystem::directory_iterator(pixInstallationPath))
    {
        if (directory_entry.is_directory())
        {
            if (newestVersionFound.empty() || newestVersionFound < directory_entry.path().filename().c_str())
            {
                newestVersionFound = directory_entry.path().filename().c_str();
            }
        }
    }

    if (newestVersionFound.empty())
    {
        // TODO: Error, no PIX installation found
    }

    return pixInstallationPath / newestVersionFound / L"WinPixGpuCapturer.dll";
}

INT WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR /*lpCmdLine*/, _In_ INT nCmdShow)
{
#ifdef _DEBUG
    // Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
    // This may happen if the application is launched through the PIX UI. 
    if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
    {
        LoadLibrary(GetLatestWinPixGpuCapturerPath_Cpp17().c_str());
    }
#endif

    bool coInit = false, isMFStartup = false;
    HWND hwnd;
    // Initialize the common controls
    const INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icex);

    // Note: The shell common File dialog requires apartment threading.
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
    {
        goto done;
    }
    coInit = true;

    CHECK_HR(hr = MFStartup(MF_VERSION));

    isMFStartup = true;

    hwnd = CreateMainWindow(hInstance);
    if (hwnd == 0)
    {
        ShowError(NULL, L"CreateMainWindow failed.", hr);
        goto done;
    }

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop.

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

done:
    if (FAILED(hr))
    {
        ShowError(NULL, L"Failed to start application", hr);
    }
    if (isMFStartup)
    {
        MFShutdown();
    }
    if (coInit)
    {
        CoUninitialize();
    }
    return 0;
}


// Dialog functions

HRESULT OnInitDialog(HWND hwnd, ChooseDeviceParam* pParam);
HRESULT OnOK(HWND hwnd, ChooseDeviceParam* pParam);

// Window procedure for the "Choose Device" dialog.

INT_PTR CALLBACK ChooseDeviceDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static ChooseDeviceParam* param = NULL;

    switch (msg)
    {
    case WM_INITDIALOG:
        param = (ChooseDeviceParam*)lParam;
        OnInitDialog(hwnd, param);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            OnOK(hwnd, param);
            EndDialog(hwnd, LOWORD(wParam));
            return TRUE;

        case IDCANCEL:
            EndDialog(hwnd, LOWORD(wParam));
            return TRUE;
        }
        break;
    }

    return FALSE;
}

// Handler for WM_INITDIALOG

HRESULT OnInitDialog(HWND hwnd, ChooseDeviceParam* pParam)
{
    HRESULT hr = S_OK;
    HWND list = GetDlgItem(hwnd, IDC_DEVICE_LIST);

    // Display a list of the devices.
    for (DWORD i = 0; i < pParam->count; i++)
    {
        WCHAR* friendlyName = NULL;
        UINT32 name;

        hr = pParam->devices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
            &friendlyName, &name);
        if (FAILED(hr))
        {
            break;
        }

        int index = ListBox_AddString(list, friendlyName);
        ListBox_SetItemData(list, index, i);
        CoTaskMemFree(friendlyName);
    }

    // Assume no selection for now.
    pParam->selection = (UINT32)-1;
    if (pParam->count == 0)
    {
        // If there are no devices, disable the "OK" button.
        EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
    }
    else
    {
        // Select the first device in the list.
        ListBox_SetCurSel(list, 0);
    }

    return hr;
}

// Handler for the OK button

HRESULT OnOK(HWND hwnd, ChooseDeviceParam* pParam)
{
    HWND list = GetDlgItem(hwnd, IDC_DEVICE_LIST);

    // Get the current selection and return it to the application.
    int sel = ListBox_GetCurSel(list);

    if (sel != LB_ERR)
    {
        pParam->selection = (UINT32)ListBox_GetItemData(list, sel);
    }

    return S_OK;
}


HWND CreateStatusBar(HWND hParent, UINT nID)
{
    return CreateStatusWindow(WS_CHILD | WS_VISIBLE, L"", hParent, nID);
}

BOOL StatusSetText(HWND hwnd, int iPart, const TCHAR* szText, BOOL bNoBorders = FALSE, BOOL bPopOut = FALSE)
{
    UINT flags = 0;
    if (bNoBorders)
    {
        flags |= SBT_NOBORDERS;
    }
    if (bPopOut)
    {
        flags |= SBT_POPOUT;
    }

    return (BOOL)SendMessage(hwnd, SB_SETTEXT, (WPARAM)(iPart | flags), (LPARAM)szText);
}

// Implements the window procedure for the main application window.
namespace MainWindow
{
    HWND preview = NULL;
    HWND status = NULL;
    bool previewing = false;
    com_ptr<IMFActivate> selectedDevice;

    inline void _SetStatusText(const WCHAR* szStatus)
    {
        StatusSetText(status, 0, szStatus);
    }

    void OnChooseDevice(HWND hwnd);
    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
    void OnPaint(HWND hwnd);
    void OnSize(HWND hwnd, UINT state, int cx, int cy);
    void OnDestroy(HWND hwnd);
    void OnChooseDevice(HWND hwnd);
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

    void UpdateUI(HWND hwnd)
    {
        if (g_engine->IsPreviewing() != previewing)
        {
            previewing = g_engine->IsPreviewing();
            if (previewing)
            {
                SetMenuItemText(GetMenu(hwnd), ID_CAPTURE_PREVIEW, L"Stop Preview");
            }
            else
            {
                SetMenuItemText(GetMenu(hwnd), ID_CAPTURE_PREVIEW, L"Start Preview");
            }
        }
        else if (g_engine->IsPreviewing())
        {
            _SetStatusText(L"Previewing");
        }
        else
        {
            _SetStatusText(L"Please select a device or start preview (using the default device).");
        }
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT /*lpCreateStruct*/)
    {
        BOOL                success = FALSE;
        com_ptr<IMFAttributes> attributes;
        HRESULT             hr = S_OK;

        preview = CreatePreviewWindow(GetModuleHandle(NULL), hwnd);
        if (preview == NULL)
        {
            goto done;
        }

        status = CreateStatusBar(hwnd, IDC_STATUS_BAR);
        if (status == NULL)
        {
            goto done;
        }

        CHECK_HR(hr = CaptureManager::CreateInstance(hwnd, &g_engine));

        hr = g_engine->InitializeCaptureManager(preview, status, selectedDevice.get());
        if (FAILED(hr))
        {
            ShowError(hwnd, IDS_ERR_SET_DEVICE, hr);
            goto done;
        }

        UpdateUI(hwnd);
        success = TRUE;

    done:
        return success;
    }

    void OnPaint(HWND hwnd)
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hwnd, &ps);
    }


    void OnSize(HWND /*hwnd*/, UINT state, int cx, int cy)
    {
        if (state == SIZE_RESTORED || state == SIZE_MAXIMIZED)
        {
            // Resize the status bar.
            SendMessageW(status, WM_SIZE, 0, 0);

            // Resize the preview window.
            RECT statusRect;
            SendMessageW(status, SB_GETRECT, 0, (LPARAM)&statusRect);
            cy -= (statusRect.bottom - statusRect.top);

            MoveWindow(preview, 0, 0, cx, cy, TRUE);
        }
    }

    void OnDestroy(HWND hwnd)
    {
        delete g_engine;
        g_engine = NULL;

        PostQuitMessage(0);
    }

    void OnChooseDevice(HWND hwnd)
    {
        ChooseDeviceParam param;

        com_ptr<IMFAttributes> attributes;
        INT_PTR result = NULL;

        HRESULT hr = MFCreateAttributes(attributes.put(), 1);
        if (FAILED(hr))
        {
            goto done;
        }

        // Ask for source type = video capture devices
        CHECK_HR(hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));

        // Enumerate devices.
        CHECK_HR(hr = MFEnumDeviceSources(attributes.get(), &param.devices, &param.count));

        // Ask the user to select one.
        result = DialogBoxParam(GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDD_CHOOSE_DEVICE), hwnd,
            ChooseDeviceDlgProc, (LPARAM)&param);

        if ((result == IDOK) && (param.selection != (UINT32)-1))
        {
            UINT iDevice = param.selection;

            if (iDevice >= param.count)
            {
                hr = E_UNEXPECTED;
                goto done;
            }

            CHECK_HR(hr = g_engine->InitializeCaptureManager(preview, status, param.devices[iDevice]));

            //selectedDevice.detach(); // TODO: detach instead of release? 
            selectedDevice.copy_from(param.devices[iDevice]);
        }

    done:
        if (FAILED(hr))
        {
            ShowError(hwnd, IDS_ERR_SET_DEVICE, hr);
        }
        UpdateUI(hwnd);
    }

    void OnStopPreview(HWND hwnd)
    {
        HRESULT hr = g_engine->StopPreview();
        if (FAILED(hr))
        {
            ShowError(hwnd, IDS_ERR_CAPTURE, hr);
        }
        UpdateUI(hwnd);
    }
    void OnStartPreview(HWND hwnd)
    {
        HRESULT hr = g_engine->StartPreview();
        if (FAILED(hr))
        {
            ShowError(hwnd, IDS_ERR_CAPTURE, hr);
        }
        UpdateUI(hwnd);
    }

    void OnCommand(HWND hwnd, int id, HWND /*hwndCtl*/, UINT /*codeNotify*/)
    {
        switch (id)
        {
        case ID_CAPTURE_CHOOSEDEVICE:
            OnChooseDevice(hwnd);
            break;

        case ID_CAPTURE_PREVIEW:
            if (g_engine->IsPreviewing())
            {
                OnStopPreview(hwnd);
            }
            else
            {
                OnStartPreview(hwnd);
            }
            break;
        }
    }


    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
            HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
            HANDLE_MSG(hwnd, WM_SIZE, OnSize);
            HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);

        case WM_ERASEBKGND:
            return 1;

        case WM_APP_CAPTURE_EVENT:
        {
            if (g_engine)
            {
                HRESULT hr = g_engine->OnCaptureEvent(wParam, lParam);
                if (FAILED(hr))
                {
                    ShowError(hwnd, g_engine->ErrorID(), hr);
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }

            UpdateUI(hwnd);
        }
        return 0;
        
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
};

HWND CreateMainWindow(HINSTANCE hInstance)
{
    // Register the window class.
    const wchar_t CLASS_NAME[] = L"Capture Engine Window Class";

    WNDCLASS wc = { };

    wc.lpfnWndProc = MainWindow::WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);

    RegisterClass(&wc);

    // Create the window.
    return CreateWindowEx(0, CLASS_NAME, L"Capture Application",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);
};