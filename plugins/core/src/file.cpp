// Copyright Chad Engler

#include "he/core/file.h"

#include "he/core/enum_ops.h"

namespace he
{
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
