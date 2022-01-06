// Copyright Chad Engler

#include "struct_layout.h"

#include "he/core/assert.h"

#include <algorithm>
#include <limits>

namespace he::schema
{
    struct FieldRefOrdinalSort
    {
        bool operator()(const StructLayout::FieldRef& a, const StructLayout::FieldRef& b) const
        {
            return a.field->ordinal < b.field->ordinal;
        }
    };

    static void CollectFields(Declaration& decl, Vector<StructLayout::FieldRef>& out)
    {
        HE_ASSERT(decl.kind == DeclKind::Struct);

        out.Reserve(out.Size() + decl.struct_.fields.Size());

        for (Field& field : decl.struct_.fields)
        {
            if (field.isGroup || field.isUnion)
                continue;

            out.PushBack({ &field, &decl });
        }

        for (Declaration& child : decl.children)
        {
            if (child.kind == DeclKind::Struct && (child.struct_.isGroup || child.struct_.isUnion))
                CollectFields(child, out);
        }
    }

    void StructLayout::CollectSortedFields(Declaration& decl, Vector<FieldRef>& out)
    {
        CollectFields(decl, out);
        std::sort(out.begin(), out.end(), FieldRefOrdinalSort{});
    }

    StructLayout::StructLayout(Declaration& decl)
        : m_decl(decl)
    {
        StructLayout::CollectSortedFields(m_decl, m_sortedFields);
    }

    void StructLayout::CalculateLayout()
    {
        for (const FieldRef& ref : m_sortedFields)
        {
            PlaceField(ref);
        }

        while (!IsAligned(m_dataOffset, 8u))
            ++m_dataOffset;

        const uint32_t wordSize = m_dataOffset / 64;
        HE_ASSERT(wordSize < std::numeric_limits<uint16_t>::max());
        m_decl.struct_.dataWordSize = static_cast<uint16_t>(wordSize);
    }

    void StructLayout::PlaceField(const FieldRef& ref)
    {
        Field* field = ref.field;

        // For void we store nothing so it always has a zero offset
        if (field->type.kind == TypeKind::Void)
        {
            field->index = 0;
            field->dataOffset = 0;
            return;
        }

        // For data fields we use the size of the field to try and optimally pack it in.
        // Generally speaking the field's size and alignment are the same, with the one
        // exception being array types. Their size is (element size * array size) and the
        // alignment is just (element size).
        const uint32_t fieldSize = GetTypeSize(field->type);
        const uint32_t fieldAlign = GetTypeAlign(field->type);

        // If this is not the first field in the union, try to find space for it by
        // overlapping in the space allocated for a previous union field.
        if (ref.parent->struct_.isUnion && field > ref.parent->struct_.fields.Data())
        {
            if (TryPlaceUnionField(ref, fieldSize, fieldAlign))
                return;
        }

        // For pointer fields simply store the index of the next pointer section slot.
        if (IsPointer(field->type))
        {
            if (field->type.kind == TypeKind::Array)
            {
                field->index = m_decl.struct_.pointerCount;
                m_decl.struct_.pointerCount += field->type.array_.size;
            }
            else
            {
                field->index = m_decl.struct_.pointerCount++;
            }

            return;
        }

        field->index = m_dataFieldCount++;
        field->dataOffset = PlaceDataField(fieldSize, fieldAlign);
    }

