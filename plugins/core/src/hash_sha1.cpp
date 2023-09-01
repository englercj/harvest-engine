// Copyright Chad Engler

#include "he/core/hash.h"

#include "he/core/assert.h"
#include "he/core/cpu.h"
#include "he/core/cpu_info.h"
#include "he/core/memory_ops.h"
#include "he/core/simd.h"

#if !defined(HE_ENABLE_HARDWARE_SHA1)
    #define HE_ENABLE_HARDWARE_SHA1     (HE_CPU_X86 || HE_CPU_ARM_64)
#endif

namespace he
{
    //-------------------------------------------------------------------------------------------------
    #define HE_SHA1_ROTL32(x, r)    ((x << r) | (x >> (32 - r)))

    #define HE_SHA1_F0(x, y, z)     (z ^ (x & (y ^ z)))
    #define HE_SHA1_F1(x, y, z)     (x ^ y ^ z)
    #define HE_SHA1_F2(x, y, z)     ((x & y) | (z & (x | y)))
    #define HE_SHA1_F3(x, y, z)     (x ^ y ^ z)

    #define HE_SHA1_FF0(a, b, c, d, e, i) e = (HE_SHA1_ROTL32(a, 5) + HE_SHA1_F0(b, c, d) + e + W[i] + 0x5a827999UL); b = HE_SHA1_ROTL32(b, 30);
    #define HE_SHA1_FF1(a, b, c, d, e, i) e = (HE_SHA1_ROTL32(a, 5) + HE_SHA1_F1(b, c, d) + e + W[i] + 0x6ed9eba1UL); b = HE_SHA1_ROTL32(b, 30);
    #define HE_SHA1_FF2(a, b, c, d, e, i) e = (HE_SHA1_ROTL32(a, 5) + HE_SHA1_F2(b, c, d) + e + W[i] + 0x8f1bbcdcUL); b = HE_SHA1_ROTL32(b, 30);
    #define HE_SHA1_FF3(a, b, c, d, e, i) e = (HE_SHA1_ROTL32(a, 5) + HE_SHA1_F3(b, c, d) + e + W[i] + 0xca62c1d6UL); b = HE_SHA1_ROTL32(b, 30);

    [[maybe_unused]] HE_FORCE_INLINE void Sha1Block_SW(uint32_t state[5], const uint8_t* data)
    {
        uint32_t W[80];
        for (uint32_t i = 0; i < 16; ++i)
        {
            W[i] = LoadBE(*reinterpret_cast<const uint32_t*>(data + (4 * i)));
        }

        uint32_t a = state[0];
        uint32_t b = state[1];
        uint32_t c = state[2];
        uint32_t d = state[3];
        uint32_t e = state[4];

        for (uint32_t i = 16; i < 80; ++i)
        {
            W[i] = HE_SHA1_ROTL32(W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16], 1);
        }

        uint32_t i = 0;
        uint32_t t = 0;
        for (; i < 20; ++i)
        {
            HE_SHA1_FF0(a, b, c, d, e, i); t = e; e = d; d = c; c = b; b = a; a = t;
        }

        for (; i < 40; ++i)
        {
            HE_SHA1_FF1(a, b, c, d, e, i); t = e; e = d; d = c; c = b; b = a; a = t;
        }

        for (; i < 60; ++i)
        {
            HE_SHA1_FF2(a, b, c, d, e, i); t = e; e = d; d = c; c = b; b = a; a = t;
        }

