// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/memory_ops.h"
#include "he/core/span.h"
#include "he/core/types.h"
#include "he/core/type_traits.h"
#include "he/core/utils.h"
#include "he/core/vector.h"
#include "he/schema/schema.h"

namespace he::schema
{
    // --------------------------------------------------------------------------------------------
    class ListBuilder;
    class PointerBuilder;
    class StructBuilder;
    class Builder;

    class ListReader;
    class PointerReader;
    class StructReader;

    struct String { class Reader; class Builder; };
    template <typename T> struct List { using ElementType = T; class Reader; class Builder; };
    //template <typename T> struct PointerHelper;

    // --------------------------------------------------------------------------------------------
    using Word = uint64_t;
    constexpr uint32_t BytesPerWord = sizeof(Word);
    constexpr uint32_t BitsPerByte = 8;
    constexpr uint32_t BitsPerWord = BytesPerWord * BitsPerByte;

    // --------------------------------------------------------------------------------------------
    template <size_t S> struct ElementSizeForByteSize;
    template <> struct ElementSizeForByteSize<1> { static constexpr ElementSize Value = ElementSize::Byte; };
    template <> struct ElementSizeForByteSize<2> { static constexpr ElementSize Value = ElementSize::TwoBytes; };
    template <> struct ElementSizeForByteSize<4> { static constexpr ElementSize Value = ElementSize::FourBytes; };
    template <> struct ElementSizeForByteSize<8> { static constexpr ElementSize Value = ElementSize::EightBytes; };

    // --------------------------------------------------------------------------------------------
    template <typename T> struct ListElementSize { static constexpr ElementSize Value = ElementSize::Pointer; };
    template <> struct ListElementSize<Void> { static constexpr ElementSize Value = ElementSize::Void; };
    template <> struct ListElementSize<bool> { static constexpr ElementSize Value = ElementSize::Bit; };

