#include "init.h"

#include "includes.h"

#include "global.h"
#include "log.h"
#include "utils.hpp"

CX86Disasm64 dis;

bool InitCapstone()
{
    if (dis.GetError())
    {
        Log("Failed to initialize capstone disassembler");
        return false;
    }

    dis.SetDetail(CS_OPT_ON);
    dis.SetSyntax(CS_OPT_SYNTAX_INTEL);

    return true;
}

bool FindPatchingTargets()
{
    Log("Searching addresses to patch...");

    for (const auto& [text, byteToReplace] : Global::jmpPatchingTargets)
    {
        const auto stringRef = FindStringReferenceA(text);
        if (!stringRef)
        {
            Log("Couldn't find string reference to {}", text);
            return false;
        }

        const auto patchAddress = PatternScanExactReverse(std::vector(1, byteToReplace), stringRef, 0x10);

        Log("Found address to patch (jmp) {} @ 0x{:X}", text, reinterpret_cast<uintptr_t>(patchAddress));

        Global::targetAddresses.try_emplace(patchAddress, std::make_pair(byteToReplace, 0xEB));
    }

    for (const auto& [text, insnToReplace] : Global::nopPatchingTargets)
    {
        const auto stringRef = FindStringReferenceA(text);
        if (!stringRef)
        {
            Log("Couldn't find string reference to {}", text);
            return false;
        }

        const auto previousIntInsn = PatternScanExactReverse(std::vector<byte>(1, 0xCC), stringRef, 0x500);

        const auto insns = dis.Disasm(previousIntInsn, stringRef - previousIntInsn, reinterpret_cast<size_t>(previousIntInsn));

        for (size_t i = insns->Count - 1; i > 0; --i)
        {
            const auto curInsn = insns->Instructions(i);
            if (string(curInsn->mnemonic) != insnToReplace)
                continue;

            const auto insnAddr = reinterpret_cast<byte*>(curInsn->address);

            Log("Found address to patch (nop) {} @ 0x{:X} ({} bytes)", text, reinterpret_cast<uintptr_t>(insnAddr), curInsn->size);

            for (auto addr = insnAddr; addr < insnAddr + curInsn->size; ++addr)
            {
                Global::targetAddresses.try_emplace(addr, std::make_pair(*addr, 0x90));
            }

            break;
        }
    }

    return true;
}

bool FindFunctions()
{
    Log("Searching function addresses...");

    const auto stringRef = FindStringReferenceA("SaveLoadTester Step");
    if (!stringRef)
    {
        Log("Failed to find target string reference");
        return false;
    }

    const auto followingIntInsn = PatternScanFromStartExact(std::vector<byte>(1, 0xCC), stringRef, 0x200);
    if (!followingIntInsn)
    {
        Log("Failed to find function end");
        return false;
    }

    const auto insns = dis.Disasm(stringRef, followingIntInsn - stringRef, reinterpret_cast<size_t>(stringRef));

    for (size_t i = 0; i < insns->Count; ++i)
    {
        const bool isTargetInsn =
            string(insns->Instructions(i)->mnemonic) == "call"
            && string(insns->Instructions(i + 1)->mnemonic) == "mov"
            && string(insns->Instructions(i + 2)->mnemonic) == "mov"
            && string(insns->Instructions(i + 3)->mnemonic) == "call";
        if (!isTargetInsn)
            continue;

        const auto scaleFormManagerPtrInsn = insns->Instructions(i + 2);
        const auto executeCommandInsn = insns->Instructions(i + 3);

        Global::scaleformManagerPtr = reinterpret_cast<void**>(scaleFormManagerPtrInsn->address + 7 + scaleFormManagerPtrInsn->detail->x86.disp);
        Global::executeCommandPtr = reinterpret_cast<void*>(executeCommandInsn->detail->x86.operands[0].imm);
        break;
    }

    if (!Global::scaleformManagerPtr || !Global::executeCommandPtr)
    {
        Log("Failed to find function address");
        return false;
    }

    Log("Found ExecuteCommand @ 0x{:X}", reinterpret_cast<uintptr_t>(Global::executeCommandPtr));

    return true;
}
