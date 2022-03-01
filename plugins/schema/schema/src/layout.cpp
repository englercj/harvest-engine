// Copyright Chad Engler

#include "he/schema/layout.h"

#include "he/core/macros.h"

namespace he::schema
{
    static constexpr uint32_t BitsPerElementSize[] =
    {
        0,  // Void
        1,  // Bit
        8,  // Byte
        16, // TwoBytes
        32, // FourBytes
        64, // EightBytes
        64,  // Pointer
        0,  // Composite
    };
    static_assert(HE_LENGTH_OF(BitsPerElementSize) == static_cast<uint32_t>(ElementSize::_Count));

    ListReader PointerReader::TryGetList(ElementSize expectedElementSize, const Word* defaultValue) const
    {
        PointerReader ref(*this);

        if (ref.IsNull())
        {
        useDefault:
            ref = PointerReader(defaultValue);
            defaultValue = nullptr;

            if (ref.IsNull())
                return ListReader();
        }

        // TODO: Cycle checks.

        if (!HE_VERIFY(ref.Kind() == PointerKind::List, "Expected List pointer, but got a {} pointer.", ref.Kind()))
            goto useDefault;

        const ElementSize elementSize = ref.ListElementSize();

        if (!HE_VERIFY(expectedElementSize == elementSize, "Expected list of {} elements, but got a list of {} elements.", expectedElementSize, elementSize))
            goto useDefault;

        if (elementSize == ElementSize::Composite)
        {
            const uint32_t wordCount = ref.ListSize();
            PointerReader tag = PointerReader(ref.Target());

            // TODO: Bounds checking

            if (!HE_VERIFY(tag.Kind() == PointerKind::Struct, "Composite lists of non-struct types are not supported."))
                goto useDefault;

            const uint32_t size = tag.ListCompositeTagSize();
            const uint32_t wordsPerElement = tag.StructWordSize();

            if (!HE_VERIFY(size * wordsPerElement <= wordCount, "Composite list's elements overrun its word count."))
                goto useDefault;

            // TODO: Check for a broken buffer that reports a huge list size with no real data
            // if (wordsPerElement == 0)
            // {
            //     if (!HE_VERIFY(CheckWeCanReadSizeFromBuffer(size), "List pointer has an unreasonably large size"))
            //         goto useDefault;
            // }

            return ListReader(
                tag.m_data + 1,
                size,
                wordsPerElement * BitsPerWord,
                ElementSize::Composite);
        }

        // TODO: Check for a broken buffer that reports a huge list size with no real data
        // if (elementSize == ElementSize::Void)
        // {
        //     if (!HE_VERIFY(CheckWeCanReadSizeFromBuffer(size), "List pointer has an unreasonably large size"))
        //         goto useDefault;
        // }

        HE_ASSERT(elementSize < ElementSize::_Count);
        const uint32_t elementBitSize = BitsPerElementSize[static_cast<uint16_t>(elementSize)];

        return ListReader(ref.Target(), ref.ListSize(), elementBitSize, elementSize);
    }

    StructReader PointerReader::TryGetStruct(const Word* defaultValue) const
    {
        PointerReader ref(*this);

        if (ref.IsNull())
        {
        useDefault:
            ref = PointerReader(defaultValue);
            defaultValue = nullptr;

            if (ref.IsNull())
                return StructReader();
        }

        // TODO: Cycle checks.

        if (!HE_VERIFY(ref.Kind() == PointerKind::Struct, "Expected Struct pointer, but got a {} pointer.", ref.Kind()))
            goto useDefault;

        // TODO: Bounds checking

        return StructReader(ref.Target(), ref.StructDataWordSize(), ref.StructPointerCount());
    }

    uint16_t ListReader::StructDataFieldCount() const
    {
        if (m_elementSize != ElementSize::Composite || Size() == 0)
            return 0;

        return GetCompositeElement(0).DataFieldCount();
    }

