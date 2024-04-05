// Copyright Chad Engler

#include "he/core/base64.h"

#include "he/core/assert.h"
#include "he/core/string.h"

#include "simdutf.h"

namespace he
{
    uint32_t Base64Encode(char* dst, uint32_t dstLen, const void* src, uint32_t srcLen)
    {
        if (!HE_VERIFY(dstLen > Base64EncodedSize(srcLen)))
            return 0;

        const size_t len = simdutf::binary_to_base64(static_cast<const char*>(src), srcLen, dst);
        dst[len] = '\0';
        return static_cast<uint32_t>(len);
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
        if (!HE_VERIFY(dstLen >= Base64MaxDecodedSize(srcLen)))
            return 0;

        const simdutf::result rc = simdutf::base64_to_binary(src, srcLen, static_cast<char*>(dst));
        if (rc.error != simdutf::SUCCESS)
            return 0;

        return static_cast<uint32_t>(rc.count);
    }
}
