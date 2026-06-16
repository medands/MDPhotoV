#ifndef HELPER_H
#define HELPER_H

#include <windows.h>
#include <string>

// Just declarations! No code blocks {} here.
void SetRegistryValue(HKEY hKeyParent, const std::wstring& subKey, const std::wstring& valueName, const std::wstring& data);
void DeleteRegistryKey(HKEY hKeyParent, const std::wstring& subKey);
void RegisterFileAssociations();
void UnregisterFileAssociations();

#endif