// Copyright Chad Engler

#pragma once

#include "he/core/file.h"

namespace he
{
    const char* AsString(FileResult v)
    {
        switch (v)
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
