// Copyright Chad Engler

#include "write_file_data.h"

#include "he/core/allocator.h"
#include "he/core/appender.h"
#include "he/core/ascii.h"
#include "he/core/memory_ops.h"
#include "he/core/result_fmt.h"
#include "he/core/string_view_fmt.h"
#include "he/core/utils.h"

#include "fmt/format.h"

void WriteFileData(he::File& file, he::StringView name, const uint8_t* data, size_t size, bool asText)
{
    constexpr size_t HexBytesPerLine = 16;
    constexpr size_t HexByteStrLen = HE_LENGTH_OF("0x00, ") - 1;
    constexpr size_t HexLineStrLen = HexBytesPerLine * HexByteStrLen;
    constexpr size_t MemoryBufferSize = (1024 * 8);

    constexpr auto HexByteFmt = FMT_STRING("{:#04x}, ");
    constexpr auto HexLineFmt = FMT_STRING("    {}// {}\n");

    he::String buf;
    buf.Reserve(MemoryBufferSize + 256); // little extra to prevent regrowth if a line goes over a bit

    uint32_t bytesWritten = 0;

    if (asText)
        fmt::format_to(he::Appender(buf), "static const char {}[{}] =\n{{\n", name, size);
    else
        fmt::format_to(he::Appender(buf), "static const unsigned char {}[{}] =\n{{\n", name, size);

    if (data != nullptr)
    {
        char hex[HexLineStrLen + 1];
        char ascii[HexBytesPerLine + 1];
        size_t hexPos = 0;
        size_t asciiPos = 0;

        for (size_t i = 0; i < size; ++i)
        {
            auto fmtResult = fmt::format_to_n(&hex[hexPos], sizeof(hex) - hexPos, HexByteFmt, data[asciiPos]);
            *fmtResult.out = '\0';
            HE_ASSERT(fmtResult.size == 6);
            hexPos += fmtResult.size;

            ascii[asciiPos] = he::IsPrint(data[asciiPos]) && data[asciiPos] != '\\' ? data[asciiPos] : '.';
            asciiPos++;

            if (asciiPos == (HE_LENGTH_OF(ascii) - 1))
            {
                ascii[asciiPos] = '\0';
                fmt::format_to(he::Appender(buf), HexLineFmt, hex, ascii);

                if (buf.Size() >= MemoryBufferSize)
                {
                    he::Result r = file.Write(buf.Data(), buf.Size(), &bytesWritten);
                    if (!HE_VERIFY(r, HE_KV(result, r)))
                        return;
                    buf.Clear();
                }

                data += asciiPos;
                hexPos = 0;
                asciiPos = 0;
            }
        }

        if (asciiPos != 0)
        {
            // padd for ascii alignment
            if (hexPos < HexLineStrLen)
            {
                he::MemSet(hex + hexPos, ' ', HexLineStrLen - hexPos);
                hex[HexLineStrLen] = '\0';
            }

            ascii[asciiPos] = '\0';
            fmt::format_to(he::Appender(buf), HexLineFmt, hex, ascii);
        }
    }

    buf += "};\n";

    he::Result r = file.Write(buf.Data(), buf.Size(), &bytesWritten);
    if (!HE_VERIFY(r, HE_KV(result, r)))
        return;
}
