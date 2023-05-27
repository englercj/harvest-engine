// Copyright Chad Engler

#pragma once

#include "he/core/concepts.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/types.h"

namespace he
{
    /// Calculates the number of base64 characters required to encode `size` bytes.
    ///
    /// \param[in] size The number of bytes to encode.
    /// \return The number of base64 characters required to encode the given number of bytes.
    constexpr uint32_t Base64EncodedSize(uint32_t size) { return ((size + 2) / 3) * 4; }

    /// Calculates the maximum number of bytes that can be decoded from a base64
    /// encoded string with length `size`.
    ///
    /// \param[in] size The number of encoded base64 characters.
    /// \return The maximum number of decoded bytes.
    constexpr uint32_t Base64MaxDecodedSize(uint32_t size) { return ((size * 3) + 3) / 4; }

    /// Encodes the source data into a base64 string.
    ///
    /// \note The destination buffer must be at least `Base64EncodedSize(srcLen)` bytes long.
    ///
    /// \param[in] dst The destination buffer to write the encoded data to.
    /// \param[in] dstLen The length of the destination buffer.
    /// \param[in] src The source data to encode.
    /// \param[in] srcLen The length of the source data.
    /// \return The number of bytes written to the destination buffer, zero if there was an error.
    uint32_t Base64Encode(char* dst, uint32_t dstLen, const uint8_t* src, uint32_t srcLen);

    /// Encodes the source data into a base64 string.
    ///
    /// \param[in] src The source data to encode.
    /// \param[in] srcLen The length of the source data.
    /// \return The base64 encoded string.
    String Base64Encode(const uint8_t* src, uint32_t srcLen);

    /// Encodes the source data into a base64 string.
    ///
    /// \param[in] src The source data to encode.
    /// \return The base64 encoded string.
    String Base64Encode(Span<const uint8_t> src);

    /// Encodes the source data into a base64 string.
    ///
    /// \note The characters are appended to the destination string, existing content is mutated.
    ///
    /// \param[in] dst The destination string to write the encoded data to.
    /// \param[in] src The source data to encode.
    /// \param[in] srcLen The length of the source data.
    /// \return The base64 encoded string.
    void Base64EncodeTo(String& dst, const uint8_t* src, uint32_t srcLen);

    /// Encodes the source data into a base64 string.
    ///
    /// \note The characters are appended to the destination string, existing content is mutated.
    ///
    /// \param[in] dst The destination string to write the encoded data to.
    /// \param[in] src The source data to encode.
    /// \return The base64 encoded string.
    void Base64EncodeTo(String& dst, Span<const uint8_t> src);

    /// Decodes the source base64 string into the destination buffer.
    ///
    /// \note The destination buffer must be at least `Base64MaxDecodedSize(srcLen)` bytes long.
    ///
    /// \param[in] dst The destination buffer to write the decoded data to.
    /// \param[in] dstLen The length of the destination buffer.
    /// \param[in] src The source base64 string to decode.
    /// \param[in] srcLen The length of the source base64 string.
    /// \return The number of bytes written to the destination buffer, zero if there was an error.
    uint32_t Base64Decode(uint8_t* dst, uint32_t dstLen, const char* src, uint32_t srcLen);

    /// Decodes the source base64 string into the destination buffer.
    ///
    /// \note The destination buffer must be at least `Base64MaxDecodedSize(srcLen)` bytes long.
    ///
    /// \param[in] dst The destination buffer to write the decoded data to.
    /// \param[in] dstLen The length of the destination buffer.
    /// \param[in] src The source base64 string to decode.
    /// \return The number of bytes written to the destination buffer, zero if there was an error.
    uint32_t Base64Decode(uint8_t* dst, uint32_t dstLen, Span<const char> src);

    /// Decodes the source base64 string into the destination container.
    ///
    /// \tparam The type of the container.
    /// \param[in] dst The destination container to write the decoded data to.
    /// \param[in] src The source base64 string to decode.
    /// \param[in] srcLen The length of the source base64 string.
    template <ContiguousRange<uint8_t> T>
    bool Base64Decode(T& dst, const char* src, uint32_t srcLen)
    {
        const uint32_t dstLen = dst.Size();
        dst.Expand(Base64MaxDecodedSize(srcLen));
        const uint32_t newLen = Base64Decode(dst.Data() + dstLen, dst.Size() - dstLen, src, srcLen);
        dst.Resize(dstLen + newLen);
        return newLen > 0;
    }

    /// Decodes the source base64 string into the destination container.
    ///
    /// \tparam The type of the container.
    /// \param[in] dst The destination container to write the decoded data to.
    /// \param[in] src The source base64 string to decode.
    template <ContiguousRange<uint8_t> T>
    bool Base64Decode(T& dst, Span<const char> src) { return Base64Decode<T>(dst, src.Data(), src.Size()); }
}
