// Copyright Chad Engler

#include "he/core/random.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

#include <bcrypt.h>

namespace he
{
    using Pfn_BCryptGenRandom = NTSTATUS (WINAPI*)(_In_opt_ BCRYPT_ALG_HANDLE hAlgorithm, _Out_writes_bytes_(cbBuffer) PUCHAR pbBuffer, _In_ ULONG cbBuffer, _In_ ULONG dwFlags);

    struct _SystemRandomState
    {
        _SystemRandomState()
        {
            handle = ::LoadLibraryExW(L"bcrypt.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
            BCryptGenRandom = handle ? reinterpret_cast<Pfn_BCryptGenRandom>(::GetProcAddress(handle, "BCryptGenRandom")) : nullptr;
        }

        ~_SystemRandomState()
        {
            ::FreeLibrary(handle);
        }

        HMODULE handle;
        Pfn_BCryptGenRandom BCryptGenRandom;
    };

    bool GetSystemRandomBytes(uint8_t* dst, size_t count)
    {
        static _SystemRandomState state;
        if (!state.BCryptGenRandom)
            return false;

        constexpr ULONG ChunkSize = ~ULONG(0);
        while (count > ChunkSize)
        {
            if (!BCRYPT_SUCCESS(state.BCryptGenRandom(nullptr, dst, ChunkSize, BCRYPT_USE_SYSTEM_PREFERRED_RNG)))
                return false;

            dst += ChunkSize;
            count -= ChunkSize;
        }

        if (count > 0)
            return BCRYPT_SUCCESS(state.BCryptGenRandom(nullptr, dst, ULONG(count), BCRYPT_USE_SYSTEM_PREFERRED_RNG));

        return true;
    }
}

#endif
