#pragma once

#include <Windows.h>

// Recommended to add this to compiler options:
// /d1trimfile:"$(SolutionDir)\"

struct SourceLocation
{
    SourceLocation(int line, const TCHAR* file, const TCHAR* function, const TCHAR* funcsig)
        : line(line)
        , file(file)
        , function(function)
        , funcsig(funcsig)
    {
    }

    int line;
    const TCHAR* file;
    const TCHAR* function;
    const TCHAR* funcsig;
};

#define SRC_LOC SourceLocation(__LINE__, _T(__FILE__), _T(__FUNCTION__), _T(__FUNCSIG__))

#undef ERROR
enum class LogLevel
{
    DEBUG,
    INFO,
    WARN,
    ERROR,
    NONE,
};

extern LogLevel LogDebug;
extern LogLevel LogStdout;
extern LogLevel LogMsgBox;

void InitLog(HWND hWnd, LPCTSTR app_name);

void Log(LogLevel level, LPCTSTR expr, const SourceLocation& src_loc);
void Log(LogLevel level, LPCTSTR expr, const SourceLocation& src_loc, LPCTSTR format, ...);

#define LOG(l, x, ...) Log(l, x, SRC_LOC, __VA_ARGS__)
#define CHECK(l, b, ...) if (!(b)) { LOG(l, TEXT(#b), __VA_ARGS__); }
#define CHECK_RET(l, b, r, ...) if (!(b)) { LOG(l, TEXT(#b), __VA_ARGS__); return r; }
#define CHECK_THROW(l, b, t, ...) if (!(b)) { LOG(l, TEXT(#b), __VA_ARGS__); throw t; }
