// Copyright Chad Engler

#include "he/core/stack_trace.h"

#if defined(HE_PLATFORM_LINUX)

// We only care about unwinding local process stacks
#define UNW_LOCAL_ONLY

#include <dwarf.h>
#include <errno.h>
#include <execinfo.h>
#include <libunwind.h>
#include <unistd.h>
#include <elfutils/libdw.h>
#include <elfutils/libdwfl.h>

namespace he
{
    // --------------------------------------------------------------------------------------------
    class DwarfInitializer
    {
    public:
        DwarfInitializer() noexcept
        {
            m_callbacks.find_elf = &dwfl_linux_proc_find_elf;
            m_callbacks.find_debuginfo = &dwfl_standard_find_debuginfo;
            m_callbacks.debuginfo_path = 0;

            m_session = dwfl_begin(&m_callbacks);

            if (m_session)
            {
                dwfl_report_begin(m_session);
                const int rc = dwfl_linux_proc_report(m_session, getpid());
                dwfl_report_end(m_session, nullptr, nullptr);

                if (rc != 0)
                {
                    dwfl_end(m_session);
                    m_session = nullptr;
                }
            }
        }

        ~DwarfInitializer() noexcept
        {
            if (m_session)
            {
                dwfl_end(m_session);
            }
        }

        Dwfl* m_session{ nullptr };
        Dwfl_Callbacks m_callbacks{};
    };

    // --------------------------------------------------------------------------------------------
    static bool IsPcInDie(Dwarf_Die* die, Dwarf_Addr pc)
    {
        Dwarf_Addr low;
        Dwarf_Addr high;

        // continuous range
        if (dwarf_hasattr(die, DW_AT_low_pc) && dwarf_hasattr(die, DW_AT_high_pc))
        {
            if (dwarf_lowpc(die, &low) != 0)
                return false;

            if (dwarf_highpc(die, &high) != 0)
            {
                Dwarf_Attribute attrMem;
                Dwarf_Attribute* attr = dwarf_attr(die, DW_AT_high_pc, &attrMem);

                Dwarf_Word value;
                if (dwarf_formudata(attr, &value) != 0)
                    return false;

                high = low + value;
            }

            return pc >= low && pc < high;
        }

        // non-continuous range.
        Dwarf_Addr base;
        ptrdiff_t offset = 0;
        while ((offset = dwarf_ranges(die, offset, &base, &low, &high)) > 0)
        {
            if (pc >= low && pc < high)
                return true;
        }

        return false;
    }

    static bool FindFuncDieByPc(Dwarf_Die* parent, Dwarf_Addr pc, Dwarf_Die* result)
    {
        if (dwarf_child(parent, result) != 0)
            return false;

        Dwarf_Die* die = result;
        do
        {
            const int tag = dwarf_tag(die);
            if (tag == DW_TAG_subprogram || tag == DW_TAG_inlined_subroutine)
            {
                if (IsPcInDie(die, pc))
                    return true;
            }

            bool declaration = false;
            Dwarf_Attribute attrMem;
            dwarf_formflag(dwarf_attr(die, DW_AT_declaration, &attrMem), &declaration);
            if (!declaration)
            {
                // Recurse here as the function may be nested in a namespace or similar construct
                Dwarf_Die inDie;
                if (FindFuncDieByPc(die, pc, &inDie))
                {
                    *result = inDie;
                    return true;
                }
            }
        } while (dwarf_siblingof(die, result) == 0);

        return false;
    }

    template <typename F>
    static bool ForEachDieByPc(Dwarf_Die* parent, Dwarf_Addr pc, F&& iterator)
    {
        Dwarf_Die node;
        if (dwarf_child(parent, &node) != 0)
            return false;

        bool pcInBranch = false;
        Dwarf_Die* die = &node;
        do
        {
            bool declaration = false;
            Dwarf_Attribute attrMem;
            dwarf_formflag(dwarf_attr(die, DW_AT_declaration, &attrMem), &declaration);
            if (!declaration)
                pcInBranch = ForEachDieByPc(die, pc, iterator);

            if (!pcInBranch)
                pcInBranch = IsPcInDie(die, pc);

            if (pcInBranch)
                iterator(die);

        } while (dwarf_siblingof(die, &node) == 0);

        return pcInBranch;
    }

