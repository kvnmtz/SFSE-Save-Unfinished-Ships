#include "init.h"

#include "includes.h"

#include "global.h"
#include "log.h"
#include "utils.h"

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

    for (const auto& targetStringRef : Global::patchingTargets)
    {
        const auto stringRef = FindStringReferenceA(targetStringRef);
        if (!stringRef)
        {
            Log("Couldn't find string reference to {}", targetStringRef);
            return false;
        }

        const auto followingRetIntInsn = PatternScanFromStartExact({ 0xC3, 0xCC }, stringRef, 0x10000);
        const auto insns = dis.Disasm(stringRef, followingRetIntInsn - stringRef, reinterpret_cast<size_t>(stringRef));

        std::vector<string> instructionSignature = { "mov", "lea", "call" };

        bool foundAddr = false;
        for (size_t i = 0; i < insns->Count - instructionSignature.size(); ++i)
        {
            const auto curInsn = insns->Instructions(i);

            bool found = true;
            for (size_t j = 0; j < instructionSignature.size(); ++j)
            {
                if (string(insns->Instructions(i + j)->mnemonic) == instructionSignature[j])
                    continue;

                found = false;
                break;
            }

            if (!found)
                continue;

            /* Patching mov dl, 2 (B2 02) opcodes to change errors into warnings */

            std::vector<byte> patch = { 0xB2, 0x01 }; // mov dl, 1

            const auto insn = insns->Instructions(i);
            const auto insnAddr = reinterpret_cast<byte*>(insn->address);

            if (!(*insnAddr == 0xB2 && *(insnAddr + 1) == 0x02))
            {
                Log("Unexpected opcodes 0x{:X} ({})", reinterpret_cast<uintptr_t>(insnAddr), targetStringRef);
                return false;
            }

            const auto byteAmount = patch.size();

            Log("Found address to patch at 0x{:X} ({})", reinterpret_cast<uintptr_t>(insnAddr), targetStringRef);

            string fromBytes = "";
            string toBytes = "";
            for (auto i = 0; i < byteAmount; ++i)
            {
                auto addr = insnAddr + i;
                fromBytes += std::format("{:02X}", *addr);
                toBytes += std::format("{:02X}", patch.at(i));
                const auto isLastByte = i == byteAmount - 1;
                if (!isLastByte)
                {
                    fromBytes += " ";
                    toBytes += " ";
                }
            }
            Log("-> {} will be replaced with {}", fromBytes, toBytes);

            for (auto i = 0; i < byteAmount; ++i)
            {
                auto addr = insnAddr + i;
                Global::targetAddresses.try_emplace(addr, std::make_pair(*addr, patch.at(i)));
            }

            foundAddr = true;
            break;
        }

        if (!foundAddr)
        {
            Log("Couldn't find address for {}", targetStringRef);
            return false;
        }
    }

    return true;
}

bool FindGlobals()
{
    Log("Searching globals...");

    {
        const auto stringRef = FindStringReferenceA("$SB_ERRORBODY_DUPLICATION_EXCEEDED");
        const auto previousRetIntInsn = PatternScanExactReverse({ 0xC3, 0xCC }, stringRef, 0x1000);

        const auto insns = dis.Disasm(previousRetIntInsn, stringRef - previousRetIntInsn, reinterpret_cast<size_t>(previousRetIntInsn));

        for (size_t i = insns->Count - 1; i > 0; --i)
        {
            const auto curInsn = insns->Instructions(i);

            /* target is cmp insn with dword ptr */
            const auto isTargetInsn =
                string(curInsn->mnemonic) == "cmp"
                && string(curInsn->op_str).find("rip") != string::npos;
            if (!isTargetInsn)
                continue;

            Global::maxShipModulesPtr = reinterpret_cast<int*>(curInsn->address + curInsn->size + curInsn->detail->x86.disp);

            Log("Found maxShipModules @ 0x{:X}", reinterpret_cast<uintptr_t>(Global::maxShipModulesPtr));
            return true;
        }

        Log("Couldn't find maxShipModules global");
    }

    return false;
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

    const auto followingIntInsn = PatternScanFromStartExact({ 0xC3, 0xCC }, stringRef, 0x1000);
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
