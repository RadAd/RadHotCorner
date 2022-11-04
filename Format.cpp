#include "Format.h"

#include <string>
#include <cassert>
#include <cstdarg>
#include <ctime>

void Format(std::string& buffer, _In_z_ _Printf_format_string_ char const* format, va_list args)
{
    int const _Result1 = _vscprintf_l(format, NULL, args);
    //if (buffer.size() < _Result1 + 1)
    buffer.resize(_Result1);
    int const _Result2 = _vsprintf_s_l(const_cast<char*>(buffer.data()), static_cast<size_t>(_Result1) + 1, format, NULL, args);
    assert(-1 != _Result2);
    assert(_Result1 == _Result2);
}

void Format(std::wstring& buffer, _In_z_ _Printf_format_string_ wchar_t const* format, va_list args)
{
    int const _Result1 = _vscwprintf_l(format, NULL, args);
    //if (buffer.size() < _Result1 + 1)
    buffer.resize(_Result1);
    int const _Result2 = _vswprintf_s_l(const_cast<wchar_t*>(buffer.data()), static_cast<size_t>(_Result1) + 1, format, NULL, args);
    assert(-1 != _Result2);
    assert(_Result1 == _Result2);
}

void Format(std::string& buffer, _In_z_ _Printf_format_string_ char const* format, ...)
{
    va_list args;
    va_start(args, format);
    Format(buffer, format, args);
    va_end(args);
}

void Format(std::wstring& buffer, _In_z_ _Printf_format_string_ wchar_t const* format, ...)
{
    va_list args;
    va_start(args, format);
    Format(buffer, format, args);
    va_end(args);
}

std::string Format(_In_z_ _Printf_format_string_ char const* format, ...)
{
    std::string buffer;
    va_list args;
    va_start(args, format);
    Format(buffer, format, args);
    va_end(args);
    return buffer;
}

std::wstring Format(_In_z_ _Printf_format_string_ wchar_t const* format, ...)
{
    std::wstring buffer;
    va_list args;
    va_start(args, format);
    Format(buffer, format, args);
    va_end(args);
    return buffer;
}

std::string Format(_In_z_ _Printf_format_string_ const char* fmt, const tm& tm)
{
    std::string str(100, '_');
    std::size_t size;
    while ((size = std::strftime(const_cast<char*>(str.data()), str.length() + 1, fmt, &tm)) == 0)   // C++17 data returns non-const
        str.resize(str.length() * 2, '_');
    str.resize(size);
    return str;
}

std::wstring Format(_In_z_ _Printf_format_string_ const wchar_t* fmt, const tm& tm)
{
    std::wstring str(100, L'_');
    std::size_t size;
    while ((size = std::wcsftime(const_cast<wchar_t*>(str.data()), str.length() + 1, fmt, &tm)) == 0)   // C++17 data returns non-const
        str.resize(str.length() * 2, L'_');
    str.resize(size);
    return str;
}