    // --------------------------------------------------------------------------------------------
    Result CaptureStackTrace(uintptr_t* frames, uint32_t& count, uint32_t skipCount)
    {
        unw_context_t ctx;
        int rc = unw_getcontext(&ctx);
        if (rc != 0)
            return Result::NotSupported;

        unw_cursor_t cursor;
        rc = unw_init_local(&cursor, &ctx);
        if (rc != 0)
            return Result::NotSupported;

        ++skipCount; // always skip CaptureStackTrace in the frame list

        size_t index = 0;
        while (index < count && unw_step(&cursor) > 0)
        {
            unw_word_t ip = 0;
            rc = unw_get_reg(&cursor, UNW_REG_IP, &ip);

            if (rc != 0)
                break;

            if (skipCount > 0)
            {
                --skipCount;
                continue;
            }

            frames[index++] = --ip;
        }

        count = index;
        return Result::Success;
    }

    // --------------------------------------------------------------------------------------------
    Result GetSymbolInfo(uintptr_t frame, SymbolInfo& info)
    {
        static DwarfInitializer s_dwarfInit{};
        if (!s_dwarfInit.m_session)
            return Result::NotSupported;

        Dwarf_Addr addr = reinterpret_cast<Dwarf_Addr>(frame);
        const char* mangledSymbolName = nullptr;

        // Find the module that contains our address
        Dwfl_Module* mod = dwfl_addrmodule(s_dwarfInit.m_session, addr);
        if (!mod)
            return Result::InvalidParameter;

        // Get the mangled name of the symbol of our address
        const char* name = dwfl_module_addrname(mod, addr);
        if (name)
            mangledSymbolName = name;

        // Get the DIE for the compilation unit containing our address
        Dwarf_Addr modBias = 0;
        Dwarf_Die* cuDie = dwfl_module_addrdie(mod, addr, &modBias);

        // Clang doesn't generate the .debug_aranges section used by dwfl_module_addrdie
        // so instead we have to manually iterate over all the CUs and find our symbol.
        if (!cuDie)
        {
            while (cuDie = dwfl_module_nextcu(mod, cuDie, &modBias))
            {
                Dwarf_Die funcDie;
                if (FindFuncDieByPc(cuDie, addr - modBias, &funcDie))
                    break;
            }
        }

        // If we still don't have it, we couldn't find this symbol. Tell the user it is invalid.
        if (!cuDie)
            return Result::InvalidParameter;

        // Load the source information so we can get the file, line, and column data
        Dwarf_Line* srcLine = dwarf_getsrc_die(cuDie, addr - modBias);
        if (srcLine)
        {
            const char* file = dwarf_linesrc(srcLine, nullptr, nullptr);
            if (file)
                info.file = file;

            int line = 0;
            int col = 0;
            dwarf_lineno(srcLine, &line);
            dwarf_linecol(srcLine, &col);
            info.line = static_cast<uint32_t>(line);
            info.column = static_cast<uint32_t>(col);
        }

        // Search for the PC in the DIEs and assign our function name.
        ForEachDieByPc(cuDie, addr - modBias, [&](Dwarf_Die* die)
        {
            const int tag = dwarf_tag(die);
            if (tag == DW_TAG_subprogram)
            {
                const char* name = dwarf_diename(die);
                if (name)
                    info.name = name;
            }
        });

        // Fall back to the mangled symbol name if we didn't find a function name in the DIEs.
        if (info.function.IsEmpty() && mangledSymbolName)
        {
            char* buf = nullptr;
            size_t len = 0;
            char* result = abi::__cxa_demangle(mangledSymbolName, buf, &len, nullptr);

            if (result)
            {
                info.name = result;
                free(result);
            }
            else
            {
                info.name = mangledSymbolName;
            }
        }

        return Result::Success;
    }
}

#endif
