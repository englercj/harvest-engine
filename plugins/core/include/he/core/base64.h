// Copyright Chad Engler

#pragma once

#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
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
    /// \note The destination buffer must be at least `Base64EncodedSize(srcLen) + 1` bytes long
    /// so that it has space for the null terminating character that is written to the end.
    ///
    /// \param[in] dst The destination buffer to write the encoded data to.
    /// \param[in] dstLen The length of the destination buffer.
    /// \param[in] src The source data to encode.
    /// \param[in] srcLen The length of the source data.
    /// \return The number of bytes written to the destination buffer not including the null
    ///     terminating byte, zero if there was an error.
    uint32_t Base64Encode(char* dst, uint32_t dstLen, const void* src, uint32_t srcLen);

    /// Encodes the source data into a base64 string.
    ///
    /// \param[in] src The source data to encode.
    /// \param[in] srcLen The length of the source data.
    /// \return The base64 encoded string.
    String Base64Encode(const void* src, uint32_t srcLen);

    /// Encodes the source data into a base64 string.
    ///
    /// \param[in] src The source data to encode.
    /// \return The base64 encoded string.
    inline String Base64Encode(StringView src) { return Base64Encode(src.Data(), src.Size()); }

    /// \copydoc Base64Encode(StringView)
    inline String Base64Encode(Span<const uint8_t> src) { return Base64Encode(src.Data(), src.Size()); }

    /// Encodes the source data into a base64 string. The encoded bytes are written to the end of
    /// the string without changing the existing content.
    ///
    /// \param[in] dst The destination string to write the encoded data to.
    /// \param[in] src The source data to encode.
    /// \param[in] srcLen The length of the source data.
    void Base64Encode(String& dst, const void* src, uint32_t srcLen);

    /// Encodes the source data into a base64 string. The encoded bytes are written to the end of
    /// the string without changing the existing content.
    ///
    /// \param[in] dst The destination string to write the encoded data to.
    /// \param[in] src The source data to encode.
    inline void Base64Encode(String& dst, StringView src) { return Base64Encode(dst, src.Data(), src.Size()); }

    /// \copydoc Base64Encode(String&, StringView)
    inline void Base64Encode(String& dst, Span<const uint8_t> src) { return Base64Encode(dst, src.Data(), src.Size()); }

    /// Decodes the source base64 string into the destination buffer.
    ///
    /// \note The destination buffer must be at least `Base64MaxDecodedSize(srcLen)` bytes long.
    ///
    /// \param[in] dst The destination buffer to write the decoded data to.
    /// \param[in] dstLen The length of the destination buffer.
    /// \param[in] src The source base64 string to decode.
    /// \param[in] srcLen The length of the source base64 string.
    /// \return The number of bytes written to the destination buffer, zero if there was an error.
    uint32_t Base64Decode(void* dst, uint32_t dstLen, const char* src, uint32_t srcLen);

    /// Decodes the source base64 string into the destination buffer.
    ///
    /// \note The destination buffer must be at least `Base64MaxDecodedSize(srcLen)` bytes long.
    ///
    /// \param[in] dst The destination buffer to write the decoded data to.
    /// \param[in] dstLen The length of the destination buffer.
    /// \param[in] src The source base64 string to decode.
    /// \return The number of bytes written to the destination buffer, zero if there was an error.
    inline uint32_t Base64Decode(void* dst, uint32_t dstLen, StringView src) { return Base64Decode(dst, dstLen, src.Data(), src.Size()); }

    /// Decodes the source base64 string into the destination container. The decoded bytes are
    /// written to the end of the container without changing the existing content.
    ///
    /// \note The content of the container is guaranteed to be unchanged if this function fails.
    /// However, the container may have been resized and therefore may have reallocated.
    ///
    /// \tparam The type of the container.
    /// \param[in] dst The destination container to write the decoded data to.
    /// \param[in] src The source base64 string to decode.
    /// \param[in] srcLen The length of the source base64 string.
    /// \return True if the decode worked correctly, or false otherwise.
    template <typename T>
    bool Base64Decode(T& dst, const char* src, uint32_t srcLen)
    {
        constexpr uint32_t ElementSize = sizeof(typename T::ElementType);

        if (srcLen == 0)
            return true;

        const uint32_t maxDecodedSize = Base64MaxDecodedSize(srcLen);
        const uint32_t elementCount = dst.Size();
        const uint32_t elementsNeeded = ElementSize == 1 ? maxDecodedSize : (maxDecodedSize + (ElementSize - 1)) / ElementSize;
        dst.Expand(elementsNeeded, DefaultInit);

        const uint32_t decodedByteSize = Base64Decode(dst.Data() + elementCount, elementsNeeded * ElementSize, src, srcLen);
        const uint32_t decodedElementSize = (decodedByteSize + (ElementSize - 1)) / ElementSize;
        dst.Resize(elementCount + decodedElementSize);
        return decodedByteSize > 0;
    }

    /// Decodes the source base64 string into the destination container. The decoded bytes are
    /// written to the end of the container without changing the existing content.
    ///
    /// \note The content of the container is guaranteed to be unchanged if this function fails.
    /// However, the container may have been resized and therefore may have reallocated.
    ///
    /// \tparam The type of the container.
    /// \param[in] dst The destination container to write the decoded data to.
    /// \param[in] src The source base64 string to decode.
    /// \return True if the decode worked correctly, or false otherwise.
    template <typename T>
    bool Base64Decode(T& dst, StringView src) { return Base64Decode(dst, src.Data(), src.Size()); }
}
