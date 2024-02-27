// Copyright Chad Engler

#include "he/core/args.h"

#include "he/core/alloca.h"
#include "he/core/allocator.h"
#include "he/core/ascii.h"
#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/string.h"
#include "he/core/string_ops.h"
#include "he/core/utils.h"

#include <algorithm>

namespace he
{
    constexpr uint32_t MaxHelpLineLen = 80;
    constexpr uint32_t RequiredDescriptionLen = 15;

    enum class ArgHelpFormat
    {
        Help,
        Usage,
    };

    template <>
    const char* EnumTraits<ArgType>::ToString(ArgType x) noexcept
    {
        switch (x)
        {
            case ArgType::Boolean: return "Boolean";
            case ArgType::Integer: return "Integer";
            case ArgType::Float: return "Float";
            case ArgType::String: return "String";
        }

        return "<unknown>";
    }

    static void WriteArgHelpName(String& ss, const ArgDesc& desc, ArgHelpFormat format)
    {
        const bool required = desc.IsRequired();

        if (!required && format != ArgHelpFormat::Help)
            ss += '[';

        if (desc.ShortName())
        {
            ss += '-';
            ss += desc.ShortName();
            switch (format)
            {
                case ArgHelpFormat::Help: ss += ", "; break;
                case ArgHelpFormat::Usage: ss += '|'; break;
            }
        }
        else if (format == ArgHelpFormat::Help)
        {
            ss += "    ";
        }

        if (desc.LongName())
        {
            ss += "--";
            ss += desc.LongName();
        }

        switch (desc.Type())
        {
            case ArgType::Boolean:
                break;
            case ArgType::Integer:
            case ArgType::Float:
            case ArgType::String:
                ss += " <value>";
                break;
        }

        if (!required && format != ArgHelpFormat::Help)
            ss += ']';
    }

    static void WriteUsageString(String& ss, Span<const ArgDesc> descs, const char* arg0, const ArgResult* result)
    {
        constexpr const char UsageMsg[] = "Usage: ";

        if (result && result->code != ArgResult::Success)
        {
            ss += result->msg.Data();
            ss += '\n';
        }

        const uint32_t usagePaddingLen = HE_LENGTH_OF(UsageMsg) + StrLen(arg0);
        char* usagePadding = HE_ALLOCA(char, usagePaddingLen);
        MemSet(usagePadding, ' ', usagePaddingLen);

        uint32_t lineStart = ss.Size();

        ss += UsageMsg;
        ss += arg0;
        ss += ' ';

        Vector<const ArgDesc*> sortedDescs;
        sortedDescs.Reserve(descs.Size());

        for (const ArgDesc& d : descs)
            sortedDescs.PushBack(&d);

        std::sort(sortedDescs.begin(), sortedDescs.end(), [](const ArgDesc* a, const ArgDesc* b)
        {
            const bool aRequired = a->IsSignedValue();
            const bool bRequired = b->IsSignedValue();

            if (aRequired != bRequired)
                return aRequired;

            if (a->ShortName() != 0 && b->ShortName() != 0)
                return a->ShortName() < b->ShortName();

            if (a->LongName() && b->LongName())
                return StrLess(a->LongName(), b->LongName());

            return !!a->LongName();
        });

        for (const ArgDesc* desc : sortedDescs)
        {
            if ((ss.Size() - lineStart) >= MaxHelpLineLen)
            {
                ss += '\n';
                lineStart = ss.Size();
                ss.Append(usagePadding, usagePaddingLen);
            }
            WriteArgHelpName(ss, *desc, ArgHelpFormat::Usage);
            ss += ' ';
        }
    }

    static void WriteDescription(String& ss, const ArgDesc& desc, uint32_t startPos)
    {
        if (!desc.Description())
            return;

        const uint32_t maxLen = MaxHelpLineLen - startPos;

        const uint32_t paddingLen = startPos + 1;
        char* padding = HE_ALLOCA(char, paddingLen);
        MemSet(padding, ' ', paddingLen);

        uint32_t len = 0;

        for (const char* s = desc.Description(); *s; ++s)
        {
            if (*s == ' ' && len >= maxLen)
            {
                ss += '\n';
                ss.Append(padding, paddingLen);
                len = 0;
            }
            else
            {
                ss += *s;
                len++;
            }
        }
    }

