/*
 * Offline test: maps a Starfield executable as an image (without running it) and runs the
 * plugin's real scanning code against it, then applies and reverts the patches.
 *
 * Usage: scan_test.exe <path to unpacked Starfield.exe>
 * The plugin log is written next to scan_test.exe (scan_test.exe.log).
 */

#include "../global.h"
#include "../init.h"
#include "../log.h"
#include "../utils.h"

#include <cstdio>

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::printf("usage: %s <Starfield.exe>\n", argv[0]);
        return 2;
    }

    const auto image = LoadLibraryExA(argv[1], nullptr, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    if (!image)
    {
        std::printf("failed to map %s (0x%lX)\n", argv[1], GetLastError());
        return 2;
    }

    if (!SetupLog())
        return 2;

    // the low bits of a resource-mapped module handle are flags
    Global::moduleBase = reinterpret_cast<uintptr_t>(image) & ~static_cast<uintptr_t>(0xFFFF);
    Log("Test image base @ 0x{:X}", Global::moduleBase);

    const bool ok = InitCapstone() && FindPatchingTargets() && FindGlobals() && FindFunctions();
    std::printf("scan: %s, %zu patch sites\n", ok ? "OK" : "FAILED", Global::targetAddresses.size());
    if (!ok)
        return 1;

    // A resource-mapped image can't be made writable, so replay every patch on a private
    // PAGE_EXECUTE_READ page (like the game's .text) holding the same byte at the same page offset.
    const auto page = static_cast<byte*>(VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    DWORD unused;
    int patched = 0, restored = 0;
    bool protectionRestored = true;
    for (const auto& [address, origAndPatch] : Global::targetAddresses)
    {
        VirtualProtect(page, 0x1000, PAGE_READWRITE, &unused);
        const auto copy = page + (reinterpret_cast<uintptr_t>(address) & 0xFFF);
        *copy = *address;
        VirtualProtect(page, 0x1000, PAGE_EXECUTE_READ, &unused);

        if (WriteToReadOnlyMemory(copy, origAndPatch.second) && *copy == origAndPatch.second)
            ++patched;
        if (WriteToReadOnlyMemory(copy, origAndPatch.first) && *copy == origAndPatch.first)
            ++restored;

        // the original code left the page RWX because the restoring VirtualProtect call failed
        MEMORY_BASIC_INFORMATION mbi{};
        VirtualQuery(copy, &mbi, sizeof mbi);
        protectionRestored &= mbi.Protect == PAGE_EXECUTE_READ;
    }

    std::printf("patch/restore: %d/%d of %zu, page protection restored: %s\n",
        patched, restored, Global::targetAddresses.size(), protectionRestored ? "yes" : "NO");

    const auto rva = [](const void* p) { return reinterpret_cast<uintptr_t>(p) - Global::moduleBase; };
    std::printf("maxShipModules rva 0x%llX, scaleformManagerPtr rva 0x%llX, ExecuteCommand rva 0x%llX\n",
        static_cast<unsigned long long>(rva(Global::maxShipModulesPtr)),
        static_cast<unsigned long long>(rva(Global::scaleformManagerPtr)),
        static_cast<unsigned long long>(rva(Global::executeCommandPtr)));

    return (patched == restored && patched == static_cast<int>(Global::targetAddresses.size()) && protectionRestored) ? 0 : 1;
}
