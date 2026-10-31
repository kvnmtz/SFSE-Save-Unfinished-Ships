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
        if (!followingRetIntInsn)
        {
            Log("Couldn't find end of function for {}", targetStringRef);
            return false;
        }

        const auto insns = dis.Disasm(stringRef, followingRetIntInsn - stringRef, reinterpret_cast<size_t>(stringRef));
        const size_t insnLimit = insns->Count > 0 ? min(insns->Count - 1, static_cast<size_t>(50)) : 0;

        bool foundAddr = false;
        for (size_t i = 0; i < insnLimit; ++i)
        {
            const auto curInsn = insns->Instructions(i);

            if (string(curInsn->mnemonic) != "mov") continue;
            if (curInsn->detail->x86.op_count != 2) continue;

            const auto movSrc = curInsn->detail->x86.operands[1];
            if (!(movSrc.type == X86_OP_IMM && movSrc.imm == 2)) continue; // "2" is the right-hand operand of the mov instruction

            const auto insnAddr = reinterpret_cast<byte*>(curInsn->address);
            const auto insnOffset = reinterpret_cast<uintptr_t>(insnAddr) - Global::moduleBase;
            const auto byteAmount = curInsn->size;
            const auto patchTargetAddr = insnAddr + byteAmount - 1;

            if (*patchTargetAddr != 0x02)
            {
                Log("Unexpected opcodes 0x{:X} (base+0x{:X}) ({})", reinterpret_cast<uintptr_t>(insnAddr), insnOffset, targetStringRef);
                return false;
            }

            Log("Found address to patch @ 0x{:X} (base+0x{:X}) ({})", reinterpret_cast<uintptr_t>(insnAddr), insnOffset, targetStringRef);

            string fromBytes = "";
            string toBytes = "";
            for (auto i = 0; i < byteAmount; ++i)
            {
                auto addr = insnAddr + i;
                fromBytes += std::format("{:02X}", *addr);
                const auto isLastByte = i == byteAmount - 1;
                toBytes += std::format("{:02X}", isLastByte ? 0x01 : *addr);
                if (!isLastByte)
                {
                    fromBytes += " ";
                    toBytes += " ";
                }
            }
            Log("-> {} ({}) will be replaced with {}", fromBytes, std::format("{} {}", curInsn->mnemonic, curInsn->op_str), toBytes);

            Global::targetAddresses.try_emplace(patchTargetAddr, std::make_pair(*patchTargetAddr, 0x01));

            foundAddr = true;
            // there can now be multiple relevant mov instructions, so no break here
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
        /* maxShipModules */

        const auto stringRef = FindStringReferenceA("$SB_ERRORBODY_DUPLICATION_EXCEEDED");
        auto previousRetIntInsn = PatternScanExactReverse({ 0xC3, 0xCC }, stringRef, 0x1000);
        if (!previousRetIntInsn)
        {
            Log("Couldn't find maxShipModules global (string reference)");
            return false;
        }

        auto insns = dis.Disasm(previousRetIntInsn, stringRef - previousRetIntInsn, reinterpret_cast<size_t>(previousRetIntInsn));

        byte* funcAddr = nullptr;
        for (size_t i = 1; i < insns->Count; ++i)
        {
            const auto insn = insns->Instructions(i);
            if (string(insn->mnemonic) == "int3") continue; // skip alignment

        	funcAddr = reinterpret_cast<byte*>(insn->address);
            const auto funcOffset = insn->address - Global::moduleBase;
            Log("Found function ($SB_ERRORBODY_DUPLICATION_EXCEEDED) @ 0x{:X} (base+0x{:X})", reinterpret_cast<uintptr_t>(funcAddr), funcOffset);
            break;
        }

        if (!funcAddr)
        {
            Log("Couldn't find maxShipModules global");
            return false;
        }

        const auto jmpRef = FindJmpReferenceToAddress(funcAddr);

        if (!jmpRef)
        {
            Log("Couldn't find maxShipModules global");
            return false;
        }

        Log("Found function ref @ 0x{:X} (base+0x{:X})", reinterpret_cast<uintptr_t>(jmpRef), reinterpret_cast<uintptr_t>(jmpRef) - Global::moduleBase);

        previousRetIntInsn = PatternScanExactReverse({ 0xC3, 0xCC }, jmpRef, 0x1000);
        if (!previousRetIntInsn)
        {
            Log("Couldn't find maxShipModules global (function start)");
            return false;
        }
        insns = dis.Disasm(previousRetIntInsn, jmpRef - previousRetIntInsn, reinterpret_cast<size_t>(previousRetIntInsn));

        bool found = false;
        for (size_t i = insns->Count; i-- > 1;)
        {
            const auto curInsn = insns->Instructions(i);

            /* target is cmp insn with dword ptr */
            const auto isTargetInsn =
                string(curInsn->mnemonic) == "cmp"
                && string(curInsn->op_str).find("rip") != string::npos;
            if (!isTargetInsn)
                continue;

            Global::maxShipModulesPtr = reinterpret_cast<int*>(curInsn->address + curInsn->size + curInsn->detail->x86.disp);

            const auto offset = reinterpret_cast<uintptr_t>(Global::maxShipModulesPtr) - Global::moduleBase;
            Log("Found maxShipModules @ 0x{:X} (base+0x{:X})", reinterpret_cast<uintptr_t>(Global::maxShipModulesPtr), offset);
            found = true;
            break;
        }

        if (!found)
        {
            Log("Couldn't find maxShipModules global");
            return false;
        }
    }

    {
        /* scaleformManagerPtr */

        const auto stringRef = FindStringReferenceA("REFR %s is at ");
        if (!stringRef)
        {
            Log("Failed to find target string reference ({})", R"("REFR %s is at ")");
            return false;
        }

        const auto previousRetIntInsn = PatternScanExactReverse({ 0xC3, 0xCC }, stringRef, 0x1000);
        if (!previousRetIntInsn)
        {
            Log("Failed to find global (scaleformManagerPtr)");
            return false;
        }
        // the base address must be the start of the disassembled range, otherwise rip-relative targets are off
        const auto insns = dis.Disasm(previousRetIntInsn, stringRef - previousRetIntInsn, reinterpret_cast<size_t>(previousRetIntInsn));

        bool found = false;
        for (size_t i = 1; i + 1 < insns->Count; ++i)
        {
            const auto insn = insns->Instructions(i);

            if (!(string(insn->mnemonic) == "mov"
                && string(insns->Instructions(i + 1)->mnemonic) == "add")) continue;

            if (string(insn->op_str).find("rip") == string::npos) continue;

            Global::scaleformManagerPtr = reinterpret_cast<void**>(insn->address + insn->size + insn->detail->x86.disp);
            const auto offset = reinterpret_cast<uintptr_t>(Global::scaleformManagerPtr) - Global::moduleBase;
            Log("Found global scaleformManagerPtr @ 0x{:X} (base+0x{:X})", reinterpret_cast<uintptr_t>(Global::scaleformManagerPtr), offset);
            found = true;
            break;
        }
        if (!found)
        {
            Log("Failed to find global (scaleformManagerPtr)");
            return false;
        }
    }

    return true;
}

