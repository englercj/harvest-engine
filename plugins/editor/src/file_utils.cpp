// Copyright Chad Engler

#include "file_utils.h"

#include "he/core/log.h"
#include "he/core/result_fmt.h"

namespace he::editor
{
    Result LoadFile(Vector<uint8_t>& dst, const char* path)
    {
        File file;

        Result r = file.Open(path, FileOpenMode::ReadExisting, FileOpenFlag::SequentialScan);
        if (!r)
            return r;

        const uint64_t fileSize = file.GetSize();

        if (fileSize > Vector<uint8_t>::MaxElements)
            return Result::NotSupported;

        dst.Resize(static_cast<uint32_t>(fileSize));

        return file.Read(dst.Data(), dst.Size());
    }

    Result SaveFile(const char* path, const Span<uint8_t>& src, bool append)
    {
        File file;

        Result r = file.Open(path, append ? FileOpenMode::WriteAppend : FileOpenMode::WriteTruncate);
        if (!r)
            return r;

        return file.Write(src.Data(), src.Size());
    }
}
