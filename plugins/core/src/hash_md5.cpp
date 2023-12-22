// Copyright Chad Engler

#include "he/core/hash.h"

#include "he/core/assert.h"
#include "he/core/memory_ops.h"

namespace he
{
    //-------------------------------------------------------------------------------------------------
    #define HE_MD5_F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
    #define HE_MD5_G(x, y, z) ((y) ^ ((z) & ((y) ^ (x))))
    #define HE_MD5_H(x, y, z) ((x) ^ (y) ^ (z))
    #define HE_MD5_I(x, y, z) ((y) ^ ((x) | (~(z))))

    #define HE_MD5_S11 7
    #define HE_MD5_S12 12
    #define HE_MD5_S13 17
    #define HE_MD5_S14 22
    #define HE_MD5_S21 5
    #define HE_MD5_S22 9
    #define HE_MD5_S23 14
    #define HE_MD5_S24 20
    #define HE_MD5_S31 4
    #define HE_MD5_S32 11
    #define HE_MD5_S33 16
    #define HE_MD5_S34 23
    #define HE_MD5_S41 6
    #define HE_MD5_S42 10
    #define HE_MD5_S43 15
    #define HE_MD5_S44 21

    #define HE_MD5_STEP(f, a, b, c, d, x, s, ac) \
            (a) += f((b), (c), (d)) + (x) + static_cast<uint32_t>(ac); \
            (a) = Rotl32((a), (s)); \
            (a) += (b)

    //-------------------------------------------------------------------------------------------------
    MD5::Value MD5::Mem(const void* data, uint32_t len)
    {
        Hash<MD5> hash;
        hash.Update(data, len);
        return hash.Final();
    }

    void Hash<MD5>::Reset()
    {
        m_length = 0;
        m_state[0] = 0x67452301;
        m_state[1] = 0xefcdab89;
        m_state[2] = 0x98badcfe;
        m_state[3] = 0x10325476;

        m_bufLen = 0;
        MemZero(m_buf, sizeof(m_buf));
    }

    Hash<MD5>& Hash<MD5>::Update(const void* data, uint32_t len)
    {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);

        while (len > 0)
        {
            if (m_bufLen == 0 && len >= BlockSize)
            {
                Block(bytes);
                m_length += BlockSize * 8;
                bytes += BlockSize;
                len -= BlockSize;
            }
            else
            {
                const uint32_t n = Min(len, (BlockSize - m_bufLen));

                MemCopy(m_buf + m_bufLen, bytes, n);
                m_bufLen += n;
                bytes += n;
                len -= n;

                if (m_bufLen == BlockSize)
                {
                    Block(m_buf);
                    m_length += BlockSize * 8;
                    m_bufLen = 0;
                }
            }
        }

