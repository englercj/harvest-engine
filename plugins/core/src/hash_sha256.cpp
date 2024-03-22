// Copyright Chad Engler

#include "he/core/hash.h"

#include "he/core/assert.h"
#include "he/core/cpu.h"
#include "he/core/cpu_info.h"
#include "he/core/memory_ops.h"
#include "he/core/simd.h"

#if !defined(HE_ENABLE_HARDWARE_SHA256)
    #define HE_ENABLE_HARDWARE_SHA256   (HE_CPU_X86 || HE_CPU_ARM_64)
#endif

namespace he
{
    //-------------------------------------------------------------------------------------------------
    HE_FORCE_INLINE uint32_t Sha256_E0(uint32_t x) { return Rotr(x, 2) ^ Rotr(x, 13) ^ Rotr(x, 22); }
    HE_FORCE_INLINE uint32_t Sha256_E1(uint32_t x) { return Rotr(x, 6) ^ Rotr(x, 11) ^ Rotr(x, 25); }
    HE_FORCE_INLINE uint32_t Sha256_S0(uint32_t x) { return Rotr(x, 7) ^ Rotr(x, 18) ^ (x >> 3); }
    HE_FORCE_INLINE uint32_t Sha256_S1(uint32_t x) { return Rotr(x, 17) ^ Rotr(x, 19) ^ (x >> 10); }
    // HE_FORCE_INLINE uint32_t Sha256_Ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
    HE_FORCE_INLINE uint32_t Sha256_Ch(uint32_t x, uint32_t y, uint32_t z) { return z ^ (x & (y ^ z)); }
    HE_FORCE_INLINE uint32_t Sha256_Maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
    // HE_FORCE_INLINE uint32_t Sha256_Maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (z & (x | y)); }
    HE_FORCE_INLINE uint32_t ByteToU32(uint8_t x, uint32_t shift) { return static_cast<uint32_t>(x) << shift; }

    static const uint32_t Sha256_K[] =
    {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
    };
    static_assert(HE_LENGTH_OF(Sha256_K) == 64);

