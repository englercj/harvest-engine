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
            return a.field.Meta().Normal().Ordinal() < b.field.Meta().Normal().Ordinal();
        }
    };

    static void CollectFields(const Declaration::Builder& decl, Vector<StructLayout::FieldRef>& out)
    {
        Declaration::Data::Struct::Builder st = decl.Data().Struct();
        List<Field>::Builder fields = st.Fields();

        out.Reserve(out.Size() + fields.Size());

        for (Field::Builder field : st.Fields())
        {
            if (!field.Meta().IsNormal())
                continue;

            out.PushBack({ field, decl });
        }

        for (Declaration::Builder child : decl.Children())
        {
            Declaration::Data::Builder data = child.Data();
            if (data.IsStruct() && (data.Struct().IsGroup() || data.Struct().IsUnion()))
                CollectFields(child, out);
        }
    }

    void StructLayout::CollectSortedFields(const Declaration::Builder& decl, Vector<FieldRef>& out)
    {
        CollectFields(decl, out);
        std::sort(out.begin(), out.end(), FieldRefOrdinalSort{});
    }

    StructLayout::StructLayout(Declaration::Builder decl)
        : m_decl(decl)
        , m_struct(decl.Data().Struct())
    {
        StructLayout::CollectSortedFields(m_decl, m_sortedFields);
    }

    void StructLayout::CalculateLayout()
    {
        // TODO: set Union tag values of Normal/Group/Union fields
        // TODO: Set index values of Normal fields
        // TODO: Finish logic for dataOffset of Normal fields

        for (const FieldRef& ref : m_sortedFields)
        {
            PlaceField(ref);
        }

        m_dataOffset += MetadataWordSize() * BitsPerWord;
        while (!IsAligned(m_dataOffset, BitsPerWord))
            ++m_dataOffset;

        const uint32_t wordSize = m_dataOffset / BitsPerWord;
        HE_ASSERT(wordSize < std::numeric_limits<uint16_t>::max());
        m_struct.SetDataWordSize(static_cast<uint16_t>(wordSize));
    }

    void StructLayout::PlaceField(const FieldRef& ref)
    {
        Field::Builder field = ref.field;
        Field::Meta::Normal::Builder f = field.Meta().Normal();

        // For void we store nothing so it always has a zero offset
        if (field.Type().Data().IsVoid())
        {
            f.SetIndex(0);
            f.SetDataOffset(0);
            return;
        }

        // For data fields we use the size of the field to try and optimally pack it in.
        // Generally speaking the field's size and alignment are the same, with the one
        // exception being array types. Their size is (element size * array size) and the
        // alignment is just (element size).
        const uint32_t fieldSize = GetTypeSize(field.Type());
        const uint32_t fieldAlign = GetTypeAlign(field.Type());

        // If this is not the first field in the union, try to find space for it by
        // overlapping in the space allocated for a previous union field.
        Declaration::Data::Struct::Builder parent = ref.parent.Data().Struct();
        if (parent.IsUnion() && f.Ordinal() > 0)
        {
            if (TryPlaceUnionField(ref, fieldSize, fieldAlign))
                return;
        }

        // For pointer fields simply store the index of the next pointer section slot.
        if (IsPointer(field.Type().Data().Tag()))
        {
            uint16_t pointerCount = m_struct.PointerCount();
            if (field.Type().Data().Tag() == Type::Data::Tag::Array)
            {
                field.Meta().Normal().SetIndex(pointerCount);
                m_struct.SetPointerCount(pointerCount + field.Type().Data().Array().Size());
            }
            else
            {
                field.Meta().Normal().SetIndex(pointerCount + 1);
            }

            return;
        }

        field.Meta().Normal().SetIndex(m_dataFieldCount++);
        field.Meta().Normal().SetDataOffset(PlaceDataField(fieldSize, fieldAlign));
    }

    bool StructLayout::TryPlaceUnionField(const FieldRef& ref, uint32_t fieldSize, uint32_t fieldAlign)
    {
        HE_ASSERT(ref.parent.Data().IsStruct() && ref.parent.Data().Struct().IsUnion());

        Field::Builder field = ref.field;
        Field::Meta::Normal::Builder f = field.Meta().Normal();

        // Before placing the second field of a union we allocate the tag value's space.
        if (f.Ordinal() == 1)
        {
            // Allocate 16 bits of space for the union tag
            ref.parent.Data().Struct().SetUnionTagOffset(PlaceDataField(16, 16));
        }

        if (m_activeParent != &ref.parent)
        {
            CollectSortedFields(ref.parent, m_sortedParentFields);
            m_activeParent = &ref.parent;
        }

        // Try to overlap a pointer field.
        if (IsPointer(field.Type()))
        {
            for (const FieldRef& r : m_sortedParentFields)
            {
                Field::Meta::Reader meta = r.field.Meta();

                if (!meta.IsNormal())
                    continue;

                if (meta.Normal().Ordinal() >= field.Meta().Normal().Ordinal())
                    break;

                // Can only overlap if they are both pointer fields
                if (!IsPointer(r.field.Type()))
                    continue;

                field.Meta().Normal().SetIndex(meta.Normal().Index());
                return true;
            }

            return false;
        }

        // Try to overlap a data field.
        for (const FieldRef& r : m_sortedParentFields)
        {
            Field::Meta::Reader meta = r.field.Meta();

            if (!meta.IsNormal())
                continue;

            if (meta.Normal().Ordinal() >= field.Meta().Normal().Ordinal())
                break;

            // Can only overlap if they are both data fields, and the offset is aligned
            const uint32_t prevFieldSize = GetTypeSize(r.field.Type());
            const uint32_t offset = meta.Normal().DataOffset() * prevFieldSize;
            if (IsPointer(r.field.Type()) || !IsAligned(offset, fieldAlign))
                continue;

            // If this field is smaller then trivially it can fit.
            if (fieldSize <= prevFieldSize)
            {
                field.Meta().Normal().SetDataOffset(offset / fieldSize);
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
                        field.Meta().Normal().SetDataOffset(offset / fieldSize);
                        return true;
                    }
                }
            }

            // Last thing to try is to detect if we can just 'extend' our size to include the
            // larger field. We can do this if the last field allocated was the field we're
            // testing now.
            if (m_dataOffset == prevFieldEnd)
            {
                field.Meta().Normal().SetDataOffset(offset / fieldSize);
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

    uint32_t StructLayout::MetadataWordSize() const
    {
        if (m_dataFieldCount <= 32) [[likely]]
            return 1;

        return 1 + (((m_dataFieldCount - 32) + (BitsPerWord - 1)) / BitsPerWord);
    }
}