        return *this;
    }

    Hash<MD5>::ValueType Hash<MD5>::Final()
    {
        HE_ASSERT(m_bufLen < sizeof(m_buf));

        // increase the length of the message
        m_length += m_bufLen * 8;

        // append the '1' bit
        m_buf[m_bufLen++] = 0x80;

        // if the length is currently above 56 bytes we append zeros
        // then compress. Then we can fall back to padding zeros and length
        // encoding like normal.
        constexpr uint32_t FullBufSize = BlockSize - 8;
        if (m_bufLen > FullBufSize)
        {
            if (m_bufLen < BlockSize)
            {
                MemZero(m_buf + m_bufLen, BlockSize - m_bufLen);
                m_bufLen = BlockSize;
            }

            Block(m_buf);
            m_bufLen = 0;
        }

        // pad up to 56 bytes of zeros
        if (m_bufLen < FullBufSize)
        {
            MemZero(m_buf + m_bufLen, FullBufSize - m_bufLen);
        }

        // store length
        StoreLE(*reinterpret_cast<uint64_t*>(m_buf + FullBufSize), m_length);
        Block(m_buf);
        m_bufLen = 0;

        // store hash
        ValueType hash;
        for (size_t i = 0; i < HE_LENGTH_OF(m_state); ++i)
        {
            StoreLE(*reinterpret_cast<uint32_t*>(hash.bytes + (4 * i)), m_state[i]);
        }

        return hash;
    }

    void Hash<MD5>::Block(const uint8_t* data)
    {
        uint32_t W[16];
        for (uint32_t i = 0; i < 16; ++i)
        {
            W[i] = *reinterpret_cast<const uint32_t*>(data + (4 * i));
        }

        uint32_t a = m_state[0];
        uint32_t b = m_state[1];
        uint32_t c = m_state[2];
        uint32_t d = m_state[3];

        // Round 1
        HE_MD5_STEP(HE_MD5_F, a, b, c, d, W[ 0], HE_MD5_S11, 0xd76aa478);
        HE_MD5_STEP(HE_MD5_F, d, a, b, c, W[ 1], HE_MD5_S12, 0xe8c7b756);
        HE_MD5_STEP(HE_MD5_F, c, d, a, b, W[ 2], HE_MD5_S13, 0x242070db);
        HE_MD5_STEP(HE_MD5_F, b, c, d, a, W[ 3], HE_MD5_S14, 0xc1bdceee);
        HE_MD5_STEP(HE_MD5_F, a, b, c, d, W[ 4], HE_MD5_S11, 0xf57c0faf);
        HE_MD5_STEP(HE_MD5_F, d, a, b, c, W[ 5], HE_MD5_S12, 0x4787c62a);
        HE_MD5_STEP(HE_MD5_F, c, d, a, b, W[ 6], HE_MD5_S13, 0xa8304613);
        HE_MD5_STEP(HE_MD5_F, b, c, d, a, W[ 7], HE_MD5_S14, 0xfd469501);
        HE_MD5_STEP(HE_MD5_F, a, b, c, d, W[ 8], HE_MD5_S11, 0x698098d8);
        HE_MD5_STEP(HE_MD5_F, d, a, b, c, W[ 9], HE_MD5_S12, 0x8b44f7af);
        HE_MD5_STEP(HE_MD5_F, c, d, a, b, W[10], HE_MD5_S13, 0xffff5bb1);
        HE_MD5_STEP(HE_MD5_F, b, c, d, a, W[11], HE_MD5_S14, 0x895cd7be);
        HE_MD5_STEP(HE_MD5_F, a, b, c, d, W[12], HE_MD5_S11, 0x6b901122);
        HE_MD5_STEP(HE_MD5_F, d, a, b, c, W[13], HE_MD5_S12, 0xfd987193);
        HE_MD5_STEP(HE_MD5_F, c, d, a, b, W[14], HE_MD5_S13, 0xa679438e);
        HE_MD5_STEP(HE_MD5_F, b, c, d, a, W[15], HE_MD5_S14, 0x49b40821);

        // Round 2
        HE_MD5_STEP(HE_MD5_G, a, b, c, d, W[ 1], HE_MD5_S21, 0xf61e2562);
        HE_MD5_STEP(HE_MD5_G, d, a, b, c, W[ 6], HE_MD5_S22, 0xc040b340);
        HE_MD5_STEP(HE_MD5_G, c, d, a, b, W[11], HE_MD5_S23, 0x265e5a51);
        HE_MD5_STEP(HE_MD5_G, b, c, d, a, W[ 0], HE_MD5_S24, 0xe9b6c7aa);
        HE_MD5_STEP(HE_MD5_G, a, b, c, d, W[ 5], HE_MD5_S21, 0xd62f105d);
        HE_MD5_STEP(HE_MD5_G, d, a, b, c, W[10], HE_MD5_S22,  0x2441453);
        HE_MD5_STEP(HE_MD5_G, c, d, a, b, W[15], HE_MD5_S23, 0xd8a1e681);
        HE_MD5_STEP(HE_MD5_G, b, c, d, a, W[ 4], HE_MD5_S24, 0xe7d3fbc8);
        HE_MD5_STEP(HE_MD5_G, a, b, c, d, W[ 9], HE_MD5_S21, 0x21e1cde6);
        HE_MD5_STEP(HE_MD5_G, d, a, b, c, W[14], HE_MD5_S22, 0xc33707d6);
        HE_MD5_STEP(HE_MD5_G, c, d, a, b, W[ 3], HE_MD5_S23, 0xf4d50d87);
        HE_MD5_STEP(HE_MD5_G, b, c, d, a, W[ 8], HE_MD5_S24, 0x455a14ed);
        HE_MD5_STEP(HE_MD5_G, a, b, c, d, W[13], HE_MD5_S21, 0xa9e3e905);
        HE_MD5_STEP(HE_MD5_G, d, a, b, c, W[ 2], HE_MD5_S22, 0xfcefa3f8);
        HE_MD5_STEP(HE_MD5_G, c, d, a, b, W[ 7], HE_MD5_S23, 0x676f02d9);
        HE_MD5_STEP(HE_MD5_G, b, c, d, a, W[12], HE_MD5_S24, 0x8d2a4c8a);

        // Round 3
        HE_MD5_STEP(HE_MD5_H, a, b, c, d, W[ 5], HE_MD5_S31, 0xfffa3942);
        HE_MD5_STEP(HE_MD5_H, d, a, b, c, W[ 8], HE_MD5_S32, 0x8771f681);
        HE_MD5_STEP(HE_MD5_H, c, d, a, b, W[11], HE_MD5_S33, 0x6d9d6122);
        HE_MD5_STEP(HE_MD5_H, b, c, d, a, W[14], HE_MD5_S34, 0xfde5380c);
        HE_MD5_STEP(HE_MD5_H, a, b, c, d, W[ 1], HE_MD5_S31, 0xa4beea44);
        HE_MD5_STEP(HE_MD5_H, d, a, b, c, W[ 4], HE_MD5_S32, 0x4bdecfa9);
        HE_MD5_STEP(HE_MD5_H, c, d, a, b, W[ 7], HE_MD5_S33, 0xf6bb4b60);
        HE_MD5_STEP(HE_MD5_H, b, c, d, a, W[10], HE_MD5_S34, 0xbebfbc70);
        HE_MD5_STEP(HE_MD5_H, a, b, c, d, W[13], HE_MD5_S31, 0x289b7ec6);
        HE_MD5_STEP(HE_MD5_H, d, a, b, c, W[ 0], HE_MD5_S32, 0xeaa127fa);
        HE_MD5_STEP(HE_MD5_H, c, d, a, b, W[ 3], HE_MD5_S33, 0xd4ef3085);
        HE_MD5_STEP(HE_MD5_H, b, c, d, a, W[ 6], HE_MD5_S34,  0x4881d05);
        HE_MD5_STEP(HE_MD5_H, a, b, c, d, W[ 9], HE_MD5_S31, 0xd9d4d039);
        HE_MD5_STEP(HE_MD5_H, d, a, b, c, W[12], HE_MD5_S32, 0xe6db99e5);
        HE_MD5_STEP(HE_MD5_H, c, d, a, b, W[15], HE_MD5_S33, 0x1fa27cf8);
        HE_MD5_STEP(HE_MD5_H, b, c, d, a, W[ 2], HE_MD5_S34, 0xc4ac5665);

        // Round 4
        HE_MD5_STEP(HE_MD5_I, a, b, c, d, W[ 0], HE_MD5_S41, 0xf4292244);
        HE_MD5_STEP(HE_MD5_I, d, a, b, c, W[ 7], HE_MD5_S42, 0x432aff97);
        HE_MD5_STEP(HE_MD5_I, c, d, a, b, W[14], HE_MD5_S43, 0xab9423a7);
        HE_MD5_STEP(HE_MD5_I, b, c, d, a, W[ 5], HE_MD5_S44, 0xfc93a039);
        HE_MD5_STEP(HE_MD5_I, a, b, c, d, W[12], HE_MD5_S41, 0x655b59c3);
        HE_MD5_STEP(HE_MD5_I, d, a, b, c, W[ 3], HE_MD5_S42, 0x8f0ccc92);
        HE_MD5_STEP(HE_MD5_I, c, d, a, b, W[10], HE_MD5_S43, 0xffeff47d);
        HE_MD5_STEP(HE_MD5_I, b, c, d, a, W[ 1], HE_MD5_S44, 0x85845dd1);
        HE_MD5_STEP(HE_MD5_I, a, b, c, d, W[ 8], HE_MD5_S41, 0x6fa87e4f);
        HE_MD5_STEP(HE_MD5_I, d, a, b, c, W[15], HE_MD5_S42, 0xfe2ce6e0);
        HE_MD5_STEP(HE_MD5_I, c, d, a, b, W[ 6], HE_MD5_S43, 0xa3014314);
        HE_MD5_STEP(HE_MD5_I, b, c, d, a, W[13], HE_MD5_S44, 0x4e0811a1);
        HE_MD5_STEP(HE_MD5_I, a, b, c, d, W[ 4], HE_MD5_S41, 0xf7537e82);
        HE_MD5_STEP(HE_MD5_I, d, a, b, c, W[11], HE_MD5_S42, 0xbd3af235);
        HE_MD5_STEP(HE_MD5_I, c, d, a, b, W[ 2], HE_MD5_S43, 0x2ad7d2bb);
        HE_MD5_STEP(HE_MD5_I, b, c, d, a, W[ 9], HE_MD5_S44, 0xeb86d391);

        m_state[0] += a;
        m_state[1] += b;
        m_state[2] += c;
        m_state[3] += d;
    }
}