        for (; i < 80; ++i)
        {
            HE_SHA1_FF3(a, b, c, d, e, i); t = e; e = d; d = c; c = b; b = a; a = t;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
    }

#if HE_ENABLE_HARDWARE_SHA1
    //-------------------------------------------------------------------------------------------------
#if HE_CPU_X86
    HE_FORCE_INLINE void Sha1Block_SSE41(__m128i& ABCD, __m128i& E0, const uint8_t* data)
    {
        const __m128i MASK = _mm_set_epi64x(0x0001020304050607ULL, 0x08090a0b0c0d0e0fULL);
        __m128i ABCD_SAVE = ABCD;
        __m128i E0_SAVE = E0;

        __m128i MSG0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data));
        __m128i MSG1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 16));
        __m128i MSG2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 32));
        __m128i MSG3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 48));

        __m128i E1;

        // Rounds 0-3
        MSG0 = _mm_shuffle_epi8(MSG0, MASK);
        E0 = _mm_add_epi32(E0, MSG0);
        E1 = ABCD;
        ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 0);

        // Rounds 4-7
        MSG1 = _mm_shuffle_epi8(MSG1, MASK);
        E1 = _mm_sha1nexte_epu32(E1, MSG1);
        E0 = ABCD;
        ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 0);
        MSG0 = _mm_sha1msg1_epu32(MSG0, MSG1);

        // Rounds 8-11
        MSG2 = _mm_shuffle_epi8(MSG2, MASK);
        E0 = _mm_sha1nexte_epu32(E0, MSG2);
        E1 = ABCD;
        ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 0);
        MSG1 = _mm_sha1msg1_epu32(MSG1, MSG2);
        MSG0 = _mm_xor_si128(MSG0, MSG2);

        // Rounds 12-15
        MSG3 = _mm_shuffle_epi8(MSG3, MASK);
        E1 = _mm_sha1nexte_epu32(E1, MSG3);
        E0 = ABCD;
        MSG0 = _mm_sha1msg2_epu32(MSG0, MSG3);
        ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 0);
        MSG2 = _mm_sha1msg1_epu32(MSG2, MSG3);
        MSG1 = _mm_xor_si128(MSG1, MSG3);

        // Rounds 16-19
        E0 = _mm_sha1nexte_epu32(E0, MSG0);
        E1 = ABCD;
        MSG1 = _mm_sha1msg2_epu32(MSG1, MSG0);
        ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 0);
        MSG3 = _mm_sha1msg1_epu32(MSG3, MSG0);
        MSG2 = _mm_xor_si128(MSG2, MSG0);

        // Rounds 20-23
        E1 = _mm_sha1nexte_epu32(E1, MSG1);
        E0 = ABCD;
        MSG2 = _mm_sha1msg2_epu32(MSG2, MSG1);
        ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 1);
        MSG0 = _mm_sha1msg1_epu32(MSG0, MSG1);
        MSG3 = _mm_xor_si128(MSG3, MSG1);

        // Rounds 24-27
        E0 = _mm_sha1nexte_epu32(E0, MSG2);
        E1 = ABCD;
        MSG3 = _mm_sha1msg2_epu32(MSG3, MSG2);
        ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 1);
        MSG1 = _mm_sha1msg1_epu32(MSG1, MSG2);
        MSG0 = _mm_xor_si128(MSG0, MSG2);

        // Rounds 28-31
        E1 = _mm_sha1nexte_epu32(E1, MSG3);
        E0 = ABCD;
        MSG0 = _mm_sha1msg2_epu32(MSG0, MSG3);
        ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 1);
        MSG2 = _mm_sha1msg1_epu32(MSG2, MSG3);
        MSG1 = _mm_xor_si128(MSG1, MSG3);

        // Rounds 32-35
        E0 = _mm_sha1nexte_epu32(E0, MSG0);
        E1 = ABCD;
        MSG1 = _mm_sha1msg2_epu32(MSG1, MSG0);
        ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 1);
        MSG3 = _mm_sha1msg1_epu32(MSG3, MSG0);
        MSG2 = _mm_xor_si128(MSG2, MSG0);

        // Rounds 36-39
        E1 = _mm_sha1nexte_epu32(E1, MSG1);
        E0 = ABCD;
        MSG2 = _mm_sha1msg2_epu32(MSG2, MSG1);
        ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 1);
        MSG0 = _mm_sha1msg1_epu32(MSG0, MSG1);
        MSG3 = _mm_xor_si128(MSG3, MSG1);

        // Rounds 40-43
        E0 = _mm_sha1nexte_epu32(E0, MSG2);
        E1 = ABCD;
        MSG3 = _mm_sha1msg2_epu32(MSG3, MSG2);
        ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 2);
        MSG1 = _mm_sha1msg1_epu32(MSG1, MSG2);
        MSG0 = _mm_xor_si128(MSG0, MSG2);

        // Rounds 44-47
        E1 = _mm_sha1nexte_epu32(E1, MSG3);
        E0 = ABCD;
        MSG0 = _mm_sha1msg2_epu32(MSG0, MSG3);
        ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 2);
        MSG2 = _mm_sha1msg1_epu32(MSG2, MSG3);
        MSG1 = _mm_xor_si128(MSG1, MSG3);

        // Rounds 48-51
        E0 = _mm_sha1nexte_epu32(E0, MSG0);
        E1 = ABCD;
        MSG1 = _mm_sha1msg2_epu32(MSG1, MSG0);
        ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 2);
        MSG3 = _mm_sha1msg1_epu32(MSG3, MSG0);
        MSG2 = _mm_xor_si128(MSG2, MSG0);

        // Rounds 52-55
        E1 = _mm_sha1nexte_epu32(E1, MSG1);
        E0 = ABCD;
        MSG2 = _mm_sha1msg2_epu32(MSG2, MSG1);
        ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 2);
        MSG0 = _mm_sha1msg1_epu32(MSG0, MSG1);
        MSG3 = _mm_xor_si128(MSG3, MSG1);

        // Rounds 56-59
        E0 = _mm_sha1nexte_epu32(E0, MSG2);
        E1 = ABCD;
        MSG3 = _mm_sha1msg2_epu32(MSG3, MSG2);
        ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 2);
        MSG1 = _mm_sha1msg1_epu32(MSG1, MSG2);
        MSG0 = _mm_xor_si128(MSG0, MSG2);

        // Rounds 60-63
        E1 = _mm_sha1nexte_epu32(E1, MSG3);
        E0 = ABCD;
        MSG0 = _mm_sha1msg2_epu32(MSG0, MSG3);
        ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 3);
        MSG2 = _mm_sha1msg1_epu32(MSG2, MSG3);
        MSG1 = _mm_xor_si128(MSG1, MSG3);

        // Rounds 64-67
        E0 = _mm_sha1nexte_epu32(E0, MSG0);
        E1 = ABCD;
        MSG1 = _mm_sha1msg2_epu32(MSG1, MSG0);
        ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 3);
        MSG3 = _mm_sha1msg1_epu32(MSG3, MSG0);
        MSG2 = _mm_xor_si128(MSG2, MSG0);

        // Rounds 68-71
        E1 = _mm_sha1nexte_epu32(E1, MSG1);
        E0 = ABCD;
        MSG2 = _mm_sha1msg2_epu32(MSG2, MSG1);
        ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 3);
        MSG3 = _mm_xor_si128(MSG3, MSG1);

        // Rounds 72-75
        E0 = _mm_sha1nexte_epu32(E0, MSG2);
        E1 = ABCD;
        MSG3 = _mm_sha1msg2_epu32(MSG3, MSG2);
        ABCD = _mm_sha1rnds4_epu32(ABCD, E0, 3);

        // Rounds 76-79
        E1 = _mm_sha1nexte_epu32(E1, MSG3);
        E0 = ABCD;
        ABCD = _mm_sha1rnds4_epu32(ABCD, E1, 3);

        // Combine state
        E0 = _mm_sha1nexte_epu32(E0, E0_SAVE);
        ABCD = _mm_add_epi32(ABCD, ABCD_SAVE);
    }

    //-------------------------------------------------------------------------------------------------