    template <typename T> requires(std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
    struct ListElementSize<T> { static constexpr ElementSize Value = ElementSizeForByteSize<sizeof(T)>::Value; };

    template <typename T> requires(std::is_enum_v<T>)
    struct ListElementSize<T> { static constexpr ElementSize Value = ElementSize::TwoBytes; };

    template <typename T> requires(T::DeclInfo::Kind == DeclKind::Struct)
    struct ListElementSize<T> { static constexpr ElementSize Value = ElementSize::Composite; };

    // --------------------------------------------------------------------------------------------
    class PointerReader
    {
    public:
        PointerReader() = default;
        explicit PointerReader(const Word* data) : m_data(data) { HE_ASSERT(IsAligned(data, BytesPerWord)); }

        // Common

        bool IsNull() const { return Value() == 0; }
        PointerKind Kind() const { return static_cast<PointerKind>(Value() & 0x03); }
        int32_t Offset() const { return BitCast<int32_t>(static_cast<uint32_t>(Value())) >> 2; }
        const Word* Target() const { return m_data + BytesPerWord + Offset(); }
        
        // Lists

        ListReader TryGetList(ElementSize expectedElementSize, const Word* defaultValue = nullptr) const;

        ElementSize ListElementSize() const { return static_cast<ElementSize>((Value() >> 32) & 0x07); }
        uint32_t ListSize() const { return static_cast<uint32_t>(Value() >> 35); }
        uint32_t ListCompositeSize() const { return static_cast<uint32_t>(Value()) >> 2; }

        template <typename T> typename List<T>::Reader TryGetList(const Word* defaultValue = nullptr) const;
        String::Reader TryGetString(const Word* defaultValue = nullptr) const;

        // Structs

        StructReader TryGetStruct(const Word* defaultValue = nullptr) const;

        uint16_t StructDataWordSize() const { return static_cast<uint16_t>(Value() >> 32); }
        uint16_t StructPointerCount() const { return static_cast<uint16_t>(Value() >> 48); }
        uint32_t StructWordSize() const { return static_cast<uint32_t>(StructDataWordSize()) + StructPointerCount(); }

        template <typename T>
        typename T::Reader TryGetStruct(const Word* defaultValue = nullptr) const
        {
            return typename T::Reader(TryGetStruct(defaultValue));
        }

    protected:
        uint64_t Value() const { return m_data ? *m_data : 0; }

    protected:
        const Word* m_data{ nullptr };
    };

    // --------------------------------------------------------------------------------------------
    class ListReader
    {
    public:
        ListReader() = default;

        ListReader(
            const Word* data,
            uint32_t size,
            uint32_t step,
            uint16_t structDataWordSize,
            uint16_t structPointerCount,
            ElementSize elementSize)
            : m_data(data)
            , m_size(size)
            , m_step(step)
            , m_structDataWordSize(structDataWordSize)
            , m_structPointerCount(structPointerCount)
            , m_elementSize(elementSize)
        {
            HE_ASSERT((structDataWordSize == 0 && structPointerCount == 0) || elementSize == ElementSize::Composite);
        }

        const Word* Data() const { return m_data; }
        uint32_t Size() const { return m_size; }
        ElementSize ElementSize() const { return m_elementSize; }

        template <typename T> T GetDataElement(uint32_t index) const
        {
            const uint8_t* b = reinterpret_cast<const uint8_t*>(m_data) + (static_cast<uint64_t>(index) * m_step / BitsPerByte);
            return *reinterpret_cast<const T*>(b);
        }

        template <> bool GetDataElement<bool>(uint32_t index) const
        {
            const uint8_t* b = reinterpret_cast<const uint8_t*>(m_data) + (index / BitsPerByte);
            const uint32_t shift = index % BitsPerByte;
            return (*b & (1 << shift)) != 0;
        }

        template <> Void GetDataElement<Void>(uint32_t) const { return {}; }

        PointerReader GetPointerElement(uint32_t index) const { return PointerReader(m_data + index); }

    protected:
        const Word* m_data{ nullptr };
        const uint32_t m_size{ 0 };
        const uint32_t m_step{ 0 };
        const uint16_t m_structDataWordSize{ 0 };
        const uint16_t m_structPointerCount{ 0 };
        const schema::ElementSize m_elementSize{ ElementSize::Void };
    };

    // --------------------------------------------------------------------------------------------
    class StructReader
    {
    public:
        StructReader() = default;
        StructReader(const StructReader& x) : StructReader(x.m_data, x.m_dataWordSize, x.m_pointerCount) {}
        StructReader(const Word* data, uint16_t dataWordSize, uint16_t pointerCount)
            : m_data(data)
            , m_dataWordSize(dataWordSize)
            , m_pointerCount(pointerCount)
        {}

        // Data fields

        bool HasDataField(uint16_t index) const
        {
            const uint16_t dataFieldCount = *reinterpret_cast<const uint16_t*>(m_data);
            if (index >= dataFieldCount)
                return false;

            if (index <= 32) [[likely]]
            {
                const uint32_t mask = *(reinterpret_cast<const uint32_t*>(m_data) + 1);
                return (mask & (1 << index)) != 0;
            }
            else
            {
                index -= 32;
                const uint32_t maskIndex = index / BitsPerWord;
                const uint32_t maskShift = index % BitsPerWord;
                const uint64_t mask = *(m_data + 1 + maskIndex);
                return (mask & (1ull << maskShift)) != 0;
            }
        }

        template <typename T> T GetDataField(uint32_t dataOffset, T defaultValue = static_cast<T>(0)) const
        {
            constexpr uint64_t BitsInType = sizeof(T) * BitsPerByte;
            if (((dataOffset + 1) * BitsInType) <= (m_dataWordSize * BitsPerWord)) [[likely]]
                return reinterpret_cast<const T*>(DataFields())[dataOffset];

            return defaultValue;
        }

        template <> bool GetDataField<bool>(uint32_t dataOffset, bool defaultValue) const
        {
            if (dataOffset < (m_dataWordSize * BitsPerWord)) [[likely]]
            {
                const uint8_t* b = reinterpret_cast<const uint8_t*>(DataFields()) + (dataOffset / BitsPerByte);
                const uint32_t shift = dataOffset % BitsPerByte;
                return (*b & (1 << shift)) != 0;
            }

            return defaultValue;
        }

        template <typename T> T TryGetDataField(uint16_t index, uint32_t dataOffset, T defaultValue = static_cast<T>(0)) const
        {
            return HasDataField(index) ? GetDataField(dataOffset) : defaultValue;
        }

        // Pointer Fields

        bool HasPointerField(uint16_t index) const
        {
            return !GetPointerField(index).IsNull();
        }

        PointerReader GetPointerField(uint16_t index) const
        {
            if (index < m_pointerCount) [[likely]]
                return PointerReader(PointerFields() + index);

            return PointerReader();
        }

        // Data Array Fields

        template <typename T>
        Span<const T> TryGetDataArrayField(uint16_t index, uint32_t dataOffset, uint16_t elementCount, Span<const T> defaultValue = {}) const
        {
            HE_ASSERT(defaultValue.IsEmpty() || defaultValue.Size() == elementCount);
            constexpr uint64_t BitsInType = sizeof(T) * BitsPerByte;
            if (((dataOffset + elementCount) * BitsInType) <= (m_dataWordSize * BitsPerWord)) [[likely]]
                return { reinterpret_cast<const T*>(DataFields())[dataOffset], elementCount };

            return defaultValue;
        }

        // Pointer Array Fields

        ListReader TryGetPointerArrayField(
            uint16_t index,
            ElementSize elementSize,
            uint16_t elementCount,
            uint16_t structDataWordSize,
            uint16_t structPointerCount,
            const Word* defaultValue = nullptr) const;

        template <typename T>
        typename List<T>::Reader TryGetPointerArrayField(uint16_t index, uint16_t elementCount, const Word* defaultValue = nullptr) const
        {
            constexpr ElementSize ESize = ListElementSize<T>::Value;
            constexpr uint16_t DataWordSize = ESize == ElementSize::Composite ? T::DeclInfo::DataWordSize : 0;
            constexpr uint16_t PointerCount = ESize == ElementSize::Composite ? T::DeclInfo::PointerCount : 0;
            return List<T>::Reader(TryGetPointerArrayField(index, ESize, elementCount, DataWordSize, PointerCount, defaultValue));
        }

    protected:
        uint32_t MetadataWordSize() const
        {
            const uint16_t dataFieldCount = *reinterpret_cast<const uint16_t*>(m_data);
            if (dataFieldCount <= 32) [[likely]]
                return 1;

            return ((dataFieldCount - 32) + (BitsPerWord - 1)) / BitsPerWord;
        }

        const Word* DataFields() const
        {
            return m_data + MetadataWordSize();
        }

        const Word* PointerFields() const
        {
            return m_data + m_dataWordSize;
        }

    protected:
        const Word* m_data{ nullptr };

        const uint16_t m_dataWordSize{ 0 };
        const uint16_t m_pointerCount{ 0 };
    };

    // --------------------------------------------------------------------------------------------
    void WriteStructPointer(Word* ptr, const StructBuilder& value);
    void WriteListPointer(Word* ptr, const ListBuilder& value);

    // --------------------------------------------------------------------------------------------
    class Builder
    {
    public:
        explicit Builder(Allocator& allocator = Allocator::GetDefault()) : Builder(32, allocator) {}
        explicit Builder(uint32_t initialWordSize, Allocator& allocator = Allocator::GetDefault())
            : m_data(allocator)
        {
            Reserve(initialWordSize);
            m_data.PushBack(0); // pointer to root
        }

        Word* Data() { return m_data.Data(); }
        const Word* Data() const { return m_data.Data(); }
        uint32_t Size() const { return m_data.Size(); }
        uint32_t ByteSize() const { return Size() * BytesPerWord; }

        void Reserve(uint32_t words) { m_data.Reserve(words); }

        void SetRoot(const StructBuilder& root) { WriteStructPointer(m_data.Data(), root); }

        StructBuilder AddStruct(uint16_t dataFieldCount, uint16_t dataWordSize, uint16_t pointerCount);
        ListBuilder AddList(ElementSize elementSize, uint32_t elementCount);
        ListBuilder AddStructList(uint32_t elementCount, uint16_t dataWordSize, uint16_t pointerCount);

        template <typename T>
        T::Builder AddStruct()
        {
            constexpr uint16_t DataFieldCount = T::DeclInfo::DataFieldCount;
            constexpr uint16_t DataWordSize = T::DeclInfo::DataWordSize;
            constexpr uint16_t PointerCount = T::DeclInfo::PointerCount;
            return T::Builder(AddStruct(DataFieldCount, DataWordSize, PointerCount));
        }

        template <typename T>
        List<T>::Builder AddList(uint32_t elementCount)
        {
            if constexpr (ListElementSize<T>::Value == ElementSize::Composite)
            {
                constexpr uint16_t DataWordSize = T::DeclInfo::DataWordSize;
                constexpr uint16_t PointerCount = T::DeclInfo::PointerCount;
                return List<T>::Builder(AddStructList(elementCount, DataWordSize, PointerCount));
            }
            else
            {
                return List<T>::Builder(AddList(ListElementSize<T>::Value, elementCount));
            }
        }

    private:
        Vector<Word> m_data;
    };

    // --------------------------------------------------------------------------------------------
    class ListBuilder
    {
    public:
        ListBuilder() = default;
        ListBuilder(const ListBuilder& x) : ListBuilder(x.m_builder, x.m_wordOffset, x.m_size, x.m_step, x.m_structDataWordSize, x.m_structPointerCount, x.m_elementSize) {}
        ListBuilder(
            Builder& builder,
            uint32_t wordOffset,
            uint32_t size,
            uint32_t step,
            uint16_t structDataWordSize,
            uint16_t structPointerCount,
            ElementSize elementSize)
            : m_builder(builder)
            , m_wordOffset(wordOffset)
            , m_size(size)
            , m_step(step)
            , m_structDataWordSize(structDataWordSize)
            , m_structPointerCount(structPointerCount)
            , m_elementSize(elementSize)
        {
            HE_ASSERT((structDataWordSize == 0 && structPointerCount == 0) || elementSize == ElementSize::Composite);
        }

        Builder& Builder() { return m_builder; }
        const schema::Builder& Builder() const { return m_builder; }

        Word* Location() { return m_builder.Data() + m_wordOffset; }
        const Word* Location() const { return m_builder.Data() + m_wordOffset; }

        uint32_t Size() const { return m_size; }
        ElementSize ElementSize() const { return m_elementSize; }

    protected:
        schema::Builder& m_builder;
        const uint32_t m_wordOffset{ 0 };
        const uint32_t m_size{ 0 };
        const uint32_t m_step{ 0 };
        const uint16_t m_structDataWordSize{ 0 };
        const uint16_t m_structPointerCount{ 0 };
        const schema::ElementSize m_elementSize{ ElementSize::Void };
    };

    // --------------------------------------------------------------------------------------------
    class StructBuilder
    {
    public:
        StructBuilder() = default;
        StructBuilder(const StructBuilder& x) : StructBuilder(x.m_builder, x.m_wordOffset, x.m_dataFieldCount, x.m_dataWordSize, x.m_pointerCount) {}
        StructBuilder(Builder& builder, uint32_t wordOffset, uint16_t dataFieldCount, uint16_t dataWordSize, uint16_t pointerCount)
            : m_builder(builder)
            , m_wordOffset(wordOffset)
            , m_dataFieldCount(dataFieldCount)
            , m_dataWordSize(dataWordSize)
            , m_pointerCount(pointerCount)
        {
            Word* metadata = DataSection();
            *metadata |= static_cast<uint64_t>(m_dataFieldCount);
        }

        StructReader AsReader() const { return StructReader(m_builder.Data() + m_wordOffset, m_dataWordSize, m_pointerCount); }

        Builder& Builder() { return m_builder; }
        const schema::Builder& Builder() const { return m_builder; }

        Word* Location() { return m_builder.Data() + m_wordOffset; }
        const Word* Location() const { return m_builder.Data() + m_wordOffset; }

        uint16_t DataFieldCount() const { return m_dataFieldCount; }
        uint16_t DataWordSize() const { return m_dataWordSize; }
        uint16_t PointerCount() const { return m_pointerCount; }

        void ClearDataFields() { MemZero(DataSection(), m_dataWordSize * BytesPerWord); }
        void ClearPointerFields() { MemZero(PointerSection(), m_pointerCount * BytesPerWord); }
        void ClearAllFields() { MemZero(DataSection(), (m_dataWordSize + m_pointerCount) * BytesPerWord); }

        template <typename T>
        void SetDataField(uint32_t dataOffset, T value)
        {
            reinterpret_cast<T*>(DataFields())[dataOffset] = value;
        }

        template <typename T>
        void SetDataField(uint16_t index, uint32_t dataOffset, T value)
        {
            MarkDataFieldSet(index);
            reinterpret_cast<T*>(DataFields())[dataOffset] = value;
        }

        template <>
        void SetDataField<bool>(uint16_t index, uint32_t dataOffset, bool value)
        {
            MarkDataFieldSet(index);
            uint8_t* b = reinterpret_cast<uint8_t*>(DataFields()) + (dataOffset / BitsPerByte);
            const uint32_t shift = dataOffset % BitsPerByte;
            *b = (*b & ~(1 << shift)) | (static_cast<uint8_t>(value) << shift);
        }

        template <>
        void SetDataField<Void>(uint16_t, uint32_t, Void) {}

        void SetPointerField(uint16_t index, const StructBuilder& value)
        {
            HE_ASSERT(&value.Builder() == &m_builder);
            Word* ptr = PointerSection() + index;
            WriteStructPointer(ptr, value);
        }

        void SetPointerField(uint16_t index, const ListBuilder& value)
        {
            HE_ASSERT(&value.Builder() == &m_builder);
            Word* ptr = PointerSection() + index;
            WriteListPointer(ptr, value);
        }

    protected:
        Word* DataSection() { return m_builder.Data() + m_wordOffset; }
        const Word* DataSection() const { return m_builder.Data() + m_wordOffset; }

        Word* PointerSection() { return DataSection() + m_dataWordSize; }
        const Word* PointerSection() const { return DataSection() + m_dataWordSize; }

        uint32_t MetadataWordSize() const
        {
            if (m_dataFieldCount <= 32) [[likely]]
                return 1;

            return ((m_dataFieldCount - 32) + (BitsPerWord - 1)) / BitsPerWord;
        }

        Word* DataFields() { return DataSection() + MetadataWordSize(); }
        const Word* DataFields() const { return DataSection() + MetadataWordSize(); }

        void MarkDataFieldSet(uint16_t index)
        {
            HE_ASSERT(index < m_dataFieldCount);

            if (index <= 32) [[likely]]
            {
                uint32_t* mask = reinterpret_cast<uint32_t*>(DataSection()) + 1;
                *mask |= (1 << index);
            }
            else
            {
                index -= 32;
                const uint32_t maskIndex = index / BitsPerWord;
                const uint32_t maskShift = index % BitsPerWord;
                uint64_t* mask = DataSection() + 1 + maskIndex;
                *mask |= (1ull << maskShift);
            }
        }

    protected:
        schema::Builder& m_builder;
        uint32_t m_wordOffset{ 0 };
        uint16_t m_dataFieldCount{ 0 };
        uint16_t m_dataWordSize{ 0 };
        uint16_t m_pointerCount{ 0 };
    };

    // --------------------------------------------------------------------------------------------
    //struct String
    //{
        class String::Reader : public ListReader
        {
        public:
            Reader(ListReader reader) : ListReader(reader) {}

            const char* Data() const { return reinterpret_cast<const char*>(ListReader::Data()); }
            uint32_t Size() const { return ListReader::Size() - 1; }
        };

        class String::Builder : public ListBuilder
        {
        public:
            Builder(ListBuilder builder) : ListBuilder(builder) {}

            char* Data() { return reinterpret_cast<char*>(ListBuilder::Location()); }
            const char* Data() const { return reinterpret_cast<const char*>(ListBuilder::Location()); }

            uint32_t Size() const { return ListBuilder::Size() - 1; }

            char& operator[](uint32_t index)
            {
                HE_ASSERT(index < Size());
                return Data()[index];
            }
        };
    //};

    // --------------------------------------------------------------------------------------------
    //template <typename T>
    //struct List
    //{
        //using ElementType = T;

        template <typename T>
        class List<T>::Reader : public ListReader
        {
        public:
            explicit Reader(ListReader reader) : ListReader(reader) {}

            typename T::Reader operator[](uint32_t index) const
            {
                HE_ASSERT(index < Size());
                if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_same_v<T, Void>)
                    return ListReader::GetDataElement<T>(index);
                else
                    return PointerHelper<T>::Get(ListReader::GetPointerElement(index));
            }
        };