    StructReader ListReader::GetCompositeElement(uint32_t index) const
    {
        HE_ASSERT(IsValid());
        HE_ASSERT(index < Size());
        HE_ASSERT(m_elementSize == ElementSize::Composite);
        HE_ASSERT(IsAligned(m_step, BitsPerWord));
        const Word* element = m_data + (index * (m_step / BitsPerWord));
        return StructReader(element, StructDataWordSize(), StructPointerCount());
    }

    PointerReader ListReader::GetPointerElement(uint32_t index) const
    {
        HE_ASSERT(IsValid());
        HE_ASSERT(index < Size());
        HE_ASSERT(m_elementSize == ElementSize::Pointer);
        return PointerReader(m_data + index);
    }

    ListReader StructReader::TryGetPointerArrayField(uint16_t index, uint16_t elementCount, const Word* defaultValue) const
    {
        HE_ASSERT(IsValid());
        HE_ASSERT(elementCount > 0);

        if ((index + elementCount) > m_pointerCount) [[unlikely]]
            return PointerReader().TryGetList(ElementSize::Pointer, defaultValue);

        constexpr uint16_t ElementSizeIndex = static_cast<uint16_t>(ElementSize::Pointer);
        constexpr uint32_t elementBitSize = BitsPerElementSize[ElementSizeIndex];

        return ListReader(
            PointerFields() + index,
            elementCount,
            elementBitSize,
            ElementSize::Pointer);
    }

    void Builder::SetRoot(const StructReader& root)
    {
        PointerBuilder(this, 0).Set(root);
    }

    void Builder::SetRoot(const ListReader& root)
    {
        PointerBuilder(this, 0).Set(root);
    }

    ListBuilder Builder::AddList(ElementSize elementSize, uint32_t elementCount)
    {
        HE_ASSERT(elementSize < ElementSize::_Count);
        HE_ASSERT(elementSize != ElementSize::Composite);

        if (elementCount == 0)
            return ListBuilder();

        const uint32_t elementBitSize = BitsPerElementSize[static_cast<uint16_t>(elementSize)];

        const uint64_t bitSize = static_cast<uint64_t>(elementBitSize) * elementCount;
        const uint32_t wordSize =  static_cast<uint32_t>((bitSize + (BitsPerWord - 1)) / BitsPerWord);
        const uint32_t wordOffset = m_data.Size();
        m_data.Expand(wordSize);
        return ListBuilder(this, wordOffset, elementCount, elementBitSize, elementSize);
    }

    ListBuilder Builder::AddStructList(uint32_t elementCount, uint16_t dataFieldCount, uint16_t dataWordSize, uint16_t pointerCount)
    {
        if (elementCount == 0)
            return ListBuilder();

        // TODO: Check for overflow in `structWordSize * elementCount`.
        const uint32_t structWordSize = static_cast<uint32_t>(dataWordSize) + pointerCount;
        const uint32_t wordSize = 1 + (structWordSize * elementCount); // +1 for the tag value
        const uint32_t wordOffset = m_data.Size();
        m_data.Expand(wordSize);

        ListBuilder list(this, wordOffset + 1, elementCount, structWordSize * BitsPerWord, dataFieldCount);

        PointerBuilder tag = list.Tag();
        tag.SetListCompositeTagSize(elementCount);
        tag.SetStructDataWordSize(dataWordSize);
        tag.SetStructPointerCount(pointerCount);

        // Each get of the composite element constructs a StructBuilder which initializes the
        // field count value in each struct's field metadata.
        for (uint32_t i = 0; i < elementCount; ++i)
            list.GetCompositeElement(i);

        return list;
    }

    String::Builder Builder::AddString(StringView str)
    {
        if (str.IsEmpty())
            return {};

        ListBuilder list = AddList(ElementSize::Byte, str.Size() + 1);
        MemCopy(list.Data(), str.Data(), str.Size());
        return String::Builder(list);
    }

