// Copyright Chad Engler

#pragma once

#include "he/sqlite/orm.h"
#include "he/sqlite/statement.h"

namespace he::sqlite
{
    template <typename T> struct SqlDataTypeTraits;

    struct OrmCustomValue { uint64_t val; };

    template <>
    struct SqlDataTypeTraits<OrmCustomValue>
    {
        static constexpr StringView Sql = "FAKE_TYPE";
        static bool Bind(Statement& stmt, int32_t index, const OrmCustomValue& value) { stmt.Bind(index, BitCast<int64_t>(value.val)); }
        static void Read(const ColumnReader& column, OrmCustomValue& value) { value.val = BitCast<uint64_t>(column.AsInt64()); }
    };

    struct OrmTestParent final
    {
        uint32_t id{ 0 };

        String filePath{};
        uint32_t filePathDepth{ 0 };
        SystemTime fileWriteTime{ 0 };
        uint32_t fileSize{ 0 };

        String sourcePath{};
        SystemTime sourceWriteTime{ 0 };
        uint32_t sourceSize{ 0 };

        uint32_t scanToken{ 0 };
        OrmCustomValue custom{};
    };

    struct OrmTestChild final
    {
        uint32_t id{ 0 };
        uint32_t parentId{ 0 };
        String name{};
    };

    struct OrmTestHasFunc
    {
        void DoStuff() {}
    };

    template <typename T> struct IsIntTest { static constexpr bool Value = IsSame<T, int>; };
    template <typename T> struct IsFloatTest { static constexpr bool Value = IsSame<T, float>; };
}
