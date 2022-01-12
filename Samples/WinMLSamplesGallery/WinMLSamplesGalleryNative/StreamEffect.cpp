#include "pch.h"
#include "StreamEffect.h"
#include "StreamEffect.g.cpp"

#include <commctrl.h>
#include <mfapi.h>
#include <powrprof.h>


#include "Capture.h"
#include "common.h"
#include "Resource.h"

CaptureManager* g_pEngine = NULL;
winrt::hstring         g_modelPath = L"";
HPOWERNOTIFY    g_hPowerNotify = NULL;
HPOWERNOTIFY    g_hPowerNotifyMonitor = NULL;
SYSTEM_POWER_CAPABILITIES   g_pwrCaps{};
bool            g_fSleepState = false;


// Forward declarations
INT_PTR CALLBACK ChooseDeviceDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Dialog functions
HRESULT OnInitDialog(HWND hwnd, ChooseDeviceParam* pParam);
HRESULT OnOK(HWND hwnd, ChooseDeviceParam* pParam);

static HMODULE GetCurrentModule()
{ // NB: XP+ solution!
    HMODULE hModule = NULL;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCTSTR)GetCurrentModule,
        &hModule);

    return hModule;
}

INT_PTR CALLBACK ChooseDeviceDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static ChooseDeviceParam* pParam = NULL;

    switch (msg)
    {
    case WM_INITDIALOG:
        pParam = (ChooseDeviceParam*)lParam;
        OnInitDialog(hwnd, pParam);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            OnOK(hwnd, pParam);
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

    HWND hList = GetDlgItem(hwnd, IDC_DEVICE_LIST);

    // Display a list of the devices.

    for (DWORD i = 0; i < pParam->count; i++)
    {
        WCHAR* szFriendlyName = NULL;
        UINT32 cchName;

        hr = pParam->ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
            &szFriendlyName, &cchName);
        if (FAILED(hr))
        {
            break;
        }

        int index = ListBox_AddString(hList, szFriendlyName);

        ListBox_SetItemData(hList, index, i);

        CoTaskMemFree(szFriendlyName);
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
        ListBox_SetCurSel(hList, 0);
    }

    return hr;
}

