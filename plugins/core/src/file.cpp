// Copyright Chad Engler

#include "he/core/file.h"

#include "he/core/enum_ops.h"

namespace he
{
    Result File::WriteAll(const void* src, uint32_t size, const char* path, uint32_t* bytesWritten)
    {
        File f;
        Result r = f.Open(path, FileOpenMode::WriteTruncate);
        if (!r)
            return r;

        return f.WriteAt(src, 0, size, bytesWritten);
    }

    template <>
    const char* AsString(FileResult x)
    {
        switch (x)
        {
            case FileResult::Success: return "Success";
            case FileResult::Failure: return "Failure";
            case FileResult::AccessDenied: return "AccessDenied";
            case FileResult::AlreadyExists: return "AlreadyExists";
            case FileResult::DiskFull: return "DiskFull";
            case FileResult::NotFound: return "NotFound";
            case FileResult::NoData: return "NoData";
        }

        return "<unknown>";
    }
}