    StructBuilder Builder::AddStruct(uint16_t dataFieldCount, uint16_t dataWordSize, uint16_t pointerCount)
    {
        const uint32_t wordSize = static_cast<uint32_t>(dataWordSize) + pointerCount;
        const uint32_t wordOffset = m_data.Size();
        m_data.Expand(wordSize);
        return StructBuilder(this, wordOffset, dataFieldCount, dataWordSize, pointerCount);
    }

    void PointerBuilder::Set(const StructReader& value)
    {
        if (!value.IsValid())
        {
            SetNull();
            return;
        }

        HE_ASSERT(value.Data() >= m_builder->Data() && value.Data() <= m_builder->Data() + m_builder->Size());

        if ((value.DataWordSize() + value.PointerCount()) == 0)
        {
            SetZeroStruct();
            return;
        }

        SetTargetAndKind(value.Data(), PointerKind::Struct);
        SetStructDataWordSize(value.DataWordSize());
        SetStructPointerCount(value.PointerCount());
    }

    void PointerBuilder::Set(const ListReader& value)
    {
        if (!value.IsValid())
        {
            SetNull();
            return;
        }

        HE_ASSERT(value.Data() >= m_builder->Data() && value.Data() <= m_builder->Data() + m_builder->Size());

        const Word* target = value.Data();
        uint32_t listSize = value.Size();

        if (value.ElementSize() == ElementSize::Composite)
        {
            // point to the tag before the list values
            target -= 1;

            // size of the list for a composite list is the word size, not the element count
            const uint32_t structWordSize = value.StructDataWordSize() + value.StructPointerCount();
            listSize = structWordSize * value.Size();
        }

        HE_ASSERT(listSize <= 0x1fffffff);
        SetTargetAndKind(target, PointerKind::List);
        SetList(value.ElementSize(), listSize);
    }

    void PointerBuilder::Copy(const PointerReader& reader)
    {
        if (reader.IsNull())
        {
            SetNull();
            return;
        }

        switch (reader.Kind())
        {
            case PointerKind::List:
            {
                ListReader srcList = reader.TryGetList(reader.ListElementSize());
                ListBuilder dstList;
                if (srcList.ElementSize() == ElementSize::Composite)
                    dstList = m_builder->AddStructList(srcList.Size(), srcList.StructDataFieldCount(), srcList.StructDataWordSize(), srcList.StructPointerCount());
                else
                    dstList = m_builder->AddList(srcList.ElementSize(), srcList.Size());
                dstList.Copy(srcList);
                Set(dstList);
                break;
            }
            case PointerKind::Struct:
            {
                StructReader srcStruct = reader.TryGetStruct();
                StructBuilder dstStruct = m_builder->AddStruct(srcStruct.DataFieldCount(), srcStruct.DataWordSize(), srcStruct.PointerCount());
                dstStruct.Copy(srcStruct);
                Set(dstStruct);
                break;
            }
            case PointerKind::_Count:
                HE_ASSERT(false, "Encountered invalid pointer kind");
                break;
        }
    }

    ListBuilder PointerBuilder::TryGetList(ElementSize expectedElementSize) const
    {
        ListReader reader = AsReader().TryGetList(expectedElementSize);
        if (!reader.IsValid())
            return ListBuilder();

        HE_ASSERT(reader.Data() >= m_builder->Data() && reader.Data() < m_builder->Data() + m_builder->Size());
        const uint32_t wordOffset = static_cast<uint32_t>(reader.Data() - m_builder->Data());

        if (reader.ElementSize() == ElementSize::Composite)
            return ListBuilder(m_builder, wordOffset, reader.Size(), reader.StepSize(), reader.StructDataFieldCount());

        return ListBuilder(m_builder, wordOffset, reader.Size(), reader.StepSize(), reader.ElementSize());
    }