        template <typename T>
        class List<T>::Builder : public ListBuilder
        {
        public:
            Builder(ListBuilder builder) : ListBuilder(builder) {}

            // TODO!
            //typename T::Builder operator[](uint32_t index) const
            //{
            //    HE_ASSERT(index < Size());
            //    return PointerHelper<T>::Get(ListBuilder::GetPointerElement(index));
            //}
        };
    //};

    //template <typename T> requires(std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_same_v<T, Void>)
    //struct List<T>
    //{
    //    using ElementType = T;

        //template <typename T> requires()
        //class List<T>::Reader : public ListReader
        //{
        //public:
        //    explicit Reader(ListReader reader) : ListReader(reader) {}

        //    const T* Data() const { return reinterpret_cast<const T*>(m_data); }

        //    T operator[](uint32_t index) const
        //    {
        //        HE_ASSERT(index < Size());
        //        return ListReader::GetDataElement<T>(index);
        //    }
        //};

        //template <typename T> requires(std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_same_v<T, Void>)
        //class List<T>::Builder : public ListBuilder
        //{
        //public:
        //    Builder(ListBuilder builder) : ListBuilder(builder) {}

        //    // TODO!
        //    //T& operator[](uint32_t index)
        //    //{
        //    //    HE_ASSERT(index < Size());
        //    //    return ListBuilder::GetDataElement<T>(index);
        //    //}
        //};
    //};

