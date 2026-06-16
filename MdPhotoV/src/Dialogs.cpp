#include "Dialogs.h"
#include "menu.h"
#include <sstream>
#include <algorithm>

// External globals from main.cpp
extern HFONT g_hFontBold;
extern HFONT g_hFontRegular;
extern const wchar_t* g_HelpText;

const wchar_t* g_AboutText1 = 
		L"   MdPhotoV is a fast, lightweight image viewer designed to open your photos instantly and display them with crystal-clear quality.\r\n"
        L"   Built with simplicity in mind, it focuses entirely on speed, letting you flip through high-resolution picture folders smoothly and without any loading delays.";

const wchar_t* g_AboutText2 =
        L"Developed by: MDev Company.\r\n"
        L"Contact mdaissa77@gmail.com\r\n\n"
		L"Uses open-source libraries:\r\n"
    	L"- libwebp (BSD 3-Clause)\r\n"
    	L"- libavif (BSD 2-Clause)\r\n"
    	L"- MinGW-w64 Runtime (GPL v3 with Runtime Exception)\n\n"
    	L"Full license text available in 'LICENSE.txt'.";
    
void ShowAboutDialog(HWND owner) {
	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT_DIALOG), owner, AboutDlgProc);
}

void ShowHelpWindow(HWND owner) {
    static bool classRegistered = false;
    HINSTANCE hInst = GetModuleHandleW(NULL);

    if (!classRegistered) {
        WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
        wcex.lpfnWndProc = HelpWndProc;
        wcex.hInstance = hInst;
        wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
        wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
        wcex.lpszClassName = L"MdPhotoV_Help_Class";
        wcex.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(100));
        RegisterClassExW(&wcex);
        classRegistered = true;
    }

    HWND hExisting = FindWindowW(L"MdPhotoV_Help_Class", L"MdPhotoV User Documentation");
    if (hExisting) {
        SetForegroundWindow(hExisting);
        return;
    }

    HWND hHelp = CreateWindowExW(WS_EX_WINDOWEDGE, L"MdPhotoV_Help_Class", L"MdPhotoV User Documentation", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 580, 500, owner, NULL, hInst, NULL);
    if (hHelp) {
        ShowWindow(hHelp, SW_SHOWNORMAL);
        UpdateWindow(hHelp);
    }
}

void ShowOptionsDialog(HWND owner) {
    HINSTANCE hInst = GetModuleHandle(NULL);
    DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_OPTIONS_DIALOG), owner, OptionsDlgProc, 0);
}

// --- Dialog Procedures ---

INT_PTR CALLBACK AboutDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    	case WM_INITDIALOG: {
            HINSTANCE hInstance = GetModuleHandleW(NULL);

            // 1. Set the Icon
            HWND hIconCtrl = GetDlgItem(hWnd, IDC_APP_ICON);
            if (hIconCtrl) {
                HICON hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(100));
                SendMessageW(hIconCtrl, STM_SETICON, (WPARAM)hIcon, 0);
            }

            // 2. Set Fonts for specific controls (Optional if RC font is enough, but you wanted Bold for Title)
            HWND hTitleCtrl = GetDlgItem(hWnd, IDC_TITLE);
            if (hTitleCtrl && g_hFontBold) {
                SendMessageW(hTitleCtrl, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);
            }
            
            HWND hCopyCtrl = GetDlgItem(hWnd, IDC_COPYRIGHT);
            if (hCopyCtrl && g_hFontRegular) {
                SendMessageW(hCopyCtrl, WM_SETFONT, (WPARAM)g_hFontRegular, TRUE);
            }

            // 3. Set the Dynamic Text
            HWND hEdit1 = GetDlgItem(hWnd, IDC_EDIT1);
            if (hEdit1 && g_AboutText1) {
                SetWindowTextW(hEdit1, g_AboutText1);
            }

            HWND hEdit2 = GetDlgItem(hWnd, IDC_EDIT2);
            if (hEdit2 && g_AboutText2) {
                SetWindowTextW(hEdit2, g_AboutText2);
            }

            HWND hOwner = GetWindow(hWnd, GW_OWNER);
            if (hOwner) {
                RECT rcOwner, rcDlg;
                GetWindowRect(hOwner, &rcOwner);
                GetWindowRect(hWnd, &rcDlg);
                
                int x = rcOwner.left + (rcOwner.right - rcOwner.left - (rcDlg.right - rcDlg.left)) / 2;
                int y = rcOwner.top + (rcOwner.bottom - rcOwner.top - (rcDlg.bottom - rcDlg.top)) / 2;
                
                SetWindowPos(hWnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
            }
            
            SetFocus(GetDlgItem(hWnd, IDOK)); // Set focus to OK button
            return TRUE; // Focus was set
        }
        
		case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;
        case WM_DESTROY: {
            HWND hOwner = GetWindow(hWnd, GW_OWNER);
            if (hOwner != NULL) {
                EnableWindow(hOwner, TRUE);
                SetForegroundWindow(hOwner);
            }
            return 0;
        }
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

INT_PTR CALLBACK HelpWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    const int IDC_HELP_EDIT = 100;
    switch (uMsg) {
        case WM_CREATE: {
            HINSTANCE hInstance = GetModuleHandleW(NULL);
            HWND hEditWnd = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 0, 0, 0, 0, hWnd, (HMENU)IDC_HELP_EDIT, hInstance, NULL);
            SendMessageW(hEditWnd, WM_SETFONT, (WPARAM)g_hFontRegular, TRUE);
            SetWindowTextW(hEditWnd, g_HelpText);

            HWND hBtnOk = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 180, 420, 80, 26, hWnd, (HMENU)IDOK, hInstance, NULL);
            SendMessageW(hBtnOk, WM_SETFONT, (WPARAM)g_hFontRegular, TRUE);
            SetFocus(hBtnOk);
            return 0;
        }
        case WM_SIZE: {
            HWND hEditWnd = GetDlgItem(hWnd, IDC_HELP_EDIT);
            if (hEditWnd != nullptr) {
                MoveWindow(hEditWnd, 12, 12, LOWORD(lParam) - 24, HIWORD(lParam) - 24, TRUE);
            }
            return 0;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) DestroyWindow(hWnd);
            return 0;
        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;
        default:
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}

INT_PTR CALLBACK OptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                case IDC_BTN_ASSOCIATE:
                    // Call helper functions (must be declared in helper.cpp or included)
                    // Assuming they are linked from helper.cpp
                    extern void RegisterFileAssociations();
                    RegisterFileAssociations();
                    MessageBoxW(hDlg, L"File associations successfully registered!\n\nMdPhotoV is now configured to handle standard image formats inside Windows Explorer.", L"Success", MB_ICONINFORMATION);
                    return TRUE;
            }
            return FALSE;
        default:
            return FALSE;
    }
}
