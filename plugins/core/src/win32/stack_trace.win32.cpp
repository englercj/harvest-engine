// Copyright Chad Engler

#include "he/core/stack_trace.h"

#include "he/core/cpu.h"
#include "he/core/path.h"
#include "he/core/string.h"
#include "he/core/sync.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

#include <dbghelp.h>

namespace he
{
    // --------------------------------------------------------------------------------------------
    using Pfn_SymInitialize = BOOL(WINAPI*)(_In_ HANDLE hProcess, _In_opt_ PCSTR UserSearchPath, _In_ BOOL fInvadeProcess);
    using Pfn_SymCleanup = BOOL(WINAPI*)(_In_ HANDLE hProcess);
    using Pfn_SymFromAddr = BOOL(WINAPI*)(_In_ HANDLE hProcess, _In_ DWORD64 Address, _Out_opt_ PDWORD64 Displacement, _Inout_ PSYMBOL_INFO Symbol);
    using Pfn_SymGetLineFromAddr64 = BOOL(WINAPI*)(_In_ HANDLE hProcess, _In_ DWORD64 qwAddr, _Out_ PDWORD pdwDisplacement, _Out_ PIMAGEHLP_LINE64 Line64);
    using Pfn_UnDecorateSymbolName = DWORD(WINAPI*)(_In_ PCSTR name, _Out_writes_(maxStringLength) PSTR outputString, _In_ DWORD maxStringLength, _In_ DWORD flags);

    // --------------------------------------------------------------------------------------------
    class SymLoader
    {
    public:
        SymLoader()
        {
            m_lib = ::LoadLibraryW(L"dbghelp.dll");
            if (m_lib)
            {
                m_SymInitialize = reinterpret_cast<Pfn_SymInitialize>(::GetProcAddress(m_lib, "SymInitialize"));
                m_SymCleanup = reinterpret_cast<Pfn_SymCleanup>(::GetProcAddress(m_lib, "SymCleanup"));
                m_SymFromAddr = reinterpret_cast<Pfn_SymFromAddr>(::GetProcAddress(m_lib, "SymFromAddr"));
                m_SymGetLineFromAddr64 = reinterpret_cast<Pfn_SymGetLineFromAddr64>(::GetProcAddress(m_lib, "SymGetLineFromAddr64"));
                m_UnDecorateSymbolName = reinterpret_cast<Pfn_UnDecorateSymbolName>(::GetProcAddress(m_lib, "UnDecorateSymbolName"));

                if (m_SymInitialize && m_SymCleanup && m_SymFromAddr && m_SymGetLineFromAddr64 && m_UnDecorateSymbolName)
                {
                    HANDLE proc = ::GetCurrentProcess();
                    m_initialized = m_SymInitialize(proc, nullptr, TRUE);
                }
            }
        }

        ~SymLoader()
        {
            if (m_initialized)
            {
                HANDLE proc = ::GetCurrentProcess();
                m_SymCleanup(proc);
            }

            if (m_lib)
            {
                ::FreeLibrary(m_lib);
            }
        }

        Mutex m_mutex{};
        BOOL m_initialized{ FALSE };

        HMODULE m_lib{ nullptr };
        Pfn_SymInitialize m_SymInitialize{ nullptr };
        Pfn_SymCleanup m_SymCleanup{ nullptr };
        Pfn_SymFromAddr m_SymFromAddr{ nullptr };
        Pfn_SymGetLineFromAddr64 m_SymGetLineFromAddr64{ nullptr };
        Pfn_UnDecorateSymbolName m_UnDecorateSymbolName{ nullptr };
    };

    // --------------------------------------------------------------------------------------------
    Result CaptureStackTrace(uintptr_t* frames, uint32_t& count, uint32_t skipCount)
    {
        static_assert(sizeof(void*) == sizeof(uintptr_t));
        ++skipCount; // always skip CaptureStackTrace in the frame list
        count = ::RtlCaptureStackBackTrace(skipCount, count, reinterpret_cast<void**>(frames), nullptr);
        return Result::Success;
    }

    // --------------------------------------------------------------------------------------------
    Result GetSymbolInfo(uintptr_t frame, SymbolInfo& info)
    {
        static SymLoader s_sym{};
        if (!s_sym.m_initialized)
            return Result::NotSupported;

        HANDLE proc = ::GetCurrentProcess();

        // DbgHelp functions are not thread safe, so we need to lock around them.
        LockGuard lock(s_sym.m_mutex);

        // Load the symbol information for this frame address
        // Win32 expects a buffer that can hold the SYMBOL_INFO structure and the symbol name
        char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
        MemZero(symbolBuffer, sizeof(symbolBuffer));
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;
        if (!s_sym.m_SymFromAddr(proc, frame, nullptr, symbol))
            return Result::FromLastError();

        // Try to demangle the function's name
        info.name.Resize(MAX_SYM_NAME);
        const DWORD len = s_sym.m_UnDecorateSymbolName(symbol->Name, info.name.Data(), info.name.Size(), UNDNAME_COMPLETE);
        if (len > 0)
            info.name.Resize(len);
        else
            info.name = symbol->Name;

        // Try to load the source file & line information for the address
        IMAGEHLP_LINE64 line{};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD offset = 0;
        if (s_sym.m_SymGetLineFromAddr64(proc, frame, &offset, &line))
        {
            info.file = line.FileName;
            info.line = line.LineNumber;
            info.column = offset;
        }
        else
        {
            info.file.Clear();
            info.line = 0;
            info.column = 0;
        }
        return Result::Success;
    }
}

#endif
