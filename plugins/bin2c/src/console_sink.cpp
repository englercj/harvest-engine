// Copyright Chad Engler

#include "console_sink.h"

#include "he/core/appender.h"
#include "he/core/allocator.h"
#include "he/core/enum_fmt.h"
#include "he/core/log_sinks.h"
#include "he/core/string.h"

#include "fmt/core.h"

#include <iostream>

void ConsoleSink(void* userData, const he::LogSource& source, const he::LogKV* kvs, uint32_t count)
{
    HE_UNUSED(userData);

    he::String msg(he::Allocator::GetTemp());
    fmt::format_to(he::Appender(msg), "{}({}): [{}]({}) ", source.file, source.line, source.level, source.category);
    he::FormatKVsTo(msg, kvs, count);
    msg.PushBack('\n');

    if (source.level >= he::LogLevel::Warn)
    {
        std::cerr << msg.Data() << std::endl;
    }
    else
    {
        std::cout << msg.Data() << std::endl;
    }
}