    [[maybe_unused]] HE_FORCE_INLINE void Sha256Block_SW(uint32_t state[5], const uint8_t* data)
    {
        uint32_t a = state[0];
        uint32_t b = state[1];
        uint32_t c = state[2];
        uint32_t d = state[3];
        uint32_t e = state[4];
        uint32_t f = state[5];
        uint32_t g = state[6];
        uint32_t h = state[7];

        uint32_t X[16];
        uint32_t i = 0;

        for (; i < 16; ++i)
        {
            X[i] = ByteToU32(data[0], 24) | ByteToU32(data[1], 16) | ByteToU32(data[2], 8) | ByteToU32(data[3], 0);
            data += 4;

            const uint32_t t1 = X[i] + h + Sha256_E1(e) + Sha256_Ch(e, f, g) + Sha256_K[i];
            const uint32_t t2 = Sha256_E0(a) + Sha256_Maj(a, b, c);

            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        for (; i < 64; ++i)
        {
            const uint32_t s0 = Sha256_S0(X[(i + 1) & 0x0f]);
            const uint32_t s1 = Sha256_S1(X[(i + 14) & 0x0f]);

            X[i & 0xf] += s0 + s1 + X[(i + 9) & 0xf];
            const uint32_t t1 = X[i & 0xf] + h + Sha256_E1(e) + Sha256_Ch(e, f, g) + Sha256_K[i];
            const uint32_t t2 = Sha256_E0(a) + Sha256_Maj(a, b, c);

            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

#if HE_ENABLE_HARDWARE_SHA256
    //-------------------------------------------------------------------------------------------------
#if HE_CPU_X86
    HE_FORCE_INLINE void Sha256Block_SSE41(__m128i& STATE0, __m128i& STATE1, const uint8_t* data)
    {
        const __m128i MASK = _mm_set_epi64x(0x0c0d0e0f08090a0bull, 0x0405060700010203ull);
        __m128i STATE0_SAVE = STATE0;
        __m128i STATE1_SAVE = STATE1;
        __m128i MSG;
        __m128i TMP;

        // Rounds 0-3
        __m128i MSG0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data));
        MSG0 = _mm_shuffle_epi8(MSG0, MASK);
        MSG = _mm_add_epi32(MSG0, _mm_set_epi64x(0xe9b5dba5b5c0fbcfull, 0x71374491428a2f98ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

        // Rounds 4-7
        __m128i MSG1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 16));
        MSG1 = _mm_loadu_si128((const __m128i*) (data+16));
        MSG1 = _mm_shuffle_epi8(MSG1, MASK);
        MSG = _mm_add_epi32(MSG1, _mm_set_epi64x(0xab1c5ed5923f82a4ull, 0x59f111f13956c25bull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG0 = _mm_sha256msg1_epu32(MSG0, MSG1);

        // Rounds 8-11
        __m128i MSG2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 32));
        MSG2 = _mm_loadu_si128((const __m128i*) (data+32));
        MSG2 = _mm_shuffle_epi8(MSG2, MASK);
        MSG = _mm_add_epi32(MSG2, _mm_set_epi64x(0x550c7dc3243185beull, 0x12835b01d807aa98ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG1 = _mm_sha256msg1_epu32(MSG1, MSG2);

        // Rounds 12-15
        __m128i MSG3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 48));
        MSG3 = _mm_loadu_si128((const __m128i*) (data+48));
        MSG3 = _mm_shuffle_epi8(MSG3, MASK);
        MSG = _mm_add_epi32(MSG3, _mm_set_epi64x(0xc19bf1749bdc06a7ull, 0x80deb1fe72be5d74ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(MSG3, MSG2, 4);
        MSG0 = _mm_add_epi32(MSG0, TMP);
        MSG0 = _mm_sha256msg2_epu32(MSG0, MSG3);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG2 = _mm_sha256msg1_epu32(MSG2, MSG3);

        // Rounds 16-19
        MSG = _mm_add_epi32(MSG0, _mm_set_epi64x(0x240ca1cc0fc19dc6ull, 0xefbe4786e49b69c1ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(MSG0, MSG3, 4);
        MSG1 = _mm_add_epi32(MSG1, TMP);
        MSG1 = _mm_sha256msg2_epu32(MSG1, MSG0);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG3 = _mm_sha256msg1_epu32(MSG3, MSG0);

        // Rounds 20-23
        MSG = _mm_add_epi32(MSG1, _mm_set_epi64x(0x76f988da5cb0a9dcull, 0x4a7484aa2de92c6full));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(MSG1, MSG0, 4);
        MSG2 = _mm_add_epi32(MSG2, TMP);
        MSG2 = _mm_sha256msg2_epu32(MSG2, MSG1);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG0 = _mm_sha256msg1_epu32(MSG0, MSG1);

        // Rounds 24-27
        MSG = _mm_add_epi32(MSG2, _mm_set_epi64x(0xbf597fc7b00327c8ull, 0xa831c66d983e5152ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(MSG2, MSG1, 4);
        MSG3 = _mm_add_epi32(MSG3, TMP);
        MSG3 = _mm_sha256msg2_epu32(MSG3, MSG2);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG1 = _mm_sha256msg1_epu32(MSG1, MSG2);

        // Rounds 28-31
        MSG = _mm_add_epi32(MSG3, _mm_set_epi64x(0x1429296706ca6351ull,  0xd5a79147c6e00bf3ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(MSG3, MSG2, 4);
        MSG0 = _mm_add_epi32(MSG0, TMP);
        MSG0 = _mm_sha256msg2_epu32(MSG0, MSG3);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG2 = _mm_sha256msg1_epu32(MSG2, MSG3);

        // Rounds 32-35
        MSG = _mm_add_epi32(MSG0, _mm_set_epi64x(0x53380d134d2c6dfcull, 0x2e1b213827b70a85ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(MSG0, MSG3, 4);
        MSG1 = _mm_add_epi32(MSG1, TMP);
        MSG1 = _mm_sha256msg2_epu32(MSG1, MSG0);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG3 = _mm_sha256msg1_epu32(MSG3, MSG0);

        // Rounds 36-39
        MSG = _mm_add_epi32(MSG1, _mm_set_epi64x(0x92722c8581c2c92eull, 0x766a0abb650a7354ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(MSG1, MSG0, 4);
        MSG2 = _mm_add_epi32(MSG2, TMP);
        MSG2 = _mm_sha256msg2_epu32(MSG2, MSG1);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG0 = _mm_sha256msg1_epu32(MSG0, MSG1);

        // Rounds 40-43
        MSG = _mm_add_epi32(MSG2, _mm_set_epi64x(0xc76c51a3c24b8b70ull, 0xa81a664ba2bfe8a1ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(MSG2, MSG1, 4);
        MSG3 = _mm_add_epi32(MSG3, TMP);
        MSG3 = _mm_sha256msg2_epu32(MSG3, MSG2);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG1 = _mm_sha256msg1_epu32(MSG1, MSG2);

        // Rounds 44-47
        MSG = _mm_add_epi32(MSG3, _mm_set_epi64x(0x106aa070f40e3585ull, 0xd6990624d192e819ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(MSG3, MSG2, 4);
        MSG0 = _mm_add_epi32(MSG0, TMP);
        MSG0 = _mm_sha256msg2_epu32(MSG0, MSG3);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG2 = _mm_sha256msg1_epu32(MSG2, MSG3);

        // Rounds 48-51
        MSG = _mm_add_epi32(MSG0, _mm_set_epi64x(0x34b0bcb52748774cull, 0x1e376c0819a4c116ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(MSG0, MSG3, 4);
        MSG1 = _mm_add_epi32(MSG1, TMP);
        MSG1 = _mm_sha256msg2_epu32(MSG1, MSG0);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
        MSG3 = _mm_sha256msg1_epu32(MSG3, MSG0);

        // Rounds 52-55
        MSG = _mm_add_epi32(MSG1, _mm_set_epi64x(0x682e6ff35b9cca4full, 0x4ed8aa4a391c0cb3ull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(MSG1, MSG0, 4);
        MSG2 = _mm_add_epi32(MSG2, TMP);
        MSG2 = _mm_sha256msg2_epu32(MSG2, MSG1);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

        // Rounds 56-59
        MSG = _mm_add_epi32(MSG2, _mm_set_epi64x(0x8cc7020884c87814ull, 0x78a5636f748f82eeull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        TMP = _mm_alignr_epi8(MSG2, MSG1, 4);
        MSG3 = _mm_add_epi32(MSG3, TMP);
        MSG3 = _mm_sha256msg2_epu32(MSG3, MSG2);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

        // Rounds 60-63
        MSG = _mm_add_epi32(MSG3, _mm_set_epi64x(0xc67178f2bef9a3f7ull, 0xa4506ceb90befffaull));
        STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
        MSG = _mm_shuffle_epi32(MSG, 0x0E);
        STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

        // Combine state
        STATE0 = _mm_add_epi32(STATE0, STATE0_SAVE);
        STATE1 = _mm_add_epi32(STATE1, STATE1_SAVE);
    }

    //-------------------------------------------------------------------------------------------------
#elif HE_CPU_ARM_64
    HE_FORCE_INLINE void Sha256Block_NEON(uint32x4_t& STATE0, uint32x4_t& STATE1, const uint8_t* data)
    {
        uint32x4_t STATE0_SAVE = STATE0;
        uint32x4_t STATE1_SAVE = STATE1;

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

        TMP0 = vaddq_u32(MSG0, vld1q_u32(&Sha256_K[0x00]));
    #endif

        // Rounds 0-3
        MSG0 = vsha256su0q_u32(MSG0, MSG1);
        TMP2 = STATE0;
        TMP1 = vaddq_u32(MSG1, vld1q_u32(&Sha256_K[0x04]));
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG0 = vsha256su1q_u32(MSG0, MSG2, MSG3);

        // Rounds 4-7
        MSG1 = vsha256su0q_u32(MSG1, MSG2);
        TMP2 = STATE0;
        TMP0 = vaddq_u32(MSG2, vld1q_u32(&Sha256_K[0x08]));
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
        MSG1 = vsha256su1q_u32(MSG1, MSG3, MSG0);

        // Rounds 8-11
        MSG2 = vsha256su0q_u32(MSG2, MSG3);
        TMP2 = STATE0;
        TMP1 = vaddq_u32(MSG3, vld1q_u32(&Sha256_K[0x0c]));
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG2 = vsha256su1q_u32(MSG2, MSG0, MSG1);

        // Rounds 12-15
        MSG3 = vsha256su0q_u32(MSG3, MSG0);
        TMP2 = STATE0;
        TMP0 = vaddq_u32(MSG0, vld1q_u32(&Sha256_K[0x10]));
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
        MSG3 = vsha256su1q_u32(MSG3, MSG1, MSG2);

        // Rounds 16-19
        MSG0 = vsha256su0q_u32(MSG0, MSG1);
        TMP2 = STATE0;
        TMP1 = vaddq_u32(MSG1, vld1q_u32(&Sha256_K[0x14]));
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG0 = vsha256su1q_u32(MSG0, MSG2, MSG3);

        // Rounds 20-23
        MSG1 = vsha256su0q_u32(MSG1, MSG2);
        TMP2 = STATE0;
        TMP0 = vaddq_u32(MSG2, vld1q_u32(&Sha256_K[0x18]));
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
        MSG1 = vsha256su1q_u32(MSG1, MSG3, MSG0);

        // Rounds 24-27
        MSG2 = vsha256su0q_u32(MSG2, MSG3);
        TMP2 = STATE0;
        TMP1 = vaddq_u32(MSG3, vld1q_u32(&Sha256_K[0x1c]));
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG2 = vsha256su1q_u32(MSG2, MSG0, MSG1);

        // Rounds 28-31
        MSG3 = vsha256su0q_u32(MSG3, MSG0);
        TMP2 = STATE0;
        TMP0 = vaddq_u32(MSG0, vld1q_u32(&Sha256_K[0x20]));
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
        MSG3 = vsha256su1q_u32(MSG3, MSG1, MSG2);

        // Rounds 32-35
        MSG0 = vsha256su0q_u32(MSG0, MSG1);
        TMP2 = STATE0;
        TMP1 = vaddq_u32(MSG1, vld1q_u32(&Sha256_K[0x24]));
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG0 = vsha256su1q_u32(MSG0, MSG2, MSG3);

        // Rounds 36-39
        MSG1 = vsha256su0q_u32(MSG1, MSG2);
        TMP2 = STATE0;
        TMP0 = vaddq_u32(MSG2, vld1q_u32(&Sha256_K[0x28]));
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
        MSG1 = vsha256su1q_u32(MSG1, MSG3, MSG0);

        // Rounds 40-43
        MSG2 = vsha256su0q_u32(MSG2, MSG3);
        TMP2 = STATE0;
        TMP1 = vaddq_u32(MSG3, vld1q_u32(&Sha256_K[0x2c]));
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG2 = vsha256su1q_u32(MSG2, MSG0, MSG1);

        // Rounds 44-47
        MSG3 = vsha256su0q_u32(MSG3, MSG0);
        TMP2 = STATE0;
        TMP0 = vaddq_u32(MSG0, vld1q_u32(&Sha256_K[0x30]));
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
        MSG3 = vsha256su1q_u32(MSG3, MSG1, MSG2);

        // Rounds 48-51
        TMP2 = STATE0;
        TMP1 = vaddq_u32(MSG1, vld1q_u32(&Sha256_K[0x34]));
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);

        // Rounds 52-55
        TMP2 = STATE0;
        TMP0 = vaddq_u32(MSG2, vld1q_u32(&Sha256_K[0x38]));
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);

        // Rounds 56-59
        TMP2 = STATE0;
        TMP1 = vaddq_u32(MSG3, vld1q_u32(&Sha256_K[0x3c]));
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);

        // Rounds 60-63
        TMP2 = STATE0;
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);

        // Combine state
        STATE0 = vaddq_u32(STATE0, STATE0_SAVE);
        STATE1 = vaddq_u32(STATE1, STATE1_SAVE);
    }
#else
    #error "Hardware SHA256 not available for this architecture"
#endif
#endif

    //-------------------------------------------------------------------------------------------------
    SHA256::Value SHA256::Mem(const void* data, uint32_t len)
    {
        Hash<SHA256> hash;
        hash.Update(data, len);
        return hash.Final();
    }

    void Hash<SHA256>::Reset()
    {
        m_length = 0;
        m_state[0] = 0x6a09e667;
        m_state[1] = 0xbb67ae85;
        m_state[2] = 0x3c6ef372;
        m_state[3] = 0xa54ff53a;
        m_state[4] = 0x510e527f;
        m_state[5] = 0x9b05688c;
        m_state[6] = 0x1f83d9ab;
        m_state[7] = 0x5be0cd19;

        m_bufLen = 0;
        MemZero(m_buf, sizeof(m_buf));
    }

    Hash<SHA256>& Hash<SHA256>::Update(const void* data, uint32_t len)
    {
        if (len == 0)
            return *this;

        const uint8_t* bytes = static_cast<const uint8_t*>(data);

    #if !HE_ENABLE_HARDWARE_SHA256
        Process_SW(bytes, len);
    #elif HE_CPU_X86
        if (GetCpuInfo().x86.sha256)
            Process_SSE41(bytes, len);
        else
            Process_SW(bytes, len);
    #elif HE_CPU_ARM_64
        if (GetCpuInfo().arm.sha256)
            Process_NEON(bytes, len);
        else
            Process_SW(bytes, len);
    #else
        #error "Unknown SHA256 implementation for this configuration"
    #endif

        return *this;
    }

    void Hash<SHA256>::Process_SW(const uint8_t* data, uint32_t len)
    {
        while (len > 0)
        {
            if (m_bufLen == 0 && len >= BlockSize)
            {
                Sha256Block_SW(m_state, data);
                m_length += BlockSize * 8;
                data += BlockSize;
                len -= BlockSize;
            }
            else
            {
                const uint32_t n = Min(len, (BlockSize - m_bufLen));

                MemCopy(m_buf + m_bufLen, data, n);
                m_bufLen += n;
                data += n;
                len -= n;

                if (m_bufLen == BlockSize)
                {
                    Sha256Block_SW(m_state, m_buf);
                    m_length += BlockSize * 8;
                    m_bufLen = 0;
                }
            }
        }
    }

    void Hash<SHA256>::Process_SSE41([[maybe_unused]] const uint8_t* data, [[maybe_unused]] uint32_t len)
    {
    #if HE_ENABLE_HARDWARE_SHA256 && HE_CPU_X86
        __m128i STATE0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(m_state));
        __m128i STATE1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(m_state + 4));

        const __m128i TMP0 = _mm_shuffle_epi32(STATE0, 0xB1);
        const __m128i TMP1 = _mm_shuffle_epi32(STATE1, 0x1B);
        STATE0 = _mm_alignr_epi8(TMP0, TMP1, 8);
        STATE1 = _mm_blend_epi16(TMP1, TMP0, 0xF0);

        while (len > 0)
        {
            if (m_bufLen == 0 && len >= BlockSize)
            {
                Sha256Block_SSE41(STATE0, STATE1, data);
                m_length += BlockSize * 8;
                data += BlockSize;
                len -= BlockSize;
            }
            else
            {
                const uint32_t n = Min(len, (BlockSize - m_bufLen));

                MemCopy(m_buf + m_bufLen, data, n);
                m_bufLen += n;
                data += n;
                len -= n;

                if (m_bufLen == BlockSize)
                {
                    Sha256Block_SSE41(STATE0, STATE1, m_buf);
                    m_length += BlockSize * 8;
                    m_bufLen = 0;
                }
            }
        }

        const __m128i TMP2 = _mm_shuffle_epi32(STATE0, 0x1B);
        const __m128i TMP3 = _mm_shuffle_epi32(STATE1, 0xB1);
        STATE0 = _mm_blend_epi16(TMP2, TMP3, 0xF0);
        STATE1 = _mm_alignr_epi8(TMP3, TMP2, 8);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(m_state), STATE0);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(m_state + 4), STATE1);
    #endif
    }

    void Hash<SHA256>::Process_NEON([[maybe_unused]] const uint8_t* data, [[maybe_unused]] uint32_t len)
    {
    #if HE_ENABLE_HARDWARE_SHA256 && HE_CPU_ARM_64
        uint32x4_t STATE0 = vld1q_u32(m_state);
        uint32x4_t STATE1 = vld1q_u32(m_state + 4);

        while (len > 0)
        {
            if (m_bufLen == 0 && len >= BlockSize)
            {
                Sha256Block_NEON(STATE0, STATE1, data);
                m_length += BlockSize * 8;
                data += BlockSize;
                len -= BlockSize;
            }
            else
            {
                const uint32_t n = Min(len, (BlockSize - m_bufLen));

                MemCopy(m_buf + m_bufLen, data, n);
                m_bufLen += n;
                data += n;
                len -= n;

                if (m_bufLen == BlockSize)
                {
                    Sha256Block_NEON(STATE0, STATE1, m_buf);
                    m_length += BlockSize * 8;
                    m_bufLen = 0;
                }
            }
        }

        vst1q_u32(m_state, STATE0);
        vst1q_u32(m_state + 4, STATE1);
    #endif
    }

    Hash<SHA256>::ValueType Hash<SHA256>::Final()
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

            Sha256Block_SW(m_state, m_buf);
            m_bufLen = 0;
        }

        // pad up to 56 bytes of zeros
        if (m_bufLen < FullBufSize)
        {
            MemZero(m_buf + m_bufLen, FullBufSize - m_bufLen);
        }

        // store length
        StoreBE(*reinterpret_cast<uint64_t*>(m_buf + FullBufSize), m_length);
        Sha256Block_SW(m_state, m_buf);
        m_bufLen = 0;

        // store hash
        ValueType hash;
        for (size_t i = 0; i < HE_LENGTH_OF(m_state); ++i)
        {
            StoreBE(*reinterpret_cast<uint32_t*>(hash.bytes + (4 * i)), m_state[i]);
        }

        return hash;
    }
}
