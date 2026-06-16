#ifndef DIALOGS_H
#define DIALOGS_H
#include <windows.h>

void ShowAboutDialog(HWND owner);
void ShowHelpWindow(HWND owner);
void ShowOptionsDialog(HWND owner);

// Callbacks (declared here but defined in cpp)
<<<<<<< HEAD
INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK HelpWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK OptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
=======
LRESULT CALLBACK AboutDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK HelpWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK OptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
>>>>>>> 408d8a1537e49d6cc2d3ea5c316d3eadd095fb0c

#endif