// Handler for the OK button
HRESULT OnOK(HWND hwnd, ChooseDeviceParam* pParam)
{
    HWND hList = GetDlgItem(hwnd, IDC_DEVICE_LIST);

    // Get the current selection and return it to the application.
    int sel = ListBox_GetCurSel(hList);

    if (sel != LB_ERR)
    {
        pParam->selection = (UINT32)ListBox_GetItemData(hList, sel);
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

namespace MainWindow
{
    HWND hPreview = NULL;
    HWND hStatus = NULL;
    bool bRecording = false;
    bool bPreviewing = false;
    winrt::com_ptr<IMFActivate> pSelectedDevice;

    wchar_t PhotoFileName[MAX_PATH];

    inline void _SetStatusText(const WCHAR* szStatus)
    {
        StatusSetText(hStatus, 0, szStatus);
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
        /*if (g_pEngine->IsRecording() != bRecording)
        {
            bRecording = g_pEngine->IsRecording();
            if (bRecording)
            {
                SetMenuItemText(GetMenu(hwnd), ID_CAPTURE_RECORD, L"Stop Recording");
            }
            else
            {
                SetMenuItemText(GetMenu(hwnd), ID_CAPTURE_RECORD, L"Start Recording");
            }
        }*/

        if (g_pEngine->IsPreviewing() != bPreviewing)
        {
            bPreviewing = g_pEngine->IsPreviewing();
            if (bPreviewing)
            {
                SetMenuItemText(GetMenu(hwnd), ID_CAPTURE_PREVIEW, L"Stop Preview");
            }
            else
            {
                SetMenuItemText(GetMenu(hwnd), ID_CAPTURE_PREVIEW, L"Start Preview");
            }
        }
        BOOL bEnableRecording = TRUE;
        BOOL bEnablePhoto = TRUE;

        if (bRecording)
        {
            _SetStatusText(L"Recording");
        }
        else if (g_pEngine->IsPreviewing())
        {
            _SetStatusText(L"Previewing");
        }
        else
        {
            _SetStatusText(L"Please select a device or start preview (using the default device).");
            bEnableRecording = FALSE;
        }

        if (!g_pEngine->IsPreviewing() || g_pEngine->IsPhotoPending())
        {
            bEnablePhoto = FALSE;
        }

        /*EnableMenuItem(GetMenu(hwnd), ID_CAPTURE_RECORD, bEnableRecording ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(GetMenu(hwnd), ID_CAPTURE_TAKEPHOTO, bEnablePhoto ? MF_ENABLED : MF_GRAYED);*/
    }


    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT /*lpCreateStruct*/)
    {
        BOOL                fSuccess = FALSE;
        winrt::com_ptr<IMFAttributes> pAttributes;
        HRESULT             hr = S_OK;

        // TODO: GetModuleHandle(NULL)
        hPreview = CreatePreviewWindow(GetCurrentModule(), hwnd);
        if (hPreview == NULL)
        {
            goto done;
        }

        hStatus = CreateStatusBar(hwnd, IDC_STATUS_BAR);
        if (hStatus == NULL)
        {
            goto done;
        }

        CHECK_HR(hr = CaptureManager::CreateInstance(hwnd, &g_pEngine));

        hr = g_pEngine->InitializeCaptureManager(hPreview, pSelectedDevice.get());
        if (FAILED(hr))
        {
            ShowError(hwnd, IDS_ERR_SET_DEVICE, hr);
            goto done;
        }

        // Register for connected standy changes.  This should come through the normal
        // WM_POWERBROADCAST messages that we're already handling below.
        // We also want to hook into the monitor on/off notification for AOAC (SOC) systems.
        g_hPowerNotify = RegisterSuspendResumeNotification((HANDLE)hwnd, DEVICE_NOTIFY_WINDOW_HANDLE);
        g_hPowerNotifyMonitor = RegisterPowerSettingNotification((HANDLE)hwnd, &GUID_MONITOR_POWER_ON, DEVICE_NOTIFY_WINDOW_HANDLE);
        GetPwrCapabilities(&g_pwrCaps);

        UpdateUI(hwnd);
        fSuccess = TRUE;

    done:
        return fSuccess;
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
            SendMessageW(hStatus, WM_SIZE, 0, 0);

            // Resize the preview window.
            RECT statusRect;
            SendMessageW(hStatus, SB_GETRECT, 0, (LPARAM)&statusRect);
            cy -= (statusRect.bottom - statusRect.top);

            MoveWindow(hPreview, 0, 0, cx, cy, TRUE);
        }
    }

    void OnDestroy(HWND hwnd)
    {
        delete g_pEngine;
        g_pEngine = NULL;

        if (g_hPowerNotify)
        {
            UnregisterSuspendResumeNotification(g_hPowerNotify);
            g_hPowerNotify = NULL;
        }
        PostQuitMessage(0);
    }

    void OnChooseDevice(HWND hwnd)
    {
        ChooseDeviceParam param;

        winrt::com_ptr<IMFAttributes> pAttributes;
        INT_PTR result = NULL;

        HRESULT hr = MFCreateAttributes(pAttributes.put(), 1);
        if (FAILED(hr))
        {
            goto done;
        }

        // Ask for source type = video capture devices
        CHECK_HR(hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));

        // Enumerate devices.
        CHECK_HR(hr = MFEnumDeviceSources(pAttributes.get(), &param.ppDevices, &param.count));

        // Ask the user to select one.
        result = DialogBoxParam(GetCurrentModule(),
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

            CHECK_HR(hr = g_pEngine->InitializeCaptureManager(hPreview, param.ppDevices[iDevice]));

            pSelectedDevice.detach(); // TODO: detach instead of release? 
            pSelectedDevice.attach(param.ppDevices[iDevice]);
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
        HRESULT hr = g_pEngine->StopPreview();
        if (FAILED(hr))
        {
            ShowError(hwnd, IDS_ERR_RECORD, hr);
        }
        UpdateUI(hwnd);
    }
    void OnStartPreview(HWND hwnd)
    {
        HRESULT hr = g_pEngine->StartPreview(g_modelPath);
        if (FAILED(hr))
        {
            ShowError(hwnd, IDS_ERR_RECORD, hr);
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

        /*case ID_CAPTURE_RECORD:
            if (g_pEngine->IsRecording())
            {
                OnStopRecord(hwnd);
            }
            else
            {
                OnStartRecord(hwnd);
            }
            break;

        case ID_CAPTURE_TAKEPHOTO:
            OnTakePhoto(hwnd);
            break;*/
        case ID_CAPTURE_PREVIEW:
            if (g_pEngine->IsPreviewing())
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
            if (g_pEngine)
            {
                HRESULT hr = g_pEngine->OnCaptureEvent(wParam, lParam);
                if (FAILED(hr))
                {
                    ShowError(hwnd, g_pEngine->ErrorID(), hr);
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }

            UpdateUI(hwnd);
        }
        return 0;
        case WM_POWERBROADCAST:
        {
            switch (wParam)
            {
            case PBT_APMSUSPEND:
                DbgPrint(L"++WM_POWERBROADCAST++ Stopping both preview & record stream.\n");
                g_fSleepState = true;
                g_pEngine->SleepState(g_fSleepState);
                g_pEngine->StopRecord();
                g_pEngine->StopPreview();
                g_pEngine->DestroyCaptureEngine();
                DbgPrint(L"++WM_POWERBROADCAST++ streams stopped, capture engine destroyed.\n");
                break;
            case PBT_APMRESUMEAUTOMATIC:
                DbgPrint(L"++WM_POWERBROADCAST++ Reinitializing capture engine.\n");
                g_fSleepState = false;
                g_pEngine->SleepState(g_fSleepState);
                g_pEngine->InitializeCaptureManager(hPreview, pSelectedDevice.get());
                break;
            case PBT_POWERSETTINGCHANGE:
            {
                // We should only be in here for GUID_MONITOR_POWER_ON.
                POWERBROADCAST_SETTING* pSettings = (POWERBROADCAST_SETTING*)lParam;

                // If this is a SOC system (AoAc is true), we want to check our current
                // sleep state and based on whether the monitor is being turned on/off,
                // we can turn off our media streams and/or re-initialize the capture
                // engine.
                if (pSettings != NULL && g_pwrCaps.AoAc && pSettings->PowerSetting == GUID_MONITOR_POWER_ON)
                {
                    DWORD   dwData = *((DWORD*)pSettings->Data);
                    if (dwData == 0 && !g_fSleepState)
                    {
                        // This is a AOAC machine, and we're about to turn off our monitor, let's stop recording/preview.
                        DbgPrint(L"++WM_POWERBROADCAST++ Stopping both preview & record stream.\n");
                        g_fSleepState = true;
                        g_pEngine->SleepState(g_fSleepState);
                        g_pEngine->StopRecord();
                        g_pEngine->StopPreview();
                        g_pEngine->DestroyCaptureEngine();
                        DbgPrint(L"++WM_POWERBROADCAST++ streams stopped, capture engine destroyed.\n");
                    }
                    else if (dwData != 0 && g_fSleepState)
                    {
                        DbgPrint(L"++WM_POWERBROADCAST++ Reinitializing capture engine.\n");
                        g_fSleepState = false;
                        g_pEngine->SleepState(g_fSleepState);
                        g_pEngine->InitializeCaptureManager(hPreview, pSelectedDevice.get());
                    }
                }
            }
            break;
            case PBT_APMRESUMESUSPEND:
            default:
                // Don't care about this one, we always get the resume automatic so just
                // latch onto that one.
                DbgPrint(L"++WM_POWERBROADCAST++ (wParam=%u,lParam=%u)\n", wParam, lParam);
                break;
            }
        }
        return 1;
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
};


namespace winrt::WinMLSamplesGalleryNative::implementation
{
    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
    {

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    };

    void StreamEffect::LaunchNewWindow(hstring modelPath)
    {
        HWND hwnd;
        HWND galleryHwnd = GetActiveWindow();
        HRESULT hr = S_OK;
        BOOL bMFStartup = false;
        HMODULE hmodule =NULL;
        GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            (LPCTSTR)"GetCurrentModule",
            &hmodule);
        g_modelPath = modelPath;

        // Initialize the common controls
        const INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES };
        InitCommonControlsEx(&icex);

        hr = MFStartup(MF_VERSION);
        if (FAILED(hr))
        {
            goto done;
        }

        // Create window
        const wchar_t CLASS_NAME[] = L"Capture Engine Window Class";
        INT nCmdShow = 1;

        WNDCLASS wc = { 0 };

        wc.lpfnWndProc = MainWindow::WindowProc;
        wc.hInstance = hmodule;
        wc.lpszClassName = CLASS_NAME;
        wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);

        RegisterClass(&wc);
        HMENU menu = LoadMenu(hmodule, MAKEINTRESOURCE(IDR_MENU1));
        if (!menu) {
            LPVOID lpMsgBuf;
            LPVOID lpDisplayBuf;

            DWORD errCode = GetLastError();
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                errCode,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf,
                0, NULL);
            lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
                (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)"error") + 40) * sizeof(TCHAR));
            StringCchPrintf((LPTSTR)lpDisplayBuf,
                LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                TEXT("%s failed with error %d: %s"),
                "errpr", errCode, lpMsgBuf);
        }

        hwnd = CreateWindowEx(
            0, CLASS_NAME, L"Capture Application", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT, galleryHwnd, NULL, hmodule, NULL
        );

        if (hwnd == 0)
        {
            throw_hresult(E_FAIL);
        }

        ShowWindow(hwnd, 10);

        // Run the main message loop
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }


    done: 
        if (FAILED(hr))
        {
            // TODO: Add utils from BackgroundBlur sample utils.cpp
            //ShowError(NULL, L"Failed to start application", hr);
        }
        if (bMFStartup)
        {
            MFShutdown();
        }
        return ;
    }
}
