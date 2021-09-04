// Copyright Chad Engler

#include "console_sink.h"

#include "fmt/format.h"

#include <iostream>

void ConsoleSink(const he::LogSource& source, const he::LogKV* kvs, uint32_t count)
{
    fmt::memory_buffer buf;
    fmt::format_to(fmt::appender(buf), "{}({}): [{}]({}) ", source.file, source.line, AsString(source.level), source.category);

    constexpr auto ValueFmt = FMT_STRING("{} = {}");

    for (uint32_t i = 0; i < count; ++i)
    {
        const he::LogKV& kv = kvs[i];

        switch (kv.type)
        {
            case he::LogKV::ValueType::Bool: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.b); break;
            case he::LogKV::ValueType::Int: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.i); break;
            case he::LogKV::ValueType::Uint: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.u); break;
            case he::LogKV::ValueType::Double: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.d); break;
            case he::LogKV::ValueType::String: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.s.data()); break;
        }

        if (i != (count - 1))
        {
            buf.push_back(',');
            buf.push_back(' ');
        }
    }

    buf.push_back('\n');
    buf.push_back('\0');

    if (source.level >= he::LogLevel::Warn)
    {
        std::cerr << buf.data() << std::endl;
    }
    else
    {
        std::cout << buf.data() << std::endl;
    }
}
