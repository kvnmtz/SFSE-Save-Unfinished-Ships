#include "log.h"

std::ofstream logStream;

string GetCurrentTimeString()
{
    const auto currentTime = std::time(nullptr);
    const auto localTime = std::localtime(&currentTime);

    char buffer[10];
    std::strftime(buffer, sizeof buffer, "%H:%M:%S", localTime);

    return { buffer };
}

string GetModuleFilePath()
{
    HMODULE handle;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(&GetModuleFilePath), &handle))
    {
        MessageBoxA(nullptr, std::format("GetModuleHandleExA failed (0x{:X})", GetLastError()).c_str(), "Save-Unfinished-Ships", MB_OK | MB_ICONERROR);
        return "";
    }

    char path[MAX_PATH];
    if (!GetModuleFileNameA(handle, path, sizeof path))
    {
        MessageBoxA(nullptr, std::format("GetModuleFileNameA failed (0x{:X})", GetLastError()).c_str(), "Save-Unfinished-Ships", MB_OK | MB_ICONERROR);
        return "";
    }

    return path;
}

string GetLogFilePath()
{
    auto path = GetModuleFilePath();
    if (path.empty())
        return "";

    path = path.replace(path.find(".dll"), 4, ".log");
    return path;
}

bool SetupLog()
{
    const auto path = GetLogFilePath();
    if (path.empty())
        return false;

    logStream = std::ofstream(path.c_str(), std::ios_base::out | std::ios_base::trunc);
    return true;
}

void Log(const string& text)
{
    logStream << "[" << GetCurrentTimeString() << "] " << text << std::endl;
}
