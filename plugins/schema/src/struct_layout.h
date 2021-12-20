// Copyright Chad Engler

#pragma once

#include "he/core/span.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/schema/schema.h"

namespace he::schema
{
    class StructLayout
    {
    public:
        struct FieldRef { Field* field; Declaration* parent; };
        static void CollectSortedFields(Declaration& decl, Vector<FieldRef>& out);

    public:
        StructLayout(Declaration& decl);

        Span<const FieldRef> GetSortedFields() const { return m_sortedFields; }

        void CalculateLayout();

    private:
        void PlaceField(const FieldRef& ref);
        bool TryPlaceUnionField(const FieldRef& ref, uint32_t fieldSize, uint32_t fieldAlign);
        uint32_t PlaceDataField(uint32_t fieldSize, uint32_t fieldAlign);

        uint32_t TryClaimGap(uint32_t fieldSize);
        uint32_t TryClaimGapIndex(uint32_t index, uint32_t fieldSize);
        void TrackGap(uint32_t offset, uint32_t size);

    private:
        Declaration& m_decl;
        Vector<FieldRef> m_sortedFields{};

        uint16_t m_dataFieldCount{ 0 };
        uint32_t m_dataOffset{ 0 };
        uint32_t m_gaps[6]{}; // Each gap index represents an offset to a gap of size (2^index) bits.
    };
}
