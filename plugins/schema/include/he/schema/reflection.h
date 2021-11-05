// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/vector.h"

#include <unordered_map>

namespace he::schema
{
    struct FieldInfo
    {
        uint32_t id;
        uint32_t size;
        uint32_t offset;
    };

    class Database
    {
    public:
        static Database& Get();

        template <typename T>
        const typename T::BufferReader* FindById(uint32_t id) const
        {
            const auto it = m_typeIdMap.find(id);
            if (it == m_typeIdMap.end())
                return nullptr;
            HE_ASSERT();
            const TypeIndex& t = it->second;
            switch (t.objType)
            {
                case
            }
        }

    private:
        Database(Database&&) = delete;
        Database(const Database&) = delete;
        Database& operator=(Database&&) = delete;
        Database& operator=(const Database&) = delete;

    private:
        struct TypeIdHasher
        {
            // type ids are already hashes, just use it as-is
            size_t operator()(uint32_t typeId) const { return typeId; }
        };

        struct TypeIndex
        {
            uint32_t index;
            uint32_t objType;
        };

        struct TypeStorage
        {
            void* obj;
            FieldInfo* fields;
            uint32_t fieldCount;
        };

        std::unordered_map<uint32_t, TypeIndex, TypeIdHasher> m_typeIdMap{};
        Vector<TypeStorage> m_types{};
    };
}