#elif HE_CPU_ARM_64
    HE_FORCE_INLINE void Sha1Block_NEON(uint32x4_t& ABCD, uint32_t& E0, const uint8_t* data)
    {
        uint32x4_t ABCD_SAVED = ABCD;
        uint32_t E0_SAVED = E0;

        uint32x4_t MSG0 = vld1q_u32(reinterpret_cast<const uint32_t*>(data));
        uint32x4_t MSG1 = vld1q_u32(reinterpret_cast<const uint32_t*>(data + 16));
        uint32x4_t MSG2 = vld1q_u32(reinterpret_cast<const uint32_t*>(data + 32));
        uint32x4_t MSG3 = vld1q_u32(reinterpret_cast<const uint32_t*>(data + 48));

        uint32x4_t TMP0;
        uint32x4_t TMP1;

        // Reverse for little endian
    #if HE_CPU_LITTLE_ENDIAN
        MSG0 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG0)));
        MSG1 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG1)));
        MSG2 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG2)));
        MSG3 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG3)));

        TMP0 = vaddq_u32(MSG0, vdupq_n_u32(0x5A827999));
        TMP1 = vaddq_u32(MSG1, vdupq_n_u32(0x5A827999));
    #endif

        // Rounds 0-3
        E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1cq_u32(ABCD, E0, TMP0);
        TMP0 = vaddq_u32(MSG2, vdupq_n_u32(0x5A827999));
        MSG0 = vsha1su0q_u32(MSG0, MSG1, MSG2);

        // Rounds 4-7
        E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1cq_u32(ABCD, E1, TMP1);
        TMP1 = vaddq_u32(MSG3, vdupq_n_u32(0x5A827999));
        MSG0 = vsha1su1q_u32(MSG0, MSG3);
        MSG1 = vsha1su0q_u32(MSG1, MSG2, MSG3);

        // Rounds 8-11
        E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1cq_u32(ABCD, E0, TMP0);
        TMP0 = vaddq_u32(MSG0, vdupq_n_u32(0x5A827999));
        MSG1 = vsha1su1q_u32(MSG1, MSG0);
        MSG2 = vsha1su0q_u32(MSG2, MSG3, MSG0);

        // Rounds 12-15
        E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1cq_u32(ABCD, E1, TMP1);
        TMP1 = vaddq_u32(MSG1, vdupq_n_u32(0x6ED9EBA1));
        MSG2 = vsha1su1q_u32(MSG2, MSG1);
        MSG3 = vsha1su0q_u32(MSG3, MSG0, MSG1);

        // Rounds 16-19
        E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1cq_u32(ABCD, E0, TMP0);
        TMP0 = vaddq_u32(MSG2, vdupq_n_u32(0x6ED9EBA1));
        MSG3 = vsha1su1q_u32(MSG3, MSG2);
        MSG0 = vsha1su0q_u32(MSG0, MSG1, MSG2);

        // Rounds 20-23
        E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1pq_u32(ABCD, E1, TMP1);
        TMP1 = vaddq_u32(MSG3, vdupq_n_u32(0x6ED9EBA1));
        MSG0 = vsha1su1q_u32(MSG0, MSG3);
        MSG1 = vsha1su0q_u32(MSG1, MSG2, MSG3);

        // Rounds 24-27
        E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1pq_u32(ABCD, E0, TMP0);
        TMP0 = vaddq_u32(MSG0, vdupq_n_u32(0x6ED9EBA1));
        MSG1 = vsha1su1q_u32(MSG1, MSG0);
        MSG2 = vsha1su0q_u32(MSG2, MSG3, MSG0);

        // Rounds 28-31
        E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1pq_u32(ABCD, E1, TMP1);
        TMP1 = vaddq_u32(MSG1, vdupq_n_u32(0x6ED9EBA1));
        MSG2 = vsha1su1q_u32(MSG2, MSG1);
        MSG3 = vsha1su0q_u32(MSG3, MSG0, MSG1);

        // Rounds 32-35
        E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1pq_u32(ABCD, E0, TMP0);
        TMP0 = vaddq_u32(MSG2, vdupq_n_u32(0x8F1BBCDC));
        MSG3 = vsha1su1q_u32(MSG3, MSG2);
        MSG0 = vsha1su0q_u32(MSG0, MSG1, MSG2);

        // Rounds 36-39
        E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1pq_u32(ABCD, E1, TMP1);
        TMP1 = vaddq_u32(MSG3, vdupq_n_u32(0x8F1BBCDC));
        MSG0 = vsha1su1q_u32(MSG0, MSG3);
        MSG1 = vsha1su0q_u32(MSG1, MSG2, MSG3);

        // Rounds 40-43
        E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1mq_u32(ABCD, E0, TMP0);
        TMP0 = vaddq_u32(MSG0, vdupq_n_u32(0x8F1BBCDC));
        MSG1 = vsha1su1q_u32(MSG1, MSG0);
        MSG2 = vsha1su0q_u32(MSG2, MSG3, MSG0);

        // Rounds 44-47
        E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1mq_u32(ABCD, E1, TMP1);
        TMP1 = vaddq_u32(MSG1, vdupq_n_u32(0x8F1BBCDC));
        MSG2 = vsha1su1q_u32(MSG2, MSG1);
        MSG3 = vsha1su0q_u32(MSG3, MSG0, MSG1);

        // Rounds 48-51
        E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1mq_u32(ABCD, E0, TMP0);
        TMP0 = vaddq_u32(MSG2, vdupq_n_u32(0x8F1BBCDC));
        MSG3 = vsha1su1q_u32(MSG3, MSG2);
        MSG0 = vsha1su0q_u32(MSG0, MSG1, MSG2);

        // Rounds 52-55
        E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1mq_u32(ABCD, E1, TMP1);
        TMP1 = vaddq_u32(MSG3, vdupq_n_u32(0xCA62C1D6));
        MSG0 = vsha1su1q_u32(MSG0, MSG3);
        MSG1 = vsha1su0q_u32(MSG1, MSG2, MSG3);

        // Rounds 56-59
        E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1mq_u32(ABCD, E0, TMP0);
        TMP0 = vaddq_u32(MSG0, vdupq_n_u32(0xCA62C1D6));
        MSG1 = vsha1su1q_u32(MSG1, MSG0);
        MSG2 = vsha1su0q_u32(MSG2, MSG3, MSG0);

        // Rounds 60-63
        E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1pq_u32(ABCD, E1, TMP1);
        TMP1 = vaddq_u32(MSG1, vdupq_n_u32(0xCA62C1D6));
        MSG2 = vsha1su1q_u32(MSG2, MSG1);
        MSG3 = vsha1su0q_u32(MSG3, MSG0, MSG1);

        // Rounds 64-67
        E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1pq_u32(ABCD, E0, TMP0);
        TMP0 = vaddq_u32(MSG2, vdupq_n_u32(0xCA62C1D6));
        MSG3 = vsha1su1q_u32(MSG3, MSG2);
        MSG0 = vsha1su0q_u32(MSG0, MSG1, MSG2);

        // Rounds 68-71
        E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1pq_u32(ABCD, E1, TMP1);
        TMP1 = vaddq_u32(MSG3, vdupq_n_u32(0xCA62C1D6));
        MSG0 = vsha1su1q_u32(MSG0, MSG3);

        // Rounds 72-75
        E1 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1pq_u32(ABCD, E0, TMP0);

        // Rounds 76-79
        E0 = vsha1h_u32(vgetq_lane_u32(ABCD, 0));
        ABCD = vsha1pq_u32(ABCD, E1, TMP1);

        // Combine state
        E0 += E0_SAVED;
        ABCD = vaddq_u32(ABCD_SAVED, ABCD);
    }