    bool StructLayout::TryPlaceUnionField(const FieldRef& ref, uint32_t fieldSize, uint32_t fieldAlign)
    {
        Field* field = ref.field;
        const bool fieldIsPointer = IsPointer(field->type);

        // Before placing the second field of a union we allocate the tag value's space.
        const bool isSecondField = field == &ref.parent->struct_.fields[1];
        if (isSecondField)
        {
            // Allocate 16 bits of space for the union tag
            ref.parent->struct_.unionTagOffset = PlaceDataField(16, 16);
        }

        if (m_activeParent != ref.parent)
        {
            CollectSortedFields(*ref.parent, m_sortedParentFields);
            m_activeParent = ref.parent;
        }

        // Try to overlap a pointer field.
        if (fieldIsPointer)
        {
            for (const FieldRef& f : m_sortedParentFields)
            {
                if (f.field->isGroup || f.field->isUnion)
                    continue;

                if (f.field->ordinal >= field->ordinal)
                    break;

                // Can only overlap if they are both pointer fields
                if (!IsPointer(f.field->type))
                    continue;

                field->index = f.field->index;
                return true;
            }

            return false;
        }

        // Try to overlap a data field.
        for (const FieldRef& f : m_sortedParentFields)
        {
            if (f.field->isGroup || f.field->isUnion)
                continue;

            if (f.field->ordinal >= field->ordinal)
                break;

            // Can only overlap if they are both data fields, and the offset is aligned
            const uint32_t prevFieldSize = GetTypeSize(f.field->type);
            const uint32_t offset = f.field->dataOffset * prevFieldSize;
            if (IsPointer(f.field->type) || !IsAligned(offset, fieldAlign))
                continue;

            // If this field is smaller then trivially it can fit.
            if (fieldSize <= prevFieldSize)
            {
                field->dataOffset = offset / fieldSize;
                return true;
            }

            // Check for any gap after the field that may mean we have enough space to fit
            // the field we're trying to place.
            const uint32_t prevFieldEnd = offset + prevFieldSize;
            const uint32_t requiredGapSpace = fieldSize - prevFieldSize;

            for (uint32_t i = 0; i < HE_LENGTH_OF(m_gaps); ++i)
            {
                if (m_gaps[i] == prevFieldEnd)
                {
                    const uint32_t gapOffset = TryClaimGapIndex(i, requiredGapSpace);
                    if (gapOffset != 0)
                    {
                        HE_ASSERT(gapOffset == prevFieldEnd);
                        field->dataOffset = offset / fieldSize;
                        return true;
                    }
                }
            }

            // Last thing to try is to detect if we can just 'extend' our size to include the
            // larger field. We can do this if the last field allocated was the field we're
            // testing now.
            if (m_dataOffset == prevFieldEnd)
            {
                field->dataOffset = offset / fieldSize;
                m_dataOffset += requiredGapSpace;
                return true;
            }
        }

        return false;
    }

    uint32_t StructLayout::PlaceDataField(uint32_t fieldSize, uint32_t fieldAlign)
    {
        // First try to fit the field in a gap we left behind with padding.
        const uint32_t gapOffset = TryClaimGap(fieldSize);
        if (gapOffset != 0)
        {
            HE_ASSERT(IsAligned(gapOffset, fieldAlign));
            return gapOffset / fieldAlign;
        }

        // No available gaps, so we need to pad out to align for our field.
        uint32_t paddedOffset = m_dataOffset;
        while (!IsAligned(paddedOffset, fieldAlign))
            ++paddedOffset;

        const uint32_t paddingBits = paddedOffset - m_dataOffset;
        TrackGap(m_dataOffset, paddingBits);
        m_dataOffset = paddedOffset;

        // Place our field at the current data offset and advance.
        HE_ASSERT(IsAligned(m_dataOffset, fieldAlign));
        const uint32_t offset = m_dataOffset / fieldAlign;
        m_dataOffset += fieldSize;
        return offset;
    }

    uint32_t StructLayout::TryClaimGap(uint32_t fieldSize)
    {
        for (uint32_t i = 0; i < HE_LENGTH_OF(m_gaps); ++i)
        {
            const uint32_t offset = TryClaimGapIndex(i, fieldSize);
            if (offset != 0)
                return offset;
        }

        return 0;
    }

    uint32_t StructLayout::TryClaimGapIndex(uint32_t index, uint32_t fieldSize)
    {
        const uint32_t gapSize = (1 << index);
        if (m_gaps[index] != 0 && gapSize >= fieldSize)
        {
            const uint32_t offset = Exchange(m_gaps[index], 0);

            if (gapSize > fieldSize)
                TrackGap(offset + fieldSize, gapSize - fieldSize);

            return offset;
        }

        return 0;
    }

    void StructLayout::TrackGap(uint32_t offset, uint32_t size)
    {
        while (size > 0)
        {
            for (uint32_t i = 0; i < HE_LENGTH_OF(m_gaps); ++i)
            {
                const uint32_t gapIndex = HE_LENGTH_OF(m_gaps) - i - 1;
                const uint32_t gapSize = (1 << gapIndex);

                if ((size >> gapIndex) > 0 && IsAligned(offset, gapSize))
                {
                    HE_ASSERT(m_gaps[gapIndex] == 0);
                    m_gaps[gapIndex] = offset;
                    offset += gapSize;

                    HE_ASSERT(gapSize <= size);
                    size -= gapSize;
                    break;
                }
            }
        }

        HE_ASSERT(size == 0);
    }
}
