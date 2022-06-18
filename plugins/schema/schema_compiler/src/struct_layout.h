// Copyright Chad Engler

#pragma once

#include "he/core/span.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/schema/schema.h"

#include <map>

namespace he::schema
{
    class StructLayout
    {
    public:
        explicit StructLayout(Declaration::Builder decl) noexcept;
        ~StructLayout() noexcept;

        void CalculateLayout();

    private:
        struct MemberRef
        {
            Field::Builder field{};
            Declaration::Builder decl{};
            class FieldPlacer* placer{ nullptr };

            bool isInUnion{ false };
            uint32_t parentIndex{ ~0u };
            uint32_t unionDataOffset{ ~0u };
            uint32_t unionPointerCount{ ~0u };
            uint16_t unionTagCount{ 0 };
        };

    private:
        void CollectMembers(MemberRef& parentRef, uint32_t parentIndex);
        void CollectUnionMembers(MemberRef& parentRef, uint32_t parentIndex);

        uint16_t DataWordSize() const;
        uint32_t MetadataWordSize() const;

    private:
        Declaration::Builder m_decl{};
        Declaration::Data::Struct::Builder m_struct{};

        Vector<MemberRef> m_members{};
        std::map<uint16_t, uint32_t> m_fields{};

        uint16_t m_dataFieldCount{ 0 };

        class StructFieldPlacer* m_fieldPlacer{ nullptr };
        Vector<class FieldPlacer*> m_placers{};
    };
}
