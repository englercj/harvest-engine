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
    class Builder;
    class ListBuilder;
    class PointerBuilder;
    class StructBuilder;

    class ListReader;
    class PointerReader;
    class StructReader;

    struct String { class Reader; class Builder; };
    template <typename T> struct List { using ElementType = T; class Reader; class Builder; };

    // --------------------------------------------------------------------------------------------
    using Word = uint64_t;
    constexpr uint32_t BytesPerWord = sizeof(Word);
    constexpr uint32_t BitsPerByte = 8;
    constexpr uint32_t BitsPerWord = BytesPerWord * BitsPerByte;

    // --------------------------------------------------------------------------------------------
    template <typename T> concept DataType = std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_same_v<T, Void>;

    template <typename T> struct TypeHelper { using Reader = typename T::Reader; using Builder = typename T::Builder; };
    template <DataType T> struct TypeHelper<T> { using Reader = T; using Builder = T&; };

    // --------------------------------------------------------------------------------------------
    template <size_t S> struct ElementSizeForByteSize;
    template <> struct ElementSizeForByteSize<1> { static constexpr ElementSize Value = ElementSize::Byte; };
    template <> struct ElementSizeForByteSize<2> { static constexpr ElementSize Value = ElementSize::TwoBytes; };
    template <> struct ElementSizeForByteSize<4> { static constexpr ElementSize Value = ElementSize::FourBytes; };
    template <> struct ElementSizeForByteSize<8> { static constexpr ElementSize Value = ElementSize::EightBytes; };

    // --------------------------------------------------------------------------------------------
    template <typename T> struct ElementSizeOfType { static constexpr ElementSize Value = ElementSize::Pointer; };
    template <> struct ElementSizeOfType<Void> { static constexpr ElementSize Value = ElementSize::Void; };
    template <> struct ElementSizeOfType<bool> { static constexpr ElementSize Value = ElementSize::Bit; };

    template <Arithmetic T> requires(!std::is_same_v<T, bool>)
    struct ElementSizeOfType<T> { static constexpr ElementSize Value = ElementSizeForByteSize<sizeof(T)>::Value; };

    template <Enum T>
    struct ElementSizeOfType<T> { static constexpr ElementSize Value = ElementSize::TwoBytes; };

    template <typename T> requires(T::DeclInfo::Kind == DeclKind::Struct)
    struct ElementSizeOfType<T> { static constexpr ElementSize Value = ElementSize::Composite; };

    // --------------------------------------------------------------------------------------------
    class PointerReader
    {
    public:
        PointerReader() = default;
        explicit PointerReader(const Word* data) : m_data(data) {}

        // Common

        bool IsNull() const { return Value() == 0; }
        bool IsZeroStruct() const { return Value() == 0xfffffffc; }
        PointerKind Kind() const { return static_cast<PointerKind>(Value() & 0x03); }
        int32_t Offset() const { return BitCast<int32_t>(static_cast<uint32_t>(Value())) >> 2; }
        const Word* Target() const { return m_data + 1 + Offset(); }

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

    private:
        uint64_t Value() const { return m_data ? *m_data : 0; }

    private:
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
        uint32_t StepSize() const { return m_step; }
        uint16_t StructDataWordSize() const { return m_structDataWordSize; }
        uint16_t StructPointerCount() const { return m_structPointerCount; }
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

    private:
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
            return HasDataField(index) ? GetDataField(dataOffset, defaultValue) : defaultValue;
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

            if (!HasDataField(index))
                return defaultValue;

            constexpr uint64_t BitsInType = sizeof(T) * BitsPerByte;
            if (((dataOffset + elementCount) * BitsInType) <= (m_dataWordSize * BitsPerWord)) [[likely]]
            {
                const T* data = reinterpret_cast<const T*>(DataFields()) + dataOffset;
                return { data, elementCount };
            }

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
            constexpr ElementSize ESize = ElementSizeOfType<T>::Value;
            constexpr uint16_t DataWordSize = ESize == ElementSize::Composite ? T::DeclInfo::DataWordSize : 0;
            constexpr uint16_t PointerCount = ESize == ElementSize::Composite ? T::DeclInfo::PointerCount : 0;
            return List<T>::Reader(TryGetPointerArrayField(index, ESize, elementCount, DataWordSize, PointerCount, defaultValue));
        }

    private:
        uint32_t MetadataWordSize() const
        {
            const uint16_t dataFieldCount = *reinterpret_cast<const uint16_t*>(m_data);
            if (dataFieldCount <= 32) [[likely]]
                return 1;

            return ((dataFieldCount - 32) + (BitsPerWord - 1)) / BitsPerWord;
        }

        const Word* DataFields() const { return m_data + MetadataWordSize(); }
        const Word* PointerFields() const { return m_data + m_dataWordSize; }

    private:
        const Word* m_data{ nullptr };

        const uint16_t m_dataWordSize{ 0 };
        const uint16_t m_pointerCount{ 0 };
    };

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

        void SetRoot(const StructBuilder& root);

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
            if constexpr (ElementSizeOfType<T>::Value == ElementSize::Composite)
            {
                constexpr uint16_t DataWordSize = T::DeclInfo::DataWordSize;
                constexpr uint16_t PointerCount = T::DeclInfo::PointerCount;
                return List<T>::Builder(AddStructList(elementCount, DataWordSize, PointerCount));
            }
            else
            {
                return List<T>::Builder(AddList(ElementSizeOfType<T>::Value, elementCount));
            }
        }

    private:
        Vector<Word> m_data;
    };

    // --------------------------------------------------------------------------------------------
    class PointerBuilder
    {
    public:
        explicit PointerBuilder(Builder& builder) : m_builder(builder) {}
        PointerBuilder(Builder& builder, uint32_t wordOffset)
            : m_builder(builder)
            , m_wordOffset(wordOffset)
        {}

        // Common

        PointerReader AsReader() const { return PointerReader(Location()); }

        Word* Location() { return m_builder.Data() + m_wordOffset; }
        const Word* Location() const { return m_builder.Data() + m_wordOffset; }

        bool IsNull() const { return Value() == 0; }
        void SetNull() { SetOffsetAndKind(0, Kind()); }

        bool IsZeroStruct() const { return Value() == 0xfffffffc; }
        void SetZeroStruct() { Value() = 0xfffffffc; }

        PointerKind Kind() const { return static_cast<PointerKind>(Value() & 0x03); }
        void SetKind(PointerKind kind) { SetOffsetAndKind(Offset(), kind); }

        int32_t Offset() const { return BitCast<int32_t>(static_cast<uint32_t>(Value())) >> 2; }
        void SetOffset(int32_t offset) { SetOffsetAndKind(offset, Kind()); }

        const Word* Target() const { return Location() + 1 + Offset(); }
        void SetTarget(const Word* target) { SetTargetAndKind(target, Kind()); }

        void SetOffsetAndKind(int32_t offset, PointerKind kind) { Value() = (static_cast<uint32_t>(offset) << 2) | (static_cast<uint16_t>(kind) & 0x03); }
        void SetTargetAndKind(const Word* target, PointerKind kind) { SetOffsetAndKind(static_cast<int32_t>(target - Location() - 1), kind); }

        // Lists

        ListBuilder TryGetList(ElementSize expectedElementSize, const Word* defaultValue = nullptr) const;

        ElementSize ListElementSize() const { return static_cast<ElementSize>((Value() >> 32) & 0x07); }
        uint32_t ListSize() const { return static_cast<uint32_t>(Value() >> 35); }
        uint32_t ListCompositeSize() const { return static_cast<uint32_t>(Value()) >> 2; }

        template <typename T> typename List<T>::Builder TryGetList(const Word* defaultValue = nullptr) const;
        String::Builder TryGetString(const Word* defaultValue = nullptr) const;

        // Structs

        StructBuilder TryGetStruct(const Word* defaultValue = nullptr) const;

        uint16_t StructDataWordSize() const { return static_cast<uint16_t>(Value() >> 32); }
        uint16_t StructPointerCount() const { return static_cast<uint16_t>(Value() >> 48); }
        uint32_t StructWordSize() const { return static_cast<uint32_t>(StructDataWordSize()) + StructPointerCount(); }

        template <typename T>
        typename T::Builder TryGetStruct(const Word* defaultValue = nullptr) const
        {
            return typename T::Builder(TryGetStruct(defaultValue));
        }

    private:
        uint64_t Value() const { return *Location(); }
        uint64_t& Value() { return *Location(); }

    private:
        schema::Builder& m_builder;
        const uint32_t m_wordOffset{ 0 };
    };

    // --------------------------------------------------------------------------------------------
    class ListBuilder
    {
    public:
        explicit ListBuilder(Builder& builder) : m_builder(builder) {}
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

        ListReader AsReader() const { return ListReader(Data(), m_size, m_step, m_structDataWordSize, m_structPointerCount, m_elementSize); }

        Word* Data() { return Location(); }
        const Word* Data() const { return Location(); }

        Builder& Builder() { return m_builder; }
        const schema::Builder& Builder() const { return m_builder; }

        Word* Location() { return m_builder.Data() + m_wordOffset; }
        const Word* Location() const { return m_builder.Data() + m_wordOffset; }

        uint32_t Size() const { return m_size; }
        uint32_t StepSize() const { return m_step; }
        uint16_t StructDataWordSize() const { return m_structDataWordSize; }
        uint16_t StructPointerCount() const { return m_structPointerCount; }
        ElementSize ElementSize() const { return m_elementSize; }

        template <typename T> T GetDataElement(uint32_t index) const
        {
            const uint8_t* b = reinterpret_cast<const uint8_t*>(Data()) + (static_cast<uint64_t>(index) * m_step / BitsPerByte);
            return *reinterpret_cast<const T*>(b);
        }

        template <> bool GetDataElement<bool>(uint32_t index) const
        {
            const uint8_t* b = reinterpret_cast<const uint8_t*>(Data()) + (index / BitsPerByte);
            const uint32_t shift = index % BitsPerByte;
            return (*b & (1 << shift)) != 0;
        }

        template <> Void GetDataElement<Void>(uint32_t) const { return {}; }

        PointerBuilder GetPointerElement(uint32_t index) const { return PointerBuilder(m_builder, m_wordOffset + index); }

    private:
        schema::Builder& m_builder;
        uint32_t m_wordOffset{ 0 };
        uint32_t m_size{ 0 };
        uint32_t m_step{ 0 };
        uint16_t m_structDataWordSize{ 0 };
        uint16_t m_structPointerCount{ 0 };
        schema::ElementSize m_elementSize{ ElementSize::Void };
    };

    // --------------------------------------------------------------------------------------------
    class StructBuilder
    {
    public:
        explicit StructBuilder(Builder& builder) : m_builder(builder) {}
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

        void SetPointerField(uint16_t index, const StructBuilder& value);
        void SetPointerField(uint16_t index, const ListBuilder& value);

    private:
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

    private:
        schema::Builder& m_builder;
        uint32_t m_wordOffset{ 0 };
        uint16_t m_dataFieldCount{ 0 };
        uint16_t m_dataWordSize{ 0 };
        uint16_t m_pointerCount{ 0 };
    };

    // --------------------------------------------------------------------------------------------
    class String::Reader : public ListReader
    {
    public:
        explicit Reader(ListReader reader) : ListReader(reader) {}

        const char* Data() const { return reinterpret_cast<const char*>(ListReader::Data()); }
        uint32_t Size() const { return ListReader::Size() - 1; }
    };

    class String::Builder : public ListBuilder
    {
    public:
        explicit Builder(ListBuilder builder) : ListBuilder(builder) {}

        char* Data() { return reinterpret_cast<char*>(Location()); }
        const char* Data() const { return reinterpret_cast<const char*>(Location()); }

        uint32_t Size() const { return ListBuilder::Size() - 1; }

        char& operator[](uint32_t index)
        {
            HE_ASSERT(index < Size());
            return Data()[index];
        }
    };

    // --------------------------------------------------------------------------------------------
    template <typename T>
    class List<T>::Reader : public ListReader
    {
    public:
        explicit Reader(ListReader reader) : ListReader(reader) {}

        const T* Data() const requires(DataType<T>) { return reinterpret_cast<const T*>(m_data); }

        typename TypeHelper<T>::Reader operator[](uint32_t index) const
        {
            HE_ASSERT(index < Size());
            if constexpr (DataType<T>)
                return GetDataElement<T>(index);
            else if constexpr (std::is_same_v<T, String>)
                return GetPointerElement(index).TryGetString();
            else if constexpr (IsSpecialization<T, List>)
                return GetPointerElement(index).TryGetList<T::ElementType>();
            else
                return GetPointerElement(index).TryGetStruct<T>();
        }
    };

    template <typename T>
    class List<T>::Builder : public ListBuilder
    {
    public:
        explicit Builder(ListBuilder builder) : ListBuilder(builder) {}

        T* Data() requires(DataType<T>) { return reinterpret_cast<T*>(Data()); }
        const T* Data() const requires(DataType<T>) { return reinterpret_cast<const T*>(Data()); }

        typename TypeHelper<T>::Builder operator[](uint32_t index) const
        {
            HE_ASSERT(index < Size());
            if constexpr (DataType<T>)
                return ListReader::GetDataElement<T>(index);
            else if constexpr (std::is_same_v<T, String>)
                return GetPointerElement(index).TryGetString();
            else if constexpr (IsSpecialization<T, List>)
                return GetPointerElement(index).TryGetList<T::ElementType>();
            else
                return GetPointerElement(index).TryGetStruct<T>();
        }

        // TODO: It is awkward to have to call Set, and not be able to use the operator[]...
        void Set(uint32_t index, const typename TypeHelper<T>::Reader& reader)
        {

        }
    };

    // --------------------------------------------------------------------------------------------
    template <typename T>
    typename List<T>::Reader PointerReader::TryGetList(const Word* defaultValue) const
    {
        return typename List<T>::Reader(TryGetList(ElementSizeOfType<T>::Value, defaultValue));
    }

    String::Reader PointerReader::TryGetString(const Word* defaultValue) const
    {
        return String::Reader(TryGetList(ElementSize::Byte, defaultValue));
    }

    // --------------------------------------------------------------------------------------------
    template <typename T>
    typename List<T>::Builder PointerBuilder::TryGetList(const Word* defaultValue) const
    {
        return typename List<T>::Builder(TryGetList(ElementSizeOfType<T>::Value, defaultValue));
    }

    String::Builder PointerBuilder::TryGetString(const Word* defaultValue) const
    {
        return String::Builder(TryGetList(ElementSize::Byte, defaultValue));
    }
}
