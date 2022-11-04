#pragma once

#include <string>

void Format(std::string& buffer, _In_z_ _Printf_format_string_ char const* format, va_list args);
void Format(std::wstring& buffer, _In_z_ _Printf_format_string_ wchar_t const* format, va_list args);
void Format(std::string& buffer, _In_z_ _Printf_format_string_ char const* format, ...);
void Format(std::wstring& buffer, _In_z_ _Printf_format_string_ wchar_t const* format, ...);
std::string Format(_In_z_ _Printf_format_string_ char const* format, ...);
std::wstring Format(_In_z_ _Printf_format_string_ wchar_t const* format, ...);
std::string Format(_In_z_ _Printf_format_string_ const char* fmt, const tm& tm);
std::wstring Format(_In_z_ _Printf_format_string_ const wchar_t* fmt, const tm& tm);
