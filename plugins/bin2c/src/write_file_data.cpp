// Copyright Chad Engler

#include "write_file_data.h"

#include "he/core/allocator.h"
#include "he/core/ascii.h"
#include "he/core/fmt.h"
#include "he/core/memory_ops.h"
#include "he/core/result_fmt.h"
#include "he/core/utils.h"

void WriteFileData(he::File& file, he::StringView name, const uint8_t* data, size_t size, bool asText)
{
    constexpr size_t HexBytesPerLine = 16;
    constexpr size_t HexByteStrLen = HE_LENGTH_OF("0x00, ") - 1;
    constexpr size_t HexLineStrLen = HexBytesPerLine * HexByteStrLen;
    constexpr size_t MemoryBufferSize = (1024 * 8);

    he::String buf;
    buf.Reserve(MemoryBufferSize + 256); // little extra to prevent regrowth if a line goes over a bit

    const auto flush = [&]() -> bool
    {
        he::Result r = file.Write(buf.Data(), buf.Size());
        if (!HE_VERIFY(r, HE_KV(result, r)))
            return false;
        buf.Clear();
        return true;
    };

    if (asText)
        FormatTo(buf, "static const char {}[{}] =\n{{\n", name, size);
    else
        FormatTo(buf, "static const unsigned char {}[{}] =\n{{\n", name, size);

    if (data != nullptr)
    {
        char asciiBuf[HexBytesPerLine + 1];

        while (size > HexBytesPerLine)
        {
            for (size_t i = 0; i < HexBytesPerLine; ++i)
                asciiBuf[i] = he::IsPrint(data[i]) && data[i] != '\\' ? data[i] : '.';
            asciiBuf[HexBytesPerLine] = '\0';

            he::FormatTo(buf, "    {0:#04x}, // {1}\n", he::FmtJoin(data, data + HexBytesPerLine, ", "), asciiBuf);

            if (buf.Size() >= MemoryBufferSize)
            {
                if (!flush())
                    return;
            }

            data += HexBytesPerLine;
            size -= HexBytesPerLine;
        }

        if (size > 0)
        {
            for (size_t i = 0; i < size; ++i)
                asciiBuf[i] = he::IsPrint(data[i]) && data[i] != '\\' ? data[i] : '.';
            asciiBuf[size] = '\0';

            const uint32_t lenBefore = buf.Size();
            he::FormatTo(buf, "    {:#04x},", he::FmtJoin(data, data + size, ", "));

            const uint32_t lineLength = buf.Size() - lenBefore;
            if (lineLength < HexLineStrLen)
                buf.Resize(buf.Size() + (HexLineStrLen - lineLength) + 3, ' ');

            he::FormatTo(buf, " // {}\n", asciiBuf);
        }
    }

    buf.Append("};\n");
    flush();
}
