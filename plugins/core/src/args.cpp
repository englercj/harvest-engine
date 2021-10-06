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

    static void WriteArgHelpName(String& ss, const ArgDesc& desc, ArgHelpFormat format)
    {
        const bool required = HasFlags(desc.flags, ArgFlag::Required);

        if (!required && format != ArgHelpFormat::Help)
            ss += '[';

        if (desc.shortArg)
        {
            ss += '-';
            ss += desc.shortArg;
            if (desc.longArg)
            {
                switch (format)
                {
                    case ArgHelpFormat::Help: ss += ", "; break;
                    case ArgHelpFormat::Usage: ss += '|'; break;
                }
            }
        }
        else if (format == ArgHelpFormat::Help)
        {
            ss += "    ";
        }

        if (desc.longArg)
        {
            ss += "--";
            ss += desc.longArg;
        }

        switch (desc.type)
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

    static void WriteUsageString(String& ss, Span<ArgDesc> descs, const char* arg0, const ArgResult* result)
    {
        constexpr const char UsageMsg[] = "Usage: ";

        if (result && result->code != ArgResult::Success)
        {
            ss += result->msg.Data();
            ss += '\n';
        }

        const uint32_t usagePaddingLen = HE_LENGTH_OF(UsageMsg) + String::Length(arg0);
        char* usagePadding = HE_ALLOCA(char, usagePaddingLen);
        MemSet(usagePadding, ' ', usagePaddingLen);

        uint32_t lineStart = ss.Size();

        ss += UsageMsg;
        ss += arg0;
        ss += ' ';

        Vector<const ArgDesc*> sortedDescs(Allocator::GetTemp());
        sortedDescs.Reserve(descs.Size());

        for (const ArgDesc& d : descs)
            sortedDescs.PushBack(&d);

        std::sort(sortedDescs.begin(), sortedDescs.end(), [](const ArgDesc* a, const ArgDesc* b)
        {
            const bool aRequired = HasFlags(a->flags, InternalSignedFlag);
            const bool bRequired = HasFlags(b->flags, InternalSignedFlag);

            if (aRequired != bRequired)
                return aRequired;

            if (a->shortArg != 0 && b->shortArg != 0)
                return a->shortArg < b->shortArg;

            if (a->longArg && b->longArg)
                return String::Less(a->longArg, b->longArg);

            return !!a->longArg;
        });

        for (const ArgDesc* desc : sortedDescs)
        {
            if ((ss.Size() - lineStart) >= std::streamoff(MaxHelpLineLen))
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
        if (!desc.description)
            return;

        const uint32_t maxLen = MaxHelpLineLen - startPos;

        const uint32_t paddingLen = startPos + 1;
        char* padding = HE_ALLOCA(char, paddingLen);
        MemSet(padding, ' ', paddingLen);

        uint32_t len = 0;

        for (const char* s = desc.description; *s; ++s)
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
            if (shortArg != '\0' && desc.shortArg == shortArg)
                return &desc;

            if (longArg != nullptr && String::Equal(desc.longArg, longArg))
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

    template <typename T>
    static void SetOrPushValue(void* dst, bool isVector, const T& value)
    {
        if (isVector)
            static_cast<Vector<T>*>(dst)->PushBack(value);
        else
            *static_cast<T*>(dst) = value;
    }

    static ArgResult ReadIntValue(ArgDesc& desc, const char* value)
    {
        const bool isSigned = HasFlag(desc.flags, InternalSignedFlag);
        const bool isVector = HasFlag(desc.flags, InternalVectorFlag);
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

        switch (desc.size)
        {
            case 8:
                if (isSigned)
                {
                    const int64_t val = String::ToInteger<int64_t>(value, nullptr, b);
                    SetOrPushValue(desc.buffer, isVector, val);
                }
                else
                {
                    const uint64_t val = String::ToInteger<uint64_t>(value, nullptr, b);
                    SetOrPushValue(desc.buffer, isVector, val);
                }
                desc.hasValue = true;
                break;
            case 4:
                if (isSigned)
                {
                    const int32_t val = String::ToInteger<int32_t>(value, nullptr, b);
                    SetOrPushValue(desc.buffer, isVector, val);
                }
                else
                {
                    const uint32_t val = String::ToInteger<uint32_t>(value, nullptr, b);
                    SetOrPushValue(desc.buffer, isVector, val);
                }
                desc.hasValue = true;
                break;
            case 2:
                if (isSigned)
                {
                    const int16_t val = String::ToInteger<int16_t>(value, nullptr, b);
                    SetOrPushValue(desc.buffer, isVector, val);
                }
                else
                {
                    const uint16_t val = String::ToInteger<uint16_t>(value, nullptr, b);
                    SetOrPushValue(desc.buffer, isVector, val);
                }
                desc.hasValue = true;
                break;
            case 1:
                if (isSigned)
                {
                    const int8_t val = String::ToInteger<int8_t>(value, nullptr, b);
                    SetOrPushValue(desc.buffer, isVector, val);
                }
                else
                {
                    const uint8_t val = String::ToInteger<uint8_t>(value, nullptr, b);
                    SetOrPushValue(desc.buffer, isVector, val);
                }
                desc.hasValue = true;
                break;
            default:
                HE_ASSERT(false, "Unknown integer size: {}", desc.size);
                HE_UNREACHABLE();
        }

        return ArgResult(ArgResult::Success);
    }

    static ArgResult ReadFloatValue(ArgDesc& desc, const char* value)
    {
        if (!IsFloat(value))
        {
            ArgResult result(ArgResult::InvalidValue, "Failed to parse value as float: ");
            result.msg += value;
            return result;
        }

        const bool isVector = HasFlag(desc.flags, InternalVectorFlag);

        switch (desc.size)
        {
            case 8:
            {
                const double val = String::ToFloat<double>(value);
                SetOrPushValue(desc.buffer, isVector, val);
                desc.hasValue = true;
                break;
            }
            case 4:
            {                const float val = String::ToFloat<float>(value);
                SetOrPushValue(desc.buffer, isVector, val);
                desc.hasValue = true;
                break;
            }
            default:
                HE_ASSERT(false, "Unknown floating point size: {}", desc.size);
                HE_UNREACHABLE();
        }

        return ArgResult(ArgResult::Success);
    }

    static ArgResult ReadValue(ArgDesc& desc, const char* value)
    {
        if (!HE_VERIFY(desc.buffer))
            return ArgResult(ArgResult::Success);

        const bool isVector = HasFlag(desc.flags, InternalVectorFlag);

        switch (desc.type)
        {
            case ArgType::Boolean:
            {
                HE_ASSERT(desc.size == sizeof(bool));
                SetOrPushValue(desc.buffer, isVector, true);
                desc.hasValue = true;
                break;
            }
            case ArgType::Integer:
            {
                const ArgResult r = ReadIntValue(desc, value);
                if (!r) return r;
                break;
            }
            case ArgType::Float:
            {
                const ArgResult r = ReadFloatValue(desc, value);
                if (!r) return r;
                break;
            }
            case ArgType::String:
            {
                HE_ASSERT(desc.size == sizeof(const char*));
                SetOrPushValue(desc.buffer, isVector, value);
                desc.hasValue = true;
                break;
            }
        }

        return ArgResult(ArgResult::Success);
    }

    static ArgResult ReadFlag(Span<ArgDesc>& descs, const char* arg, ArgDesc*& desc)
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

            if (desc->type == ArgType::Boolean)
            {
                const ArgResult r = ReadValue(*desc, nullptr);
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
                        const ArgResult r = ReadValue(*desc, arg);
                        if (!r) return r;
                        desc = nullptr;
                    }
                }

                break;
            }
        }

        return ArgResult(ArgResult::Success);
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
                ArgResult r = ReadValue(*flagDesc, arg);
                if (!r)
                    return r;
                flagDesc = nullptr;
            }
            else if (*arg == '-')
            {
                ArgResult r = ReadFlag(descs, arg, flagDesc);
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
            if (!desc.hasValue && HasFlags(desc.flags, ArgFlag::Required))
            {
                result.code = ArgResult::MissingRequiredArg;
                result.msg = "Required argument missing: ";
                WriteArgHelpName(result.msg, desc, ArgHelpFormat::Usage);
                return result;
            }
        }

        return result;
    }

    String MakeHelpString(Span<ArgDesc> descs, const char* arg0, const ArgResult* result)
    {
        String ss(Allocator::GetTemp());
        ss.Reserve(1024);

        WriteUsageString(ss, descs, arg0, result);

        uint32_t longestHelpLen = 0;
        {
            String buf(Allocator::GetTemp());

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
