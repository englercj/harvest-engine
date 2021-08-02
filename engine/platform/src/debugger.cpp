// Copyright Chad Engler

#include "he/core/log.h"
#include "he/core/module_registry.h"
#include "he/platform/debugger.h"

#include "fmt/format.h"

namespace he
{
    void DebugOutputSink(const LogSource& source, const LogKV* kvs, uint32_t count)
    {
        fmt::memory_buffer buf;
        fmt::format_to(fmt::appender(buf), "{}({}): [{}]({}) ", source.file, source.line, AsString(source.level), source.category);

        constexpr auto ValueFmt = FMT_STRING("{} = {}");

        for (uint32_t i = 0; i < count; ++i)
        {
            const LogKV& kv = kvs[i];

            switch (kv.type)
            {
                case LogKV::ValueType::Bool: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.b); break;
                case LogKV::ValueType::Int: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.i); break;
                case LogKV::ValueType::Uint: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.u); break;
                case LogKV::ValueType::Double: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.d); break;
                case LogKV::ValueType::String: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.s.data()); break;
            }

            if (i != (count - 1))
            {
                buf.push_back(',');
                buf.push_back(' ');
            }
        }

        buf.push_back('\n');
        buf.push_back('\0');

        ModuleRegistry::Get().GetAPI<Debugger>()->Print(buf.data());
    }
}
