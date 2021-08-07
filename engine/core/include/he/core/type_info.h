// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/hash.h"
#include "he/core/string_view.h"
#include "he/core/types.h"

namespace he
{
    struct TypeId
    {
        constexpr TypeId(const StringView& name)
            : name(name)
            , hash(FNV64::HashStringN(name.Data(), name.Size()))
        {}

        constexpr bool operator==(const TypeId& x) const { return hash == x.hash; }
        constexpr bool operator!=(const TypeId& x) const { return hash != x.hash; }
        constexpr bool operator<(const TypeId& x) const { return hash < x.hash; }

        const StringView name;
        const uint64_t hash;
    };

    template <typename T>
    constexpr StringView GetTypeName()
    {
        #if HE_COMPILER_CLANG
            constexpr char FuncSigPrefix[] = "he::StringView he::GetTypeName() [T = ";
            constexpr char FuncSigSuffix[] = "]";
        #elif HE_COMPILER_GCC
            constexpr char FuncSigPrefix[] = "constexpr he::StringView he::GetTypeName() [with T = ";
            constexpr char FuncSigSuffix[] = "]";
        #elif HE_COMPILER_MSVC
            constexpr char FuncSigPrefix[] = "class he::StringView __cdecl he::GetTypeName<";
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

    template <typename T>
    constexpr TypeId GetTypeId()
    {
        return TypeId(GetTypeName<T>());
    }
}

namespace std
{
    template <>
    struct hash<he::TypeId>
    {
        constexpr size_t operator()(const he::TypeId& id) const
        {
            return static_cast<size_t>(id.hash);
        }
    };
}
