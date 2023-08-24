// Copyright Chad Engler

#include "he/core/hash.h"

#include "he/core/cpu.h"
#include "he/core/simd.h"

namespace he
{
    static void Sha1Block_SW(uint32_t state[5], const uint8_t* data)
    {

    }

#if HE_SIMD_SSE4_1
    static void Sha1Block_X86(uint32_t state[5], const uint8_t* data)
    {

    }
#elif HE_SIMD_NEON

#endif

    Hash<SHA1>::Hash() noexcept
    {
        Reset();
    }

    void Hash<SHA1>::Reset()
    {
        m_state[0] = 0x67452301;
        m_state[1] = 0xefcdab89;
        m_state[2] = 0x98badcfe;
        m_state[3] = 0x10325476;
        m_state[4] = 0xc3d2e1f0;
        m_curlen = 0;
        m_length = 0;
    }

    Hash<SHA1>& Hash<SHA1>::Update(const void* data, uint32_t len)
    {

    }

    Hash<SHA1>::ValueType Hash<SHA1>::Finalize()
    {

    }

    void Hash<SHA1>::Block(const uint8_t* data)
    {
    #if HE_SIMD_SSE4_1
        GetCpuInfo().x86.sha ? Sha1Block_X86(m_state, data) : Sha1Block_SW(m_state, data);
    #elif HE_SIMD_NEON
        GetCpuInfo().arm
    }
}