#else
    #error "Hardware SHA1 not available for this architecture"
#endif
#endif

    //-------------------------------------------------------------------------------------------------
    SHA1::Value SHA1::Mem(const void* data, uint32_t len, uint64_t)
    {
        Hash<SHA1> hash;
        hash.Update(data, len);
        return hash.Finalize();
    }

    Hash<SHA1>::Hash() noexcept
    {
        Reset();
    }

    void Hash<SHA1>::Reset()
    {
        m_length = 0;
        m_state[0] = 0x67452301;
        m_state[1] = 0xefcdab89;
        m_state[2] = 0x98badcfe;
        m_state[3] = 0x10325476;
        m_state[4] = 0xc3d2e1f0;

        m_bufLen = 0;
        MemZero(m_buf, sizeof(m_buf));
    }

    Hash<SHA1>& Hash<SHA1>::Update(const void* data, uint32_t len)
    {
        if (len == 0)
            return *this;

        const uint8_t* bytes = static_cast<const uint8_t*>(data);

    #if !HE_ENABLE_HARDWARE_SHA1
        Process_SW(bytes, len);
    #elif HE_CPU_X86
        if (GetCpuInfo().x86.sha)
            Process_SSE41(bytes, len);
        else
            Process_SW(bytes, len);
    #elif HE_CPU_ARM_64
        if (GetCpuInfo().arm.sha1)
            Process_NEON(bytes, len);
        else
            Process_SW(bytes, len);
    #else
        #error "Unknown CRC32C implementation for this configuration"
    #endif

        return *this;
    }

    void Hash<SHA1>::Process_SW(const uint8_t* data, uint32_t len)
    {
        while (len > 0)
        {
            if (m_bufLen == 0 && len >= BlockSize)
            {
                Sha1Block_SW(m_state, data);
                m_length += BlockSize * 8;
                data += BlockSize;
                len -= BlockSize;
            }
            else
            {
                const uint32_t n = Max(len, (BlockSize - m_bufLen));

                MemCopy(m_buf + m_bufLen, data, n);
                m_bufLen += n;
                data += n;
                len -= n;

                if (m_bufLen == BlockSize)
                {
                    Sha1Block_SW(m_state, m_buf);
                    m_length += BlockSize * 8;
                    m_bufLen = 0;
                }
            }
        }
    }

    void Hash<SHA1>::Process_SSE41(const uint8_t* data, uint32_t len)
    {
        HE_UNUSED(data, len);
    #if HE_ENABLE_HARDWARE_SHA1 && HE_CPU_x86
        __m128i ABCD = _mm_loadu_si128(reinterpret_cast<const __m128i*>(m_state));
        __m128i E0 = _mm_set_epi32(m_state[4], 0, 0, 0);
        ABCD = _mm_shuffle_epi32(ABCD, 0x1B);

        while (len > 0)
        {
            if (m_bufLen == 0 && len >= BlockSize)
            {
                Sha1Block_SSE41(ABCD, E0, data);
                m_length += BlockSize * 8;
                data += BlockSize;
                len -= BlockSize;
            }
            else
            {
                const uint32_t n = Max(len, (BlockSize - m_bufLen));

                MemCopy(m_buf + m_bufLen, data, n);
                m_bufLen += n;
                data += n;
                len -= n;

                if (m_bufLen == BlockSize)
                {
                    Sha1Block_SSE41(ABCD, E0, m_buf);
                    m_length += BlockSize * 8;
                    m_bufLen = 0;
                }
            }
        }

        ABCD = _mm_shuffle_epi32(ABCD, 0x1B);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(m_state), ABCD);
        m_state[4] = _mm_extract_epi32(E0, 3);
    #endif
    }

    void Hash<SHA1>::Process_NEON(const uint8_t* data, uint32_t len)
    {
        HE_UNUSED(data, len);
    #if HE_ENABLE_HARDWARE_SHA1 && HE_CPU_ARM_64
        uint32x4_t ABCD = vld1q_u32(m_state);
        uint32_t E0 = state[4];

        while (len > 0)
        {
            if (m_bufLen == 0 && len >= BlockSize)
            {
                Sha1Block_NEON(ABCD, E0, data);
                m_length += BlockSize * 8;
                data += BlockSize;
                len -= BlockSize;
            }
            else
            {
                const uint32_t n = Max(len, (BlockSize - m_bufLen));

                MemCopy(m_buf + m_bufLen, data, n);
                m_bufLen += n;
                data += n;
                len -= n;

                if (m_bufLen == BlockSize)
                {
                    Sha1Block_NEON(ABCD, E0, m_buf);
                    m_length += BlockSize * 8;
                    m_bufLen = 0;
                }
            }
        }

        vst1q_u32(m_state, ABCD);
        m_state[4] = E0;
    #endif
    }

    Hash<SHA1>::ValueType Hash<SHA1>::Finalize()
    {
        HE_ASSERT(m_bufLen < sizeof(m_buf));

        // increase the length of the message
        m_length += m_bufLen * 8;

        // append the '1' bit
        m_buf[m_bufLen++] = 0x80;

        // if the length is currently above 56 bytes we append zeros
        // then compress.  Then we can fall back to padding zeros and length
        // encoding like normal.
        constexpr uint32_t FullBufSize = BlockSize - 8;
        if (m_bufLen > FullBufSize)
        {
            if (m_bufLen < BlockSize)
            {
                MemZero(m_buf + m_bufLen, BlockSize - m_bufLen);
                m_bufLen = BlockSize;
            }

            Sha1Block_SW(m_state, m_buf);
            m_bufLen = 0;
        }

        // pad up to 56 bytes of zeros
        if (m_bufLen < FullBufSize)
        {
            MemZero(m_buf + m_bufLen, FullBufSize - m_bufLen);
        }

        // store length
        StoreBE(*reinterpret_cast<uint64_t*>(m_buf + FullBufSize), m_length);
        Sha1Block_SW(m_state, m_buf);
        m_bufLen = 0;

        // store hash
        ValueType hash;
        for (size_t i = 0; i < 5; ++i)
        {
            StoreBE(*reinterpret_cast<uint32_t*>(hash.bytes + (4 * i)), m_state[i]);
        }

        return hash;
    }
}
