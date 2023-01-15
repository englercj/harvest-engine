// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/hash.h"
#include "he/core/string_view.h"
#include "he/core/types.h"

#include <type_traits>

namespace he
{
    template <typename T>
    [[nodiscard]] constexpr StringView _GetTypeName()
    {
        #if HE_COMPILER_CLANG
            constexpr char FuncSigPrefix[] = "he::StringView he::_GetTypeName() [T = ";
            constexpr char FuncSigSuffix[] = "]";
        #elif HE_COMPILER_GCC
            constexpr char FuncSigPrefix[] = "constexpr he::StringView he::_GetTypeName() [with T = ";
            constexpr char FuncSigSuffix[] = "]";
        #elif HE_COMPILER_MSVC
            constexpr char FuncSigPrefix[] = "class he::StringView __cdecl he::_GetTypeName<";
            constexpr char FuncSigSuffix[] = ">(void)";
        #endif

        constexpr uint32_t FuncSigLen = HE_LENGTH_OF(HE_FUNC_SIG) - 1;
        constexpr uint32_t FuncSigPrefixLen = HE_LENGTH_OF(FuncSigPrefix) - 1;
        constexpr uint32_t FuncSigSuffixLen = HE_LENGTH_OF(FuncSigSuffix) - 1;

        constexpr const char* FuncSig = HE_FUNC_SIG;
        constexpr const char* NameStart = FuncSig + FuncSigPrefixLen;
        constexpr const char* NameEnd = FuncSig + (FuncSigLen - FuncSigSuffixLen);

        return { NameStart, NameEnd };
    }

    class TypeInfo
    {
    public:
        template <typename T>
        [[nodiscard]] static constexpr TypeInfo Get()
        {
            constexpr StringView name = _GetTypeName<std::remove_cv_t<std::remove_reference_t<T>>>();
            return { FNV64::String(name), name };
        }

    public:
        TypeInfo() = default;

        [[nodiscard]] constexpr bool operator==(const TypeInfo& x) const { return m_hash == x.m_hash; }
        [[nodiscard]] constexpr bool operator!=(const TypeInfo& x) const { return m_hash != x.m_hash; }
        [[nodiscard]] constexpr bool operator<(const TypeInfo& x) const { return m_hash < x.m_hash; }

        [[nodiscard]] constexpr uint64_t Hash() const { return m_hash; }
        [[nodiscard]] constexpr const StringView& Name() const { return m_name; }

        [[nodiscard]] constexpr uint64_t HashCode() const noexcept { return Mix64(Hash()); }

    private:
        constexpr TypeInfo(uint64_t hash, const StringView& name)
            : m_hash(hash)
            , m_name(name)
        {}

        uint64_t m_hash{ 0 };
        StringView m_name{};
    };
}
