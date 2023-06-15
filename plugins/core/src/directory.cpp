// Copyright Chad Engler

#include "he/core/directory.h"

#include "he/core/enum_ops.h"
#include "he/core/file.h"
#include "he/core/path.h"
#include "he/core/string_ops.h"

namespace he
{
    Result Directory::RemoveContents(const char* path, Allocator& allocator)
    {
        if (!Exists(path))
            return Result::Success;

        Result result;

        DirectoryScanner scanner(allocator);
        result = scanner.Open(path);
        if (!result)
            return result;

        const uint32_t pathLen = StrLen(path);
        String fullPath(allocator);

        DirectoryScanner::Entry entry(allocator);
        while (scanner.NextEntry(entry))
        {
            fullPath.Assign(path, pathLen);
            ConcatPath(fullPath, entry.name);

            if (entry.isDirectory)
            {
                result = RemoveContents(fullPath.Data(), allocator);
                if (!result)
                    return result;

                result = Remove(fullPath.Data());
            }
            else
            {
                result = File::Remove(fullPath.Data());
            }

            if (!result)
                return result;
        }

        return Result::Success;
    }

    template <>
    const char* AsString(SpecialDirectory x)
    {
        switch (x)
        {
            case SpecialDirectory::Documents: return "Documents";
            case SpecialDirectory::LocalAppData: return "LocalAppData";
            case SpecialDirectory::SharedAppData: return "SharedAppData";
            case SpecialDirectory::Temp: return "Temp";
        }

        return "<unknown>";
    }
}
