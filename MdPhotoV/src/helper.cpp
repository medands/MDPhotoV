
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <windows.h>
#include <string>
#include <winreg.h>
#include <vector>
#include <shlobj.h> // Required for SHChangeNotify

// Helper function to set a registry string value
bool SetRegistryValue(HKEY hKeyRoot, const std::wstring& subKey, const std::wstring& valueName, const std::wstring& data) {
    HKEY hKey;
    LONG result = RegCreateKeyExW(hKeyRoot, subKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    if (result != ERROR_SUCCESS) return false;

    result = RegSetValueExW(hKey, valueName.empty() ? NULL : valueName.c_str(), 0, REG_SZ, 
        reinterpret_cast<const BYTE*>(data.c_str()), static_cast<DWORD>((data.length() + 1) * sizeof(wchar_t)));
    
    RegCloseKey(hKey);
    return result == ERROR_SUCCESS;
}

// Helper function to recursively delete a registry key
bool DeleteRegistryKey(HKEY hKeyRoot, const std::wstring& subKey) {
    return RegDeleteTreeW(hKeyRoot, subKey.c_str()) == ERROR_SUCCESS;
}

void RegisterFileAssociations() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring sExePath = exePath;

    // Format the open execution command string: "C:\Path\To\MdPhotoV.exe" "%1"
    std::wstring openCmd = L"\"" + sExePath + L"\" \"%1\"";
    std::wstring progId = L"MdPhotoV.Viewer";

    // 1. Create the ProgID and set the application execution target
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Classes\\" + progId, L"", L"MdPhotoV Image File");
    SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Classes\\" + progId + L"\\shell\\open\\command", L"", openCmd);

    // 2. Associate specific target file extensions with your ProgID
    std::vector<std::wstring> extensions = { L".jpg", L".jpeg", L".png", L".webp", L".bmp", L".avif", L".gif" };
    for (const auto& ext : extensions) {
        SetRegistryValue(HKEY_CURRENT_USER, L"Software\\Classes\\" + ext, L"", progId);
    }

    // 3. Force the Windows Shell to refresh immediately to display the correct icons
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

void UnregisterFileAssociations() {
    std::wstring progId = L"MdPhotoV.Viewer";

    // 1. Remove the core ProgID profile tree
    DeleteRegistryKey(HKEY_CURRENT_USER, L"Software\\Classes\\" + progId);

    // 2. Clear default program mappings for extensions (leaves clean for other apps)
    std::vector<std::wstring> extensions = { L".jpg", L".jpeg", L".png", L".webp", L".avif", L".bmp", L".gif" };
    for (const auto& ext : extensions) {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, (L"Software\\Classes\\" + ext).c_str(), 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            wchar_t value[256];
            DWORD size = sizeof(value);
            // Only clear the key if it still points to your specific application
            if (RegQueryValueExW(hKey, NULL, NULL, NULL, reinterpret_cast<BYTE*>(value), &size) == ERROR_SUCCESS) {
                if (progId == value) {
                    RegDeleteValueW(hKey, NULL); // Clear the default string association
                }
            }
            RegCloseKey(hKey);
        }
    }

    // 3. Force Windows to reload icon definitions
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}