// Copyright Chad Engler

#include "struct_layout.h"

#include "he/core/assert.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace he::schema
{
    // --------------------------------------------------------------------------------------------
    // Helper class for manaing gaps due to alignment while we're laying out fields
    class GapSet
    {
    public:
        uint32_t FindGapIndex(uint32_t size)
        {
            for (uint32_t i = 0; i < GapArrayLen; ++i)
            {
                if (CanFit(i, size))
                    return i;
            }

            return ~0u;
        }

        uint32_t TryClaimGap(uint32_t size)
        {
            const uint32_t index = FindGapIndex(size);
            if (index != ~0u)
            {
                const uint32_t gapSize = (1 << index);
                const uint32_t offset = Exchange(m_gaps[index], 0);

                if (gapSize > size)
                    TrackGap(offset + size, gapSize - size);

                return offset;
            }

            return 0;
        }

        void TrackGap(uint32_t offset, uint32_t size)
        {
            while (size > 0)
            {
                for (uint32_t i = 0; i < GapArrayLen; ++i)
                {
                    const uint32_t gapIndex = GapArrayLen - i - 1;
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

        bool TryExpandGap(uint32_t size, uint32_t offset, uint32_t newSize)
        {
            if (newSize > MaxGapSize)
                return false;

            bool claims[GapArrayLen]{};

            for (uint32_t i = 0; size < newSize && i < GapArrayLen; ++i)
            {
                // If there is no gap at this index, continue searching
                if (m_gaps[i] == 0)
                    continue;

                // If the index contains a gap, but it isn't direcly after our offset, continue searching.
                if (m_gaps[i] != (offset + size))
                    continue;

                size += (1 << i);
                claims[i] = true;
            }

            if (size >= newSize)
            {
                for (uint32_t i = 0; i < GapArrayLen; ++i)
                {
                    if (claims[i])
                        m_gaps[i] = 0;
                }
                TrackGap(offset + newSize, size - newSize);
                return true;
            }

            return false;
        }

    private:
        bool CanFit(uint32_t index, uint32_t size)
        {
            const uint32_t gapSize = (1 << index);
            return m_gaps[index] != 0 && gapSize >= size;
        }

    private:
        static constexpr uint32_t GapArrayLen = 6;
        static constexpr uint32_t MaxGapSize = 1 << GapArrayLen;

        uint32_t m_gaps[GapArrayLen]{}; // Each gap index represents an offset to a gap of size (2^index) bits.
    };

    // --------------------------------------------------------------------------------------------
    // Interface for a class that can place fields into the layout
    class FieldPlacer
    {
    public:
        virtual void PlaceVoid() = 0;
        virtual uint32_t PlaceData(uint32_t size, uint32_t align) = 0;
        virtual uint16_t PlacePointer(uint16_t count) = 0;
        virtual bool TryExpandData(uint32_t size, uint32_t offset, uint32_t amount, uint32_t newAlign) = 0;
    };

    // --------------------------------------------------------------------------------------------
    // Class that manages placing fields in a struct
    class StructFieldPlacer final : public FieldPlacer
    {
    public:
        void PlaceVoid() override {}

        uint32_t PlaceData(uint32_t size, uint32_t align) override
        {
            // First try to fit the field in a gap we left behind with padding.
            const uint32_t gapOffset = m_gaps.TryClaimGap(size);
            if (gapOffset != 0)
            {
                HE_ASSERT(IsAligned(gapOffset, align));
                return gapOffset / align;
            }

            const uint32_t allocSize = AlignUp(size, BitsPerWord);
            const uint32_t offset = m_dataOffset;
            m_dataOffset += allocSize;
            m_gaps.TrackGap(offset + size, allocSize - size);
            HE_ASSERT(IsAligned(offset, align));
            return offset / align;

            //// No available gaps, so we need to pad out to align for our field.
            //HE_ASSERT(IsPowerOf2(align));
            //const uint32_t paddedOffset = AlignUp(m_dataOffset, align);
            //const uint32_t paddingBits = paddedOffset - m_dataOffset;
            //m_gaps.TrackGap(m_dataOffset, paddingBits);
            //m_dataOffset = paddedOffset;

            //// Place our field at the current data offset and advance.
            //HE_ASSERT(IsAligned(m_dataOffset, align));
            //const uint32_t offset = m_dataOffset / align;
            //m_dataOffset += size;
            //return offset;
        }

        uint16_t PlacePointer(uint16_t count) override
        {
            const uint16_t index = m_pointerCount;
            m_pointerCount += count;
            return index;
        }

        bool TryExpandData(uint32_t size, uint32_t offset, uint32_t newSize, uint32_t newAlign) override
        {
            HE_UNUSED(newAlign);
            return m_gaps.TryExpandGap(size, offset, newSize);
        }

        uint16_t PointerCount() const { return m_pointerCount; }
        uint32_t DataOffset() const { return m_dataOffset; }

    private:
        uint16_t m_pointerCount{ 0 };
        uint32_t m_dataOffset{ 0 };
        GapSet m_gaps{};
    };

    // --------------------------------------------------------------------------------------------
    // Class that manages placing fields in a union
    class UnionFieldPlacer final : public FieldPlacer
    {
    public:
        struct DataLocation
        {
            bool TryExpandTo(UnionFieldPlacer& union_, uint32_t newSize, uint32_t newAlign)
            {
                if (newSize <= size)
                    return true;

                if (IsAligned(offset, newAlign) && union_.m_parent.TryExpandData(size, offset, newSize, newAlign))
                {
                    size = newSize;
                    return true;
                }

                return false;
            }

            uint32_t offset;
            uint32_t size;
        };

    public:
        UnionFieldPlacer(FieldPlacer& parent) : m_parent(parent) {}
        UnionFieldPlacer(const UnionFieldPlacer&) = delete;
        UnionFieldPlacer& operator=(const UnionFieldPlacer&) = delete;

        void PlaceVoid() override
        {
            m_parent.PlaceVoid();
        }

        uint32_t PlaceData(uint32_t size, uint32_t align) override
        {
            const uint32_t offset = m_parent.PlaceData(size, align);
            m_dataLocations.PushBack({ offset * align, size });
            return offset;
        }

        uint16_t PlacePointer(uint16_t count) override
        {
            const uint16_t index = m_parent.PlacePointer(count);
            for (uint16_t i = 0; i < count; ++i)
            {
                m_pointerLocations.PushBack(index + i);
            }
            return index;
        }

        void FirstGroupMemberPlaced()
        {
            ++m_groupCount;
            if (m_groupCount == 2)
                PlaceTag();
        }

        bool TryExpandData(uint32_t size, uint32_t offset, uint32_t newSize, uint32_t newAlign) override
        {
            HE_UNUSED(size, offset, newSize, newAlign);
            return false;
        }

        Span<DataLocation> DataLocations() { return m_dataLocations; }
        Span<const DataLocation> DataLocations() const { return m_dataLocations; }

        Span<uint16_t> PointerLocations() { return m_pointerLocations; }
        Span<const uint16_t> PointerLocations() const { return m_pointerLocations; }

        uint32_t TagOffset() const { return m_tagOffset; }

        void PlaceTag()
        {
            if (m_tagOffset == ~0u)
            {
                m_tagOffset = m_parent.PlaceData(16, 16);
            }
        }

    private:
        FieldPlacer& m_parent;
        uint32_t m_groupCount{ 0 };
        uint32_t m_tagOffset{ ~0u };
        Vector<DataLocation> m_dataLocations{};
        Vector<uint16_t> m_pointerLocations{};
    };

    // --------------------------------------------------------------------------------------------
    // Class that manages placing fields in a group that lives inside a union.
    // This class is also used for naked fields in a union.
    class UnionGroupFieldPlacer final : public FieldPlacer
    {
    private:
        class DataLocationUsage
        {
        public:
            DataLocationUsage() = default;
            DataLocationUsage(uint32_t size) : m_usedSize(size) {}

            uint32_t FindSmallestGapForSize(const UnionFieldPlacer::DataLocation& loc, uint32_t size)
            {
                // Location is unused, we can treat the entire location as if it were one big gap.
                if (m_usedSize == 0)
                {
                    if (size <= loc.size)
                        return loc.size;

                    return ~0u;
                }

                // If the size is the same or larger than our used size then it clearly won't
                // fit in any gaps. If the size fits in the total size of the data location,
                // we can still treat this as a gap at the "end" of our usage.
                if (size >= m_usedSize)
                {
                    if ((m_usedSize + size) <= loc.size)
                        return size;

                    return ~0u;
                }

                // Check for a gap that can fit the size and return the size if we can fit it.
                const uint32_t gapIndex = m_gaps.FindGapIndex(size);
                if (gapIndex != ~0u)
                    return 1 << gapIndex;

                // Size is smaller than current usage, but there are no gaps available that it
                // can fit inside. If we have trailing space in the data location, we can use that.
                if ((m_usedSize + size) < loc.size)
                    return m_usedSize;

                return ~0u;
            }

            uint32_t PlaceInGap(const UnionFieldPlacer::DataLocation& loc, uint32_t size)
            {
                uint32_t offset = 0;

                // Location is unused, place at start
                if (m_usedSize == 0)
                {
                    HE_ASSERT(size <= loc.size);
                    offset = 0;
                    m_usedSize = size;
                }
                // Size is at least the used size, so it trivially cannot fit any gap in the
                // used size. However, it still fits in the location's data, so expand to
                // include it.
                else if (size >= m_usedSize)
                {
                    HE_ASSERT((m_usedSize + size) <= loc.size);
                    offset = m_usedSize;
                    m_usedSize += size;
                }
                else
                {
                    offset = m_gaps.TryClaimGap(size);
                    if (offset == 0)
                    {
                        HE_ASSERT((m_usedSize + size) <= loc.size);
                        offset = m_usedSize;
                        //m_gaps.TrackGap(offset + size, size);
                        m_usedSize += size;
                    }
                }

                return loc.offset + offset;
            }

            uint32_t TryExpandingToFit(UnionGroupFieldPlacer& group, UnionFieldPlacer::DataLocation& loc, uint32_t size, uint32_t align)
            {
                if (m_usedSize == 0)
                {
                    if (loc.TryExpandTo(group.m_parent, size, align))
                    {
                        m_usedSize = size;
                        return loc.offset;
                    }

                    return ~0u;
                }

                const uint32_t offset = AlignUp(m_usedSize, align);
                if (TryExpand(group, loc, offset + size, align))
                    return loc.offset + offset;

                return ~0u;
            }

            bool TryExpand(UnionGroupFieldPlacer& group, UnionFieldPlacer::DataLocation& loc, uint32_t size, uint32_t offset, uint32_t newSize, uint32_t newAlign)
            {
                if (offset == 0 && m_usedSize == size)
                    return TryExpand(group, loc, newSize, newAlign);

                return m_gaps.TryExpandGap(size, offset, newSize);
            }

        private:
            bool TryExpand(UnionGroupFieldPlacer& group, UnionFieldPlacer::DataLocation& loc, uint32_t newSize, uint32_t newAlign)
            {
                if (newSize > loc.size)
                {
                    if (!loc.TryExpandTo(group.m_parent, newSize, newAlign))
                        return false;
                }

                m_usedSize = newSize;
                return true;
            }

        private:
            uint32_t m_usedSize{ 0 };
            GapSet m_gaps{};
        };

    public:
        UnionGroupFieldPlacer(UnionFieldPlacer& parent) : m_parent(parent) {}
        UnionGroupFieldPlacer(const UnionGroupFieldPlacer&) = delete;
        UnionGroupFieldPlacer& operator=(const UnionGroupFieldPlacer&) = delete;

        void PlaceVoid() override
        {
            PlaceAnything();
            m_parent.PlaceVoid();
        }

        uint32_t PlaceData(uint32_t size, uint32_t align) override
        {
            PlaceAnything();

            Span<UnionFieldPlacer::DataLocation> locs = m_parent.DataLocations();

            uint32_t bestGapSize = ~0u;
            uint32_t bestGapIndex = ~0u;

            // Search for the smallest gap that the field can fit in
            for (uint32_t i = 0; i < locs.Size(); ++i)
            {
                const UnionFieldPlacer::DataLocation& loc = locs[i];

                if (m_dataLocationUsages.Size() == i)
                    m_dataLocationUsages.EmplaceBack();

                DataLocationUsage& usage = m_dataLocationUsages[i];
                const uint32_t gapSize = usage.FindSmallestGapForSize(loc, size);
                if (gapSize < bestGapSize)
                {
                    bestGapSize = gapSize;
                    bestGapIndex = i;
                }
            }

            // If we found a gap, claim the offset for it and place the field
            if (bestGapIndex != ~0u)
            {
                const uint32_t offset = m_dataLocationUsages[bestGapIndex].PlaceInGap(locs[bestGapIndex], size);
                HE_ASSERT(IsAligned(offset, align));
                return offset / align;
            }

            // No holes big enough, try to expand the size to fit
            for (uint32_t i = 0; i < locs.Size(); ++i)
            {
                DataLocationUsage& usage = m_dataLocationUsages[i];
                const uint32_t offset = usage.TryExpandingToFit(*this, locs[i], size, align);
                if (offset != ~0u)
                {
                    HE_ASSERT(IsAligned(offset, align));
                    return offset / align;
                }
            }

            // No space in existing locations
            m_dataLocationUsages.EmplaceBack(size);
            return m_parent.PlaceData(size, align);
        }

        uint16_t PlacePointer(uint16_t count) override
        {
            PlaceAnything();

            Span<const uint16_t> locs = m_parent.PointerLocations();
            if ((m_usedPointerLocations + count) <= locs.Size())
            {
                const uint32_t index = m_usedPointerLocations;
                m_usedPointerLocations += count;
                return locs[index];
            }

            m_usedPointerLocations += count;
            return m_parent.PlacePointer(count);
        }

        bool TryExpandData(uint32_t size, uint32_t offset, uint32_t newSize, uint32_t newAlign) override
        {
            if (newSize > BitsPerWord || !IsAligned(offset, newAlign))
                return false;

            Span<UnionFieldPlacer::DataLocation> locs = m_parent.DataLocations();
            for (uint32_t i = 0; i < m_dataLocationUsages.Size(); ++i)
            {
                UnionFieldPlacer::DataLocation& loc = locs[i];

                // The location we're trying to expand is a subset of `loc`.
                if (loc.size >= size && offset >= loc.offset && (offset + size) <= (loc.offset + loc.size))
                {
                    DataLocationUsage& usage = m_dataLocationUsages[i];
                    const uint32_t localOffset = offset - loc.offset;
                    return usage.TryExpand(*this, loc, size, localOffset, newSize, newAlign);
                }
            }

            HE_ASSERT(false, "Tried to expand field that was never placed.");
            return false;
        }

    private:
        void PlaceAnything()
        {
            if (!m_hasMembers)
            {
                m_hasMembers = true;
                m_parent.FirstGroupMemberPlaced();
            }
        }

    private:
        UnionFieldPlacer& m_parent;
        bool m_hasMembers{ false };
        Vector<DataLocationUsage> m_dataLocationUsages{};
        uint32_t m_usedPointerLocations{ 0 };
    };

    // --------------------------------------------------------------------------------------------

    StructLayout::StructLayout(Declaration::Builder decl)
        : m_decl(decl)
        , m_struct(decl.Data().Struct())
    {
        m_fieldPlacer = Allocator::GetDefault().New<StructFieldPlacer>();

        m_members.Reserve(m_decl.Children().Size() + m_struct.Fields().Size() + 1);
        m_members.PushBack({ {}, m_decl, m_fieldPlacer });
        CollectMembers(m_members.Back(), 0);
    }

    StructLayout::~StructLayout()
    {
        Allocator::GetDefault().Delete(m_fieldPlacer);
    }

    void StructLayout::CalculateLayout()
    {
        // Go through each normal field and set the offsets, indices, and union tags
        for (std::pair<uint16_t, uint32_t> pair : m_fields)
        {
            MemberRef& member = m_members[pair.second];
            HE_ASSERT(member.field.IsValid());

            const bool isInUnion = member.isInUnion;

            MemberRef* maybeUnionMember = &member;
            while (maybeUnionMember->parentIndex != 0 && maybeUnionMember->parentIndex != ~0u)
            {
                MemberRef& parent = m_members[maybeUnionMember->parentIndex];
                if (maybeUnionMember->isInUnion)
                {
                    maybeUnionMember->field.SetUnionTag(parent.unionTagCount++);
                    maybeUnionMember->isInUnion = false; // only consider this for setting the tag once
                }
                maybeUnionMember = &parent;
            }

            if (member.field.Meta().IsNormal())
            {
                const Type::Builder fieldType = member.field.Type();
                Field::Meta::Normal::Builder norm = member.field.Meta().Normal();
                const uint32_t fieldSize = GetTypeSize(fieldType);

                if (IsPointer(fieldType))
                {
                    HE_ASSERT((fieldSize / BitsPerWord) < std::numeric_limits<uint16_t>::max());
                    const uint16_t count = static_cast<uint16_t>(fieldSize / BitsPerWord);
                    norm.SetIndex(member.placer->PlacePointer(count));
                }
                else if (fieldType.Data().IsVoid())
                {
                    member.placer->PlaceVoid();
                    norm.SetDataOffset(0);
                    norm.SetIndex(0);
                }
                else
                {
                    const uint32_t fieldAlign = GetTypeAlign(fieldType);
                    norm.SetDataOffset(member.placer->PlaceData(fieldSize, fieldAlign));
                    norm.SetIndex(isInUnion ? 0 : m_dataFieldCount++);
                }
            }
        }

        // Now that all members are done placed, set the struct size values
        //m_struct.SetDataFieldCount(m_dataFieldCount);
        m_struct.SetDataWordSize(DataWordSize());
        m_struct.SetPointerCount(m_fieldPlacer->PointerCount());

        // Go through unions and groups and finish them off with the right offsets and sizes
        for (MemberRef& member : m_members)
        {
            if (member.decl.IsValid() && member.decl.Data().IsStruct())
            {
                Declaration::Data::Struct::Builder st = member.decl.Data().Struct();
                if (st.IsUnion())
                {
                    UnionFieldPlacer* unionPlacer = static_cast<UnionFieldPlacer*>(member.placer);
                    unionPlacer->PlaceTag();
                    st.SetUnionTagOffset(unionPlacer->TagOffset());
                }
                else if (st.IsGroup())
                {
                    //st.SetDataFieldCount(m_struct.DataFieldCount());
                    st.SetDataWordSize(m_struct.DataWordSize());
                    st.SetPointerCount(m_struct.PointerCount());
                }
            }
        }
    }

    void StructLayout::CollectMembers(MemberRef& parentRef, uint32_t parentIndex)
    {
        HE_ASSERT(parentRef.decl.IsValid() && parentRef.decl.Data().IsStruct());
        Declaration::Builder decl = parentRef.decl;
        Declaration::Data::Struct::Builder st = decl.Data().Struct();
        List<Field>::Builder fields = st.Fields();


        FieldPlacer* parentPlacer = parentRef.placer;

        std::unordered_map<TypeId, uint32_t> groups;

        for (Field::Builder field : st.Fields())
        {
            m_members.PushBack({ field, {}, parentPlacer, false, parentIndex });

            if (field.Meta().IsNormal())
            {
                const uint16_t ordinal = field.Meta().Normal().Ordinal();
                const uint32_t index = m_members.Size() - 1;
                auto res = m_fields.emplace(ordinal, index);
                HE_ASSERT(res.second);
                HE_UNUSED(res);
            }
            else if (field.Meta().IsGroup())
            {
                groups.emplace(field.Meta().Group().TypeId(), m_members.Size() - 1);
            }
            else
            {
                HE_ASSERT(field.Meta().IsUnion());
                groups.emplace(field.Meta().Union().TypeId(), m_members.Size() - 1);
            }
        }

        for (Declaration::Builder child : decl.Children())
        {
            auto it = groups.find(child.Id());
            if (it == groups.end())
                continue;

            MemberRef& fieldRef = m_members[it->second];
            fieldRef.decl = child;

            HE_ASSERT(child.Data().IsStruct());
            Declaration::Data::Struct::Builder childStruct = child.Data().Struct();

            if (childStruct.IsGroup())
            {
                CollectMembers(fieldRef, it->second);
            }
            else
            {
                HE_ASSERT(childStruct.IsUnion());
                fieldRef.placer = m_placers.PushBack(Allocator::GetDefault().New<UnionFieldPlacer>(*parentPlacer));
                CollectUnionMembers(fieldRef, it->second);
            }
        }
    }

    void StructLayout::CollectUnionMembers(MemberRef& parentRef, uint32_t parentIndex)
    {
        HE_ASSERT(parentRef.decl.IsValid() && parentRef.decl.Data().IsStruct());
        Declaration::Builder decl = parentRef.decl;
        Declaration::Data::Struct::Builder st = decl.Data().Struct();
        List<Field>::Builder fields = st.Fields();

        UnionFieldPlacer* unionPlacer = static_cast<UnionFieldPlacer*>(parentRef.placer);

        std::unordered_map<TypeId, uint32_t> groups;

        for (Field::Builder field : st.Fields())
        {
            m_placers.PushBack(Allocator::GetDefault().New<UnionGroupFieldPlacer>(*unionPlacer));
            m_members.PushBack({ field, {}, m_placers.Back(), true, parentIndex });
            const uint32_t index = m_members.Size() - 1;

            if (field.Meta().IsNormal())
            {
                const uint16_t ordinal = field.Meta().Normal().Ordinal();
                auto res = m_fields.emplace(ordinal, index);
                HE_ASSERT(res.second);
                HE_UNUSED(res);
            }
            else if (field.Meta().IsGroup())
            {
                groups.emplace(field.Meta().Group().TypeId(), index);
            }
            else
            {
                HE_ASSERT(field.Meta().IsUnion());
                groups.emplace(field.Meta().Union().TypeId(), index);
            }
        }

        for (Declaration::Builder child : decl.Children())
        {
            auto it = groups.find(child.Id());
            if (it == groups.end())
                continue;

            MemberRef& fieldRef = m_members[it->second];
            fieldRef.decl = child;

            HE_ASSERT(child.Data().IsStruct());
            Declaration::Data::Struct::Builder childStruct = child.Data().Struct();

            if (childStruct.IsGroup())
            {
                CollectMembers(fieldRef, it->second);
            }
            else
            {
                HE_ASSERT(childStruct.IsUnion());
                fieldRef.placer = m_placers.PushBack(Allocator::GetDefault().New<UnionFieldPlacer>(*fieldRef.placer));
                CollectUnionMembers(fieldRef, it->second);
            }
        }
    }

    uint16_t StructLayout::DataWordSize() const
    {
        const uint32_t bitSize = m_fieldPlacer->DataOffset() + (MetadataWordSize() * BitsPerWord);
        const uint32_t alignedBitSize = AlignUp(bitSize, BitsPerWord);
        const uint32_t wordSize = alignedBitSize / BitsPerWord;
        HE_ASSERT(wordSize < std::numeric_limits<uint16_t>::max());
        return static_cast<uint16_t>(wordSize);
    }

    uint32_t StructLayout::MetadataWordSize() const
    {
        if (m_fieldPlacer->DataOffset() == 0)
            return 0;

        if (m_dataFieldCount <= 32) [[likely]]
            return 1;

        return 1 + (((m_dataFieldCount - 32) + (BitsPerWord - 1)) / BitsPerWord);
    }
}
