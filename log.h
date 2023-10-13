#pragma once

#include "includes.h"

bool SetupLog();

void Log(const string& text);

template <typename... Args>
void Log(const std::string_view formatText, Args&&... args)
{
    Log(std::vformat(formatText, std::make_format_args(args...)));
}