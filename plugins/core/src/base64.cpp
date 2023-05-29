// Copyright Chad Engler

#include "he/core/base64.h"

#include "he/core/assert.h"
#include "he/core/simd.h"

#include "libbase64.h"

namespace he
{
    uint32_t Base64Encode(char* dst, uint32_t dstLen, const void* src, uint32_t srcLen)
    {
        HE_ASSERT(dstLen > Base64EncodedSize(srcLen));

        size_t outLen = dstLen;
        base64_encode(static_cast<const char*>(src), srcLen, dst, &outLen, 0);
        dst[outLen] = '\0';
        return static_cast<uint32_t>(outLen);
    }

    String Base64Encode(const void* src, uint32_t srcLen)
    {
        String dst;
        Base64Encode(dst, src, srcLen);
        return dst;
    }

    void Base64Encode(String& dst, const void* src, uint32_t srcLen)
    {
        const uint32_t encodedSize = Base64EncodedSize(srcLen);
        const uint32_t dstLen = dst.Size();
        dst.Expand(encodedSize, DefaultInit);

        const uint32_t newLen = Base64Encode(dst.Data() + dstLen, (dst.Size() - dstLen) + 1, src, srcLen);
        dst.Resize(dstLen + newLen);
    }

    uint32_t Base64Decode(void* dst, uint32_t dstLen, const char* src, uint32_t srcLen)
    {
        HE_ASSERT(dstLen >= Base64MaxDecodedSize(srcLen));

        size_t outLen = dstLen;
        const int rc = base64_decode(src, srcLen, static_cast<char*>(dst), &outLen, 0);
        return rc == 1 ? static_cast<uint32_t>(outLen) : 0;
    }
}
