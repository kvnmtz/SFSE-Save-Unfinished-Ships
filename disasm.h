#pragma once

/* Minimal RAII wrapper over the Capstone C API (replaces the old cccapstone dependency) */

#include <capstone/capstone.h>

#include <cstddef>
#include <cstdint>
#include <memory>

class DisasmResult
{
public:
    DisasmResult(cs_insn* insns, size_t count) : m_insns(insns), Count(count) {}
    ~DisasmResult() { if (m_insns) cs_free(m_insns, Count); }

    DisasmResult(const DisasmResult&) = delete;
    DisasmResult& operator=(const DisasmResult&) = delete;

    const cs_insn* Instructions(size_t index) const { return index < Count ? &m_insns[index] : nullptr; }

private:
    cs_insn* m_insns;

public:
    const size_t Count;
};

class CX86Disasm64
{
public:
    CX86Disasm64() { m_err = cs_open(CS_ARCH_X86, CS_MODE_64, &m_handle); }
    ~CX86Disasm64() { if (m_err == CS_ERR_OK) cs_close(&m_handle); }

    CX86Disasm64(const CX86Disasm64&) = delete;
    CX86Disasm64& operator=(const CX86Disasm64&) = delete;

    bool GetError() const { return m_err != CS_ERR_OK; }
    void SetDetail(cs_opt_value value) { cs_option(m_handle, CS_OPT_DETAIL, value); }
    void SetSyntax(cs_opt_value value) { cs_option(m_handle, CS_OPT_SYNTAX, value); }

    /* Returns an empty result (Count == 0) on bad input instead of crashing */
    std::unique_ptr<DisasmResult> Disasm(const void* code, ptrdiff_t size, uint64_t address)
    {
        cs_insn* insns = nullptr;
        size_t count = 0;
        if (code && size > 0)
            count = cs_disasm(m_handle, static_cast<const uint8_t*>(code), static_cast<size_t>(size), address, 0, &insns);
        return std::make_unique<DisasmResult>(insns, count);
    }

private:
    csh m_handle = 0;
    cs_err m_err = CS_ERR_HANDLE;
};