    static ArgDesc* FindDesc(Span<ArgDesc>& descs, char shortArg, const char* longArg)
    {
        for (auto& desc : descs)
        {
            if (shortArg != '\0' && desc.ShortName() == shortArg)
                return &desc;

            if (longArg != nullptr && StrEqual(desc.LongName(), longArg))
                return &desc;
        }

        return nullptr;
    }

    static uint32_t DetectBase(const char* v)
    {
        if (v[0] != '0')
            return 10;

        switch (v[1])
        {
            case 'b': return 2;
            case 'x': return 16;
            default: return 8;
        }
    }

    ArgResult ArgDesc::ReadFlag(Span<ArgDesc>& descs, const char* arg, ArgDesc*& desc)
    {
        if (!HE_VERIFY(*arg == '-'))
            return ArgResult(ArgResult::InvalidFormat, "Flag did not start with a dash ('-').");

        const char* originalArg = arg;

        arg++;

        const bool isLong = *arg == '-';

        if (isLong)
            arg++;

        while (*arg != '\0')
        {
            desc = FindDesc(descs, isLong ? 0 : *arg, isLong ? arg : nullptr);

            if (!desc)
            {
                ArgResult result(ArgResult::UnknownArg, "Unknown option: ");
                result.msg += originalArg;
                return result;
            }

            if (desc->Type() == ArgType::Boolean)
            {
                const ArgResult r = desc->ReadValue(nullptr);
                if (!r) return r;
                desc = nullptr;

                arg++;

                // We can only read 1 long arg at a time, but there can be many short args (`-abc`)
                if (isLong)
                    break;
            }
            else
            {
                // short form can have the value next to the flag (`-a10` == `-a 10`)
                if (!isLong)
                {
                    arg++;
                    if (*arg != '\0')
                    {
                        const ArgResult r = desc->ReadValue(arg);
                        if (!r) return r;
                        desc = nullptr;
                    }
                }

                break;
            }
        }

        return ArgResult(ArgResult::Success);
    }

    ArgResult ArgDesc::ReadIntValue(const char* value)
    {
        const uint32_t b = DetectBase(value);
        const char* originalValue = value;

        // Go past leading zero
        if (b != 10)
            value++;

        // Go past the leading b (0b..) or x (0x..)
        if (b == 2 || b == 16)
            value++;

        if ((b == 16 && !IsHex(value)) || (b != 16 && !IsInteger(value)))
        {
            ArgResult result(ArgResult::InvalidValue, "Failed to parse value as integer: ");
            result.msg += originalValue;
            return result;
        }

        switch (m_size)
        {
            case 8:
            {
                if (IsSignedValue())
                {
                    const int64_t val = StrToInt<int64_t>(value, nullptr, b);
                    SetOrPushValue(val);
                }
                else
                {
                    const uint64_t val = StrToInt<uint64_t>(value, nullptr, b);
                    SetOrPushValue(val);
                }
                m_hasValue = true;
                break;
            }
            case 4:
            {
                if (IsSignedValue())
                {
                    const int32_t val = StrToInt<int32_t>(value, nullptr, b);
                    SetOrPushValue(val);
                }
                else
                {
                    const uint32_t val = StrToInt<uint32_t>(value, nullptr, b);
                    SetOrPushValue(val);
                }
                m_hasValue = true;
                break;
            }
            case 2:
            {
                if (IsSignedValue())
                {
                    const int16_t val = StrToInt<int16_t>(value, nullptr, b);
                    SetOrPushValue(val);
                }
                else
                {
                    const uint16_t val = StrToInt<uint16_t>(value, nullptr, b);
                    SetOrPushValue(val);
                }
                m_hasValue = true;
                break;
            }
            case 1:
            {
                if (IsSignedValue())
                {
                    const int8_t val = StrToInt<int8_t>(value, nullptr, b);
                    SetOrPushValue(val);
                }
                else
                {
                    const uint8_t val = StrToInt<uint8_t>(value, nullptr, b);
                    SetOrPushValue(val);
                }
                m_hasValue = true;
                break;
            }
            default:
            {
                HE_VERIFY(false,
                    HE_MSG("Unknown integer size for argument."),
                    HE_KV(arg_type, m_type),
                    HE_KV(arg_size, m_size),
                    HE_KV(arg_short_name, m_shortArg),
                    HE_KV(arg_long_name, m_longArg),
                    HE_KV(arg_description, m_description));

                return ArgResult(ArgResult::InvalidArgDesc, "Argument descriptor is invalid: unknown integer size. See log for more info.");
            }
        }

        return ArgResult(ArgResult::Success);
    }