    StructBuilder PointerBuilder::TryGetStruct() const
    {
        StructReader reader = AsReader().TryGetStruct();
        if (!reader.IsValid())
            return StructBuilder();

        HE_ASSERT(reader.Data() >= m_builder->Data() && reader.Data() < m_builder->Data() + m_builder->Size());
        const uint32_t wordOffset = static_cast<uint32_t>(reader.Data() - m_builder->Data());
        return StructBuilder(m_builder, wordOffset, reader.DataFieldCount(), reader.DataWordSize(), reader.PointerCount());
    }

    void ListBuilder::Copy(const ListReader& reader)
    {
        HE_ASSERT(m_size == reader.Size());
        HE_ASSERT(m_step == reader.StepSize());
        HE_ASSERT(m_elementSize == reader.ElementSize());

        if (m_elementSize == ElementSize::Composite)
        {
            HE_ASSERT(StructDataWordSize() == reader.StructDataWordSize());
            HE_ASSERT(StructPointerCount() == reader.StructPointerCount());

            for (uint32_t i = 0; i < m_size; ++i)
            {
                StructReader src = reader.GetCompositeElement(i);
                StructBuilder dst = GetCompositeElement(i);
                dst.Copy(src);
            }
        }
        else if (m_elementSize == ElementSize::Pointer)
        {
            for (uint32_t i = 0; i < m_size; ++i)
            {
                PointerReader src = reader.GetPointerElement(i);
                PointerBuilder dst = GetPointerElement(i);
                dst.Copy(src);
            }
        }
        else
        {
            const uint64_t byteSize = (static_cast<uint64_t>(reader.Size()) * reader.StepSize()) / BitsPerByte;
            //  TODO: Confirm that byteSize is reasonable
            MemCopy(Data(), reader.Data(), byteSize);

            const uint64_t leftoverBits = (static_cast<uint64_t>(reader.Size()) * reader.StepSize()) % BitsPerByte;
            if (leftoverBits > 0)
            {
                const uint8_t mask = (1 << leftoverBits) - 1;
                const uint8_t* readerData = reinterpret_cast<const uint8_t*>(reader.Data());
                uint8_t* data = reinterpret_cast<uint8_t*>(Data());
                *(data + byteSize) = mask & *(readerData + byteSize);
            }
        }
    }

    StructBuilder ListBuilder::GetCompositeElement(uint32_t index) const
    {
        HE_ASSERT(m_builder);
        HE_ASSERT(m_elementSize == ElementSize::Composite);
        HE_ASSERT(IsAligned(m_step, BitsPerWord));
        const uint32_t wordOffset = m_wordOffset + (index * (m_step / BitsPerWord));
        return StructBuilder(m_builder, wordOffset, StructDataFieldCount(), StructDataWordSize(), StructPointerCount());
    }

    void StructBuilder::Copy(const StructReader& reader)
    {
        HE_ASSERT(m_dataFieldCount == reader.DataFieldCount());
        HE_ASSERT(m_dataWordSize == reader.DataWordSize());
        HE_ASSERT(m_pointerCount == reader.PointerCount());

        MemCopy(DataSection(), reader.Data(), DataWordSize() * BytesPerWord);

        for (uint16_t i = 0; i < reader.PointerCount(); ++i)
        {
            PointerReader src = reader.GetPointerField(i);
            PointerBuilder dst = GetPointerField(i);
            dst.Copy(src);
        }
    }

    ListBuilder StructBuilder::GetPointerArrayField(uint16_t index, uint16_t elementCount) const
    {
        HE_ASSERT(IsValid());
        HE_ASSERT(elementCount > 0);

        if ((index + elementCount) > m_pointerCount) [[unlikely]]
            return ListBuilder();

        constexpr uint16_t ElementSizeIndex = static_cast<uint16_t>(ElementSize::Pointer);
        constexpr uint32_t ElementBitSize = BitsPerElementSize[ElementSizeIndex];

        return ListBuilder(
            m_builder,
            m_wordOffset + DataWordSize() + index,
            elementCount,
            ElementBitSize,
            ElementSize::Pointer);
    }
}
