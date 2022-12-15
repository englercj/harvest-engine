// Copyright Chad Engler

#include "he/core/stack_trace.h"

#include "he/core/appender.h"
#include "he/core/macros.h"
#include "he/core/result_fmt.h"
#include "he/core/string_fmt.h"
#include "he/core/test.h"

#include "fmt/format.h"

#include <iostream>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, stack_trace, Report)
{
    uintptr_t frames[64]{};
    uint32_t count = HE_LENGTH_OF(frames);
    Result r = CaptureStackTrace(frames, count, 0);
    HE_EXPECT(r, r);
    HE_EXPECT_GT(count, 2);

    String msg;
    SymbolInfo info;
    for (uint32_t i = 0; i < count; ++i)
    {
        const uintptr_t frame = frames[i];
        r = GetSymbolInfo(frame, info);
        HE_EXPECT(r, r);

        msg.Clear();
        if (r)
        {
            if (info.file.IsEmpty())
            {
                fmt::format_to(Appender(msg), "    #{:<3}  {} [{:#018x}]",
                    (i + 1), info.name, frame);
            }
            else
            {
                fmt::format_to(Appender(msg), "    #{:<3}  {} ({}:{})",
                    (i + 1), info.name, info.file, info.line);
            }
        }
        else
        {
            fmt::format_to(Appender(msg), "    #{:<3}  {:#018x}", (i + 1), frame);
        }
        std::cout << msg.Data() << std::endl;
    }
}