bool FindFunctions()
{
    Log("Searching function addresses...");

    {
	    /* ExecuteCommand */

        const auto stringRef = FindStringReferenceA("float fresult\nref refr\nset refr to GetSelectedRef\nset fresult to ");
        if (!stringRef)
        {
            Log("Failed to find target string reference ({})", R"("float fresult\nref refr\nset refr to GetSelectedRef\nset fresult to ")");
            return false;
        }

        const auto previousRetIntInsn = PatternScanExactReverse({ 0xC3, 0xCC }, stringRef, 0x1000);
        if (!previousRetIntInsn)
        {
            Log("Failed to find function (ExecuteCommand)");
            return false;
        }
        const auto insns = dis.Disasm(previousRetIntInsn, stringRef - previousRetIntInsn, reinterpret_cast<size_t>(previousRetIntInsn));

        bool found = false;
        for (size_t i = 1; i < insns->Count; ++i)
        {
            const auto insn = insns->Instructions(i);
            if (string(insn->mnemonic) == "int3") continue;

            Global::executeCommandPtr = reinterpret_cast<void*>(insn->address);
            const auto offset = reinterpret_cast<uintptr_t>(Global::executeCommandPtr) - Global::moduleBase;
            Log("Found ExecuteCommand @ 0x{:X} (base+0x{:X})", reinterpret_cast<uintptr_t>(Global::executeCommandPtr), offset);
            found = true;
            break;
        }
        if (!found)
        {
            Log("Failed to find function (ExecuteCommand)");
            return false;
        }
    }

    return true;
}
