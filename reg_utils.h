#pragma once

#include <Windows.h>
#include <memory>

struct HKEYDeleter
{
    typedef HKEY pointer;
    void operator()(HKEY hKey) const noexcept { RegCloseKey(hKey); }
};

typedef std::unique_ptr<HKEY, HKEYDeleter> HKEYPtr;

inline LRESULT RegGetString(_In_ HKEY hKey, _In_opt_ LPCTSTR lpValueName, LPTSTR pStr, DWORD size)
{
    TCHAR buf[1024];
    DWORD bufsize = ARRAYSIZE(buf) * sizeof(TCHAR);
    LRESULT ret = RegQueryValueEx(hKey, lpValueName, nullptr, nullptr, (LPBYTE) buf, &bufsize);
    if (ret == ERROR_SUCCESS)
        ExpandEnvironmentStrings(buf, pStr, size);
    return ret;
}

inline DWORD RegGetDWORD(_In_ HKEY hKey, _In_opt_ LPCTSTR lpValueName, DWORD defvalue)
{
    DWORD value = defvalue;
    DWORD valuesize = sizeof(value);
    LRESULT ret = RegQueryValueEx(hKey, lpValueName, nullptr, nullptr, (LPBYTE) &value, &valuesize);
    return (ret == ERROR_SUCCESS) ? value : defvalue;
}
