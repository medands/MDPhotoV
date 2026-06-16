#ifndef DIALOGS_H
#define DIALOGS_H
#include <windows.h>

void ShowAboutDialog(HWND owner);
void ShowHelpWindow(HWND owner);
void ShowOptionsDialog(HWND owner);

// Callbacks (declared here but defined in cpp)
LRESULT CALLBACK AboutDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK HelpWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK OptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif
