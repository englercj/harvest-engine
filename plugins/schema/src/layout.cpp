// Copyright Chad Engler

#include "he/schema/layout.h"

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

    static void WriteStructPointer(Word* ptr, const StructBuilder& value)
    {
        if ((value.DataWordSize() + value.PointerCount()) == 0)
        {
            // Zero sized pointer which is -1 offset with 0 for the kind
            *ptr = 0xfffffffc;
        }
        else
        {
            const Word* target = value.Location();
            *ptr = (static_cast<uint32_t>(target - ptr - 1) << 2) | static_cast<uint32_t>(PointerKind::Struct);
        }

        uint16_t* p = reinterpret_cast<uint16_t*>(ptr);
        p[2] = value.DataWordSize();
        p[3] = value.PointerCount();
    }

    static void WriteListPointer(Word* ptr, const ListBuilder& value)
    {
        uint32_t* p = reinterpret_cast<uint32_t*>(ptr);

        const Word* target = value.Location();
        p[0] = (static_cast<uint32_t>(target - ptr - 1) << 2) | static_cast<uint32_t>(PointerKind::List);

        HE_ASSERT(value.Size() <= 0x8fffffff);
        p[1] = (value.Size() << 3) | static_cast<uint32_t>(value.ElementSize());
    }

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

            const uint32_t size = tag.ListCompositeSize();
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
                tag.StructDataWordSize(),
                tag.StructPointerCount(),
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

        return ListReader(ref.Target(), ref.ListSize(), elementBitSize, 0, 0, elementSize);
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

    ListReader StructReader::TryGetPointerArrayField(
        uint16_t index,
        ElementSize elementSize,
        uint16_t elementCount,
        uint16_t structDataWordSize,
        uint16_t structPointerCount,
        const Word* defaultValue) const
    {
        if ((index + elementCount) > m_pointerCount) [[unlikely]]
            return PointerReader().TryGetList(elementSize, defaultValue);

        HE_ASSERT(elementSize < ElementSize::_Count);
        const uint32_t elementBitSize = BitsPerElementSize[static_cast<uint16_t>(elementSize)];

        return ListReader(
            PointerFields() + index,
            elementCount,
            elementBitSize,
            structDataWordSize,
            structPointerCount,
            elementSize);
    }

    void Builder::SetRoot(const StructBuilder& root)
    {
        WriteStructPointer(m_data.Data(), root);
    }

    ListBuilder Builder::AddList(ElementSize elementSize, uint32_t elementCount)
    {
        HE_ASSERT(elementSize < ElementSize::_Count);
        const uint32_t elementBitSize = BitsPerElementSize[static_cast<uint16_t>(elementSize)];

        const uint64_t bitSize = static_cast<uint64_t>(elementBitSize) * elementCount;
        const uint32_t wordSize =  static_cast<uint32_t>((bitSize + (BitsPerWord - 1)) / BitsPerWord);
        const uint32_t wordOffset = m_data.Size();
        m_data.Expand(wordSize);
        return ListBuilder(*this, wordOffset, elementCount, elementBitSize, 0, 0, elementSize);
    }

    ListBuilder Builder::AddStructList(uint32_t elementCount, uint16_t dataWordSize, uint16_t pointerCount)
    {
        const uint32_t structWordSize = static_cast<uint32_t>(dataWordSize) + pointerCount;
        const uint32_t wordSize = structWordSize * elementCount;
        const uint32_t wordOffset = m_data.Size();
        m_data.Expand(wordSize);
        return ListBuilder(*this, wordOffset, elementCount, structWordSize * BitsPerWord, dataWordSize, pointerCount, ElementSize::Composite);
    }

    StructBuilder Builder::AddStruct(uint16_t dataFieldCount, uint16_t dataWordSize, uint16_t pointerCount)
    {
        const uint32_t wordSize = static_cast<uint32_t>(dataWordSize) + pointerCount;
        const uint32_t wordOffset = m_data.Size();
        m_data.Expand(wordSize);
        return StructBuilder(*this, wordOffset, dataFieldCount, dataWordSize, pointerCount);
    }

    ListBuilder PointerBuilder::TryGetList(ElementSize expectedElementSize, const Word* defaultValue) const
    {
        ListReader reader = AsReader().TryGetList(expectedElementSize, defaultValue);
        const uint32_t wordOffset = 0; // TODO: What if the default value is used? Is that even safe? If they set a value on it...explode!
        return ListBuilder(m_builder, wordOffset, reader.Size(), reader.StepSize(), reader.StructDataWordSize(), reader.StructPointerCount(), reader.ElementSize());
    }

    StructBuilder PointerBuilder::TryGetStruct(const Word* defaultValue) const
    {
        // TODO: implement
    }

    void StructBuilder::SetPointerField(uint16_t index, const StructBuilder& value)
    {
        HE_ASSERT(&value.Builder() == &m_builder);
        Word* ptr = PointerSection() + index;
        WriteStructPointer(ptr, value);
    }

    void StructBuilder::SetPointerField(uint16_t index, const ListBuilder& value)
    {
        HE_ASSERT(&value.Builder() == &m_builder);
        Word* ptr = PointerSection() + index;
        WriteListPointer(ptr, value);
    }
}