    ArgResult ArgDesc::ReadFloatValue(const char* value)
    {
        if (!IsFloat(value))
        {
            ArgResult result(ArgResult::InvalidValue, "Failed to parse value as float: ");
            result.msg += value;
            return result;
        }

        switch (m_size)
        {
            case 8:
            {
                const double val = StrToFloat<double>(value);
                SetOrPushValue(val);
                m_hasValue = true;
                break;
            }
            case 4:
            {
                const float val = StrToFloat<float>(value);
                SetOrPushValue(val);
                m_hasValue = true;
                break;
            }
            default:
            {
                HE_VERIFY(false,
                    HE_MSG("Unknown float size for argument."),
                    HE_KV(arg_type, m_type),
                    HE_KV(arg_size, m_size),
                    HE_KV(arg_short_name, m_shortArg),
                    HE_KV(arg_long_name, m_longArg),
                    HE_KV(arg_description, m_description));

                return ArgResult(ArgResult::InvalidArgDesc, "Argument descriptor is invalid: unknown float size. See log for more info.");
            }
        }

        return ArgResult(ArgResult::Success);
    }

    ArgResult ArgDesc::ReadValue(const char* value)
    {
        if (!HE_VERIFY(m_buffer))
            return ArgResult(ArgResult::Success);

        switch (m_type)
        {
            case ArgType::Boolean:
            {
                HE_ASSERT(m_size == sizeof(bool));
                SetOrPushValue<bool>(true);
                m_hasValue = true;
                break;
            }
            case ArgType::Integer:
            {
                return ReadIntValue(value);
            }
            case ArgType::Float:
            {
                return ReadFloatValue(value);
            }
            case ArgType::String:
            {
                HE_ASSERT(m_size == sizeof(const char*));
                SetOrPushValue<const char*>(value);
                m_hasValue = true;
                break;
            }
        }

        return ArgResult(ArgResult::Success);
    }

    template <typename T>
    void ArgDesc::SetOrPushValue(const T& value)
    {
        if (IsVectorValue())
            static_cast<Vector<T>*>(m_buffer)->PushBack(value);
        else
            *static_cast<T*>(m_buffer) = value;
    }

    ArgResult ParseArgs(Span<ArgDesc> descs, int32_t argc, const char* const* argv)
    {
        ArgResult result;
        ArgDesc* flagDesc = nullptr;

        for (int32_t i = 1; i < argc; ++i)
        {
            const char* arg = argv[i];

            if (flagDesc)
            {
                ArgResult r = flagDesc->ReadValue(arg);
                if (!r)
                    return r;
                flagDesc = nullptr;
            }
            else if (*arg == '-')
            {
                ArgResult r = ArgDesc::ReadFlag(descs, arg, flagDesc);
                if (!r)
                    return r;
            }
            else
            {
                result.values.PushBack(arg);
            }
        }

        for (const ArgDesc& desc : descs)
        {
            if (!desc.HasValue() && desc.IsRequired())
            {
                result.code = ArgResult::MissingRequiredArg;
                result.msg = "Required argument missing: ";
                WriteArgHelpName(result.msg, desc, ArgHelpFormat::Usage);
                return result;
            }
        }

        return result;
    }

    String MakeHelpString(Span<const ArgDesc> descs, const char* arg0, const ArgResult* result)
    {
        String ss;
        ss.Reserve(1024);

        WriteUsageString(ss, descs, arg0, result);

        // TODO: This business is completely broken, needs a rewrite.
        uint32_t longestHelpLen = 0;
        {
            String buf;

            for (const ArgDesc& desc : descs)
            {
                buf.Clear();
                WriteArgHelpName(buf, desc, ArgHelpFormat::Help);
                const uint32_t len = buf.Size();

                if (len > longestHelpLen)
                    longestHelpLen = len;
            }
        }

        HE_ASSERT(longestHelpLen <= MaxHelpLineLen - RequiredDescriptionLen);

        ss += "\n\nOptions:\n";

        for (const ArgDesc& desc : descs)
        {
            ss += "  ";

            const uint32_t start = ss.Size();
            WriteArgHelpName(ss, desc, ArgHelpFormat::Help);
            uint32_t len = ss.Size() - start;

            while (len < longestHelpLen)
            {
                ss += ' ';
                len++;
            }

            ss += ' ';
            WriteDescription(ss, desc, len + 3); // +2 to include the starting 2 spaces, +1 to include the trailing space
            ss += '\n';
        }

        ss += '\n';

        return ss;
    }
}
