#include "AppWindow.h"
#include "helper.h"

// Global Font Assets
HFONT g_hFontBold = nullptr;
HFONT g_hFontRegular = nullptr;

// Embedded Help Documentation Text
const wchar_t* g_HelpText = 
    L"MdPhotoV User Documentation\r\n"
    L"============================\r\n\r\n"
    L"Keyboard Controls:\r\n"
    L"  Left Arrow\t: Navigate to the Previous Image\r\n"
    L"  Right Arrow\t: Navigate to the Next Image\r\n"
    L"  Ctrl + O\t\t: Open a single Image file\r\n"
    L"  Ctrl + P\t\t: Print the current active Image\r\n"
    L"  Ctrl + T\t\t: Open Application Options\r\n"
    L"  Ctrl + C\t\t: Copy Image file to Windows Clipboard\r\n"
    L"  Ctrl + Del\t: Permanently Delete current file\r\n"
    L"  F1 /Ctrl + M\t: Manual help\r\n"
    L"  F10 \t: About app\r\n"
    L"  F11 / Esc\t: Toggle / Exit Borderless Full Screen mode\r\n\r\n"
    L"Mouse Navigation:\r\n"
    L"  Mouse Wheel\t: Zoom In / Zoom Out dynamically\r\n"
    L"  Drag & Drop\t: Drop any supported image directly onto the window\r\n";

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    if (argv && argc > 1) {
        std::wstring arg1 = argv[1];

        if (arg1 == L"--register" || arg1 == L"/register") {
            {
                AppWindow app; 
                RegisterFileAssociations();
            } 
            LocalFree(argv);    
            return 0;           
        }
        
        if (arg1 == L"--unregister" || arg1 == L"/unregister") {
            {
                AppWindow app;
                UnregisterFileAssociations();
            }
            LocalFree(argv);
            return 0;           
        }
    }

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    g_hFontRegular = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    g_hFontBold = CreateFontW(-18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    {
        AppWindow app;

        if (SUCCEEDED(app.Initialize(hInstance))) {
            if (argv && argc > 1) {
                app.LoadImageFromFile(argv[0]);
            }

            ShowWindow(app.GetHwnd(), nCmdShow);
            UpdateWindow(app.GetHwnd());

            app.RunMessageLoop();
        }
    }

    if (g_hFontRegular) DeleteObject(g_hFontRegular);
    if (g_hFontBold)    DeleteObject(g_hFontBold);
    if (argv) LocalFree(argv);

    CoUninitialize();
    return 0;
}
