#pragma once

#include "includes.h"

inline std::tuple<byte*, uint32_t> GetSectionInfo(const string& section)
{
    const auto moduleBase = GetModuleHandleA(nullptr);
    const auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(moduleBase);
    const auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<byte*>(moduleBase) + dosHeader->e_lfanew);

    const auto sectionHeader = reinterpret_cast<IMAGE_SECTION_HEADER*>(reinterpret_cast<byte*>(&ntHeaders->OptionalHeader) + ntHeaders->FileHeader.SizeOfOptionalHeader);
    uint32_t sectionStart = 0;
    uint32_t sectionSize = 0;

    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i)
    {
        if (strcmp(reinterpret_cast<const char*>(sectionHeader[i].Name), section.c_str()) == 0)
        {
            sectionStart = sectionHeader[i].VirtualAddress;
            sectionSize = sectionHeader[i].SizeOfRawData;
            break;
        }
    }

    return { reinterpret_cast<byte*>(moduleBase) + sectionStart, sectionSize };
}

inline byte* PatternScanSectionExact(const std::vector<byte>& pattern, const string& section)
{
    const auto& [sectionStart, sectionSize] = GetSectionInfo(section);

    for (auto curPtr = sectionStart; curPtr < sectionStart + sectionSize - pattern.size(); ++curPtr)
    {
        bool found = true;

        for (uint32_t idxPattern = 0; idxPattern < pattern.size(); ++idxPattern)
        {
            const auto doesMatchPattern = *(curPtr + idxPattern) == pattern[idxPattern];
            if (doesMatchPattern)
                continue;

            found = false;
            break;
        }

        if (!found)
            continue;
        
        return curPtr;
    }

    return nullptr;
}

inline byte* PatternScanFromStartExact(const std::vector<byte>& pattern, byte* start, const uint32_t maxLength)
{
    for (auto curPtr = start; curPtr < start + maxLength - pattern.size(); ++curPtr)
    {
        bool found = true;

        for (uint32_t idxPattern = 0; idxPattern < pattern.size(); ++idxPattern)
        {
            const auto doesMatchPattern = *(curPtr + idxPattern) == pattern[idxPattern];
            if (doesMatchPattern)
                continue;

            found = false;
            break;
        }

        if (!found)
            continue;

        return curPtr;
    }

    return nullptr;
}

inline byte* PatternScanExactReverse(const std::vector<byte>& pattern, byte* start, const uint32_t maxLength)
{
    for (auto curPtr = start - pattern.size(); curPtr > start - maxLength; --curPtr)
    {
        bool found = true;

        for (uint32_t idxPattern = 0; idxPattern < pattern.size(); ++idxPattern)
        {
            const auto doesMatchPattern = *(curPtr + idxPattern) == pattern[idxPattern];
            if (doesMatchPattern)
                continue;

            found = false;
            break;
        }

        if (!found)
            continue;

        return curPtr;
    }

    return nullptr;
}

inline byte* PatternScanSection(const string& pattern, const string& section)
{
    constexpr uint16_t wildcardByte = 0x100;

    static auto IDAPatternToBytes = [](const string& idaPattern)
        {
            auto bytes = std::vector<uint16_t>{}; // 0x00-0xFF might all occur, extra "wildcard" byte needed
            const auto start = const_cast<char*>(idaPattern.c_str());
            const auto end = const_cast<char*>(idaPattern.c_str()) + idaPattern.size();

            for (auto current = start; current < end; ++current)
            {
                if (*current == '?')
                {
                    ++current;

                    if (*current == '?')
                        ++current;

                    bytes.push_back(wildcardByte);
                }
                else
                {
                    bytes.push_back(static_cast<uint16_t>(strtoul(current, &current, 16)));
                }
            }

            return bytes;
        };

    const auto& [sectionStart, sectionSize] = GetSectionInfo(section);
    const auto patternBytes = IDAPatternToBytes(pattern);

    for (auto curPtr = sectionStart; curPtr < sectionStart + sectionSize - patternBytes.size(); ++curPtr)
    {
        bool found = true;

        for (uint32_t idxPattern = 0; idxPattern < patternBytes.size(); ++idxPattern)
        {
            const auto doesMatchPattern = patternBytes[idxPattern] == wildcardByte || *(curPtr + idxPattern) == patternBytes[idxPattern];
            if (doesMatchPattern)
                continue;

            found = false;
            break;
        }

        if (!found)
            continue;

        return curPtr;
    }

    return nullptr;
}

inline byte* PatternScan(const string& pattern)
{
    return PatternScanSection(pattern, ".text");
}

inline byte* FindStringA(const string& text)
{
    std::vector<byte> stringBytes{};
    stringBytes.emplace_back(0); // add a null-terminator at the start (to prevent substring-results)
    for (size_t i = 0; i < text.size(); ++i)
    {
        stringBytes.emplace_back(text.c_str()[i]);
    }
    stringBytes.emplace_back(0); // add null-terminator at the end

    const auto result = PatternScanSectionExact(stringBytes, ".rdata");

    if (!result)
        return nullptr;

    return result + 1; // +1 offset because of the added null-terminator at the start of the pattern
}

inline byte* FindLeaReferenceToAddress(const byte* address)
{
    constexpr auto leaInsnSize = 7;

    const auto& [sectionStart, sectionSize] = GetSectionInfo(".text");

    for (auto curPtr = sectionStart; curPtr < sectionStart + sectionSize - leaInsnSize; ++curPtr)
    {
        const auto isLeaOpcode = (*curPtr == 0x4C || *curPtr == 0x48) && *(curPtr + 1) == 0x8D;
        if (!isLeaOpcode)
            continue;
        
        const auto ripOffset = *reinterpret_cast<uint32_t*>(curPtr + leaInsnSize - sizeof(uint32_t));
        const byte* referencedAddress = curPtr + ripOffset + leaInsnSize;

        if (referencedAddress != address)
            continue;

        return curPtr;
    }

    return nullptr;
}

inline byte* FindStringReferenceA(const string& text)
{
    const auto stringAddr = FindStringA(text);
    if (!stringAddr)
        return nullptr;

    const auto refAddr = FindLeaReferenceToAddress(stringAddr);

    return refAddr;
}

template <typename T>
void WriteToReadOnlyMemory(void* address, T value)
{
    DWORD oldProtect;
    VirtualProtect(address, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy_s(address, sizeof(T), &value, sizeof(T));
    VirtualProtect(address, 1, oldProtect, nullptr);
}