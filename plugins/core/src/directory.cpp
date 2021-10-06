// Copyright Chad Engler

#include "he/core/directory.h"

#include "he/core/file.h"
#include "he/core/path.h"

namespace he::Directory
{
    Result RemoveContents(const char* path, Allocator& allocator)
    {
        if (!Exists(path))
            return Result::Success;

        Result result;

        Scanner scanner(allocator);
        result = scanner.Open(path);
        if (!result)
            return result;

        const uint32_t pathLen = String::Length(path);
        String fullPath(allocator);

        bool isDirectory = false;
        String entry(allocator);
        while (scanner.NextEntry(entry, &isDirectory))
        {
            fullPath.Assign(path, pathLen);
            fullPath += entry;

            if (isDirectory)
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
}