    // --------------------------------------------------------------------------------------------

    template <typename T>
    typename List<T>::Reader PointerReader::TryGetList(const Word* defaultValue) const
    {
        return typename List<T>::Reader(TryGetList(ListElementSize<T>::Value, defaultValue));
    }

    String::Reader PointerReader::TryGetString(const Word* defaultValue) const
    {
        return String::Reader(TryGetList(ElementSize::Byte, defaultValue));
    }

    //template <typename T>
    //struct PointerHelper
    //{
    //    static typename T::Reader TryGet(PointerReader reader, const Word* defaultValue = nullptr)
    //    {
    //        return typename T::Reader(reader.GetStruct(defaultValue));
    //    }
    //};

    //template <typename T>
    //struct PointerHelper<List<T>>
    //{
    //    static typename List<T>::Reader TryGet(PointerReader reader, const Word* defaultValue = nullptr)
    //    {
    //        return typename List<T>::Reader(reader.GetList(ListElementSize<T>::Value, defaultValue));
    //    }
    //};

    //template <>
    //struct PointerHelper<String>
    //{
    //    static String::Reader TryGet(PointerReader reader, const Word* defaultValue = nullptr)
    //    {
    //        return String::Reader(reader.GetList(ElementSize::Byte, defaultValue));
    //    }
    //};
}
