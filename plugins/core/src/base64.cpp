// Copyright Chad Engler

#include "he/core/base64.h"

#include "he/core/assert.h"
#include "he/core/simd.h"

#include "libbase64.h"

namespace he
{
    uint32_t Base64Encode(char* dst, uint32_t dstLen, const uint8_t* src, uint32_t srcLen)
    {
        HE_ASSERT(dstLen > Base64EncodedSize(srcLen));

        size_t outLen = dstLen;
        base64_encode(reinterpret_cast<const char*>(src), srcLen, dst, &outLen, 0);
        dst[outLen] = '\0';
        return static_cast<uint32_t>(outLen);
    }

    String Base64Encode(const uint8_t* src, uint32_t srcLen)
    {
        String dst;
        Base64EncodeTo(dst, src, srcLen);
        return dst;
    }

    String Base64Encode(Span<const uint8_t> src)
    {
        return Base64Encode(src.Data(), src.Size());
    }

    void Base64EncodeTo(String& dst, const uint8_t* src, uint32_t srcLen)
    {
        const uint32_t dstLen = dst.Size();
        dst.Expand(Base64EncodedSize(srcLen));
        const uint32_t newLen = Base64Encode(dst.Data() + dstLen, (dst.Size() - dstLen) + 1, src, srcLen);
        dst.Resize(dstLen + newLen);
    }

    void Base64EncodeTo(String& dst, Span<const uint8_t> src)
    {
        return Base64EncodeTo(dst, src.Data(), src.Size());
    }

    uint32_t Base64Decode(uint8_t* dst, uint32_t dstLen, const char* src, uint32_t srcLen)
    {
        HE_ASSERT(dstLen >= Base64MaxDecodedSize(srcLen));

        size_t outLen = dstLen;
        const int rc = base64_decode(src, srcLen, reinterpret_cast<char*>(dst), &outLen, 0);
        return rc == 1 ? static_cast<uint32_t>(outLen) : 0;
    }

    uint32_t Base64Decode(uint8_t* dst, uint32_t dstLen, Span<const char> src)
    {
        return Base64Decode(dst, dstLen, src.Data(), src.Size());
    }
}
