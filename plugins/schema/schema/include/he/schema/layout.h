// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/memory_ops.h"
#include "he/core/span.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/core/type_traits.h"
#include "he/core/utils.h"
#include "he/core/vector.h"
#include "he/schema/types.h"

#include <iterator>

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

    // TODO: Better helpers for the Any* objects that make usage easier to do correctly.
    struct AnyPointer { using Reader = PointerReader; using Builder = PointerBuilder; };
    struct AnyStruct { using Reader = PointerReader; using Builder = PointerBuilder; };
    struct AnyList { using Reader = PointerReader; using Builder = PointerBuilder; };
    struct String { class Reader; class Builder; };
    template <typename T> struct List { using ElementType = T; class Reader; class Builder; };

    using Blob = List<uint8_t>;

    // --------------------------------------------------------------------------------------------
    constexpr uint32_t BytesPerWord = sizeof(Word);
    constexpr uint32_t BitsPerByte = 8;
    constexpr uint32_t BitsPerWord = BytesPerWord * BitsPerByte;

    // --------------------------------------------------------------------------------------------
    template <typename T> concept DataType = std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_same_v<T, Void>;

    template <typename T>
    struct LayoutTraits
    {
        using Reader = typename T::Reader;
        using Builder = typename T::Builder;
        static constexpr bool IsList = false;
    };

    template <DataType T>
    struct LayoutTraits<T>
    {
        using Reader = T;
        using Builder = T;
        static constexpr bool IsList = false;
    };

    template <typename T>
    struct LayoutTraits<List<T>>
    {
        using Reader = typename List<T>::Reader;
        using Builder = typename List<T>::Builder;
        static constexpr bool IsList = true;
    };

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

    template <typename T> requires(T::Kind == DeclKind::Struct)
    struct ElementSizeOfType<T> { static constexpr ElementSize Value = ElementSize::Composite; };

    // --------------------------------------------------------------------------------------------
    struct _PtrMem
    {
        uint32_t offsetAndKind;

        union
        {
            uint32_t upper32Bits;

            struct
            {
                uint32_t elementSizeAndCount;
            } listRef;

            struct
            {
                uint16_t dataWordSize;
                uint16_t pointerCount;
            } structRef;
        };
    };

    // --------------------------------------------------------------------------------------------
    // As of GCC 11 it *still* doesn't support explicit member function specialization in a
    // non-namespace scope so we have the actual definitions here and have the member functions
    // call into them.
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85282

    template <DataType T>
    inline T _ReadDataElement(const Word* data, uint32_t index, uint32_t step)
    {
        HE_ASSERT(IsAligned(step, BitsPerByte));
        const uint8_t* element = reinterpret_cast<const uint8_t*>(data) + (static_cast<uint64_t>(index) * (step / BitsPerByte));
        return *reinterpret_cast<const T*>(element);
    }

    template <>
    inline bool _ReadDataElement<bool>(const Word* data, uint32_t index, uint32_t step)
    {
        HE_UNUSED(step);
        const uint8_t* bits = reinterpret_cast<const uint8_t*>(data) + (index / BitsPerByte);
        const uint32_t shift = index % BitsPerByte;
        return (*bits & (1 << shift)) != 0;
    }

    template <>
    inline Void _ReadDataElement<Void>(const Word* data, uint32_t index, uint32_t step)
    {
        HE_UNUSED(data, index, step);
        return {};
    }

    template <DataType T>
    inline void _WriteDataElement(Word* data, uint32_t index, uint32_t step, T value)
    {
        uint8_t* dst = reinterpret_cast<uint8_t*>(data) + (static_cast<uint64_t>(index) * step / BitsPerByte);
        MemCopy(dst, &value, sizeof(T));
    }

    template <>
    inline void _WriteDataElement<bool>(Word* data, uint32_t index, uint32_t step, bool value)
    {
        HE_UNUSED(step);
        uint8_t* b = reinterpret_cast<uint8_t*>(data) + (index / BitsPerByte);
        const uint32_t shift = index % BitsPerByte;
        *b = (*b & ~(1 << shift)) | (static_cast<uint8_t>(value) << shift);
    }

    template <>
    inline void _WriteDataElement<Void>(Word* data, uint32_t index, uint32_t step, Void value)
    {
        HE_UNUSED(data, index, step, value);
    }

    template <DataType T>
    inline T _ReadDataFieldUnsafe(const Word* data, uint32_t dataOffset)
    {
        return reinterpret_cast<const T*>(data)[dataOffset];
    }

    template <>
    inline bool _ReadDataFieldUnsafe<bool>(const Word* data, uint32_t dataOffset)
    {
        const uint8_t* b = reinterpret_cast<const uint8_t*>(data) + (dataOffset / BitsPerByte);
        const uint32_t shift = dataOffset % BitsPerByte;
        return (*b & (1 << shift)) != 0;
    }

    template <>
    inline Void _ReadDataFieldUnsafe<Void>(const Word* data, uint32_t dataOffset)
    {
        HE_UNUSED(data, dataOffset);
        return {};
    }

    template <DataType T>
    inline T _ReadDataField(const Word* data, uint64_t dataFieldsWordSize, uint32_t dataOffset, T defaultValue)
    {
        constexpr uint64_t BitsInType = sizeof(T) * BitsPerByte;
        if (((dataOffset + 1) * BitsInType) <= (dataFieldsWordSize * BitsPerWord)) [[likely]]
            return _ReadDataFieldUnsafe<T>(data, dataOffset);

        return defaultValue;
    }

    template <>
    inline bool _ReadDataField<bool>(const Word* data, uint64_t dataFieldsWordSize, uint32_t dataOffset, bool defaultValue)
    {
        if (dataOffset < (dataFieldsWordSize * BitsPerWord)) [[likely]]
            return _ReadDataFieldUnsafe<bool>(data, dataOffset);

        return defaultValue;
    }

    template <>
    inline Void _ReadDataField<Void>(const Word* data, uint64_t dataFieldsWordSize, uint32_t dataOffset, Void defaultValue)
    {
        HE_UNUSED(data, dataFieldsWordSize, dataOffset, defaultValue);
        return {};
    }

    template <DataType T>
    inline void _WriteDataField(Word* data, uint32_t dataOffset, T value)
    {
        reinterpret_cast<T*>(data)[dataOffset] = value;
    }

    template <>
    inline void _WriteDataField<bool>(Word* data, uint32_t dataOffset, bool value)
    {
        uint8_t* b = reinterpret_cast<uint8_t*>(data) + (dataOffset / BitsPerByte);
        const uint32_t shift = dataOffset % BitsPerByte;
        *b = (*b & ~(1 << shift)) | (static_cast<uint8_t>(value) << shift);
    }

    template <>
    inline void _WriteDataField<Void>(Word* data, uint32_t dataOffset, Void value)
    {
        HE_UNUSED(data, dataOffset, value);
    }

    class BitSpan
    {
    public:
        bool IsSet(uint32_t index) const
        {
            const uint8_t* b = data + (index / BitsPerByte);
            const uint32_t shift = (index % BitsPerByte) + offset;
            return (*b & (1 << shift)) != 0;
        }

        void Set(uint32_t index, bool value)
        {
            uint8_t* b = data + (index / BitsPerByte);
            const uint32_t shift = index % BitsPerByte;
            *b = (*b & ~(1 << shift)) | (static_cast<uint8_t>(value) << shift);
        }

        bool IsEmpty() const { return count == 0; }

        struct Ref
        {
            BitSpan* span;
            uint32_t index;
            Ref& operator=(bool value) noexcept { span->Set(index, value); return *this; }
            [[nodiscard]] explicit operator bool() const { return span->IsSet(index); }
        };
        Ref operator[](uint32_t index) { return Ref(this, index); }

        uint8_t* data;
        uint32_t offset;
        uint32_t count;
    };

    template <DataType T> struct _ReadDataArrayReturnType { using Type = Span<T>; };
    template <> struct _ReadDataArrayReturnType<bool> { using Type = BitSpan; };
    template <> struct _ReadDataArrayReturnType<Void> { using Type = Void; };

    template <DataType T>
    inline typename _ReadDataArrayReturnType<T>::Type _ReadDataArrayField(Word* data, uint32_t dataWordSize, uint32_t dataOffset, uint16_t elementCount)
    {
        constexpr uint64_t BitsInType = sizeof(T) * BitsPerByte;
        if (((dataOffset + elementCount) * BitsInType) <= (dataWordSize * BitsPerWord)) [[likely]]
        {
            T* b = reinterpret_cast<T*>(data) + dataOffset;
            return { b, elementCount };
        }

        return {};
    }

    template <>
    inline BitSpan _ReadDataArrayField<bool>(Word* data, uint32_t dataWordSize, uint32_t dataOffset, uint16_t elementCount)
    {
        if ((dataOffset + elementCount) <= (dataWordSize * BitsPerWord)) [[likely]]
        {
            uint8_t* b = reinterpret_cast<uint8_t*>(data) + (dataOffset / BitsPerByte);
            const uint32_t offset = dataOffset % BitsPerByte;
            return { b, offset, elementCount };
        }

        return {};
    }

    template <>
    inline Void _ReadDataArrayField<Void>(Word* data, uint32_t dataWordSize, uint32_t dataOffset, uint16_t elementCount)
    {
        HE_UNUSED(data, dataWordSize, dataOffset, elementCount);
        return {};
    }

    // --------------------------------------------------------------------------------------------
    class PointerReader
    {
    public:
        PointerReader() = default;
        explicit PointerReader(const Word* data) noexcept : m_data(data) {}

        // Common

        const Word* Data() const { return m_data; }
        bool IsValid() const { return m_data != nullptr; }
        bool IsNull() const { return m_data == nullptr || (Value().offsetAndKind == 0 && Value().upper32Bits == 0); }
        bool IsZeroStruct() const { return Value().offsetAndKind == 0xfffffffc && Value().upper32Bits == 0; }
        PointerKind Kind() const { return static_cast<PointerKind>(Value().offsetAndKind & 0x03); }
        int32_t Offset() const { return BitCast<int32_t>(Value().offsetAndKind) >> 2; }
        const Word* Target() const { return m_data + 1 + Offset(); }

        // Lists

        ListReader TryGetList(ElementSize expectedElementSize, const Word* defaultValue = nullptr) const;

        ElementSize ListElementSize() const { return static_cast<ElementSize>(Value().listRef.elementSizeAndCount & 0x07); }
        uint32_t ListSize() const { return Value().listRef.elementSizeAndCount >> 3; }
        uint32_t ListCompositeTagSize() const { return Value().offsetAndKind >> 2; }

        template <typename T> typename List<T>::Reader TryGetList(const Word* defaultValue = nullptr) const;
        String::Reader TryGetString(const Word* defaultValue = nullptr) const;
        Blob::Reader TryGetBlob(const Word* defaultValue = nullptr) const;

        // Structs

        StructReader TryGetStruct(const Word* defaultValue = nullptr) const;

        uint16_t StructDataWordSize() const { return Value().structRef.dataWordSize; }
        uint16_t StructPointerCount() const { return Value().structRef.pointerCount; }
        uint32_t StructWordSize() const { return static_cast<uint32_t>(StructDataWordSize()) + StructPointerCount(); }

        template <typename T>
        typename T::Reader TryGetStruct(const Word* defaultValue = nullptr) const
        {
            return typename T::Reader(TryGetStruct(defaultValue));
        }

    private:
        const _PtrMem& Value() const
        {
            HE_ASSERT(m_data);
            return *reinterpret_cast<const _PtrMem*>(m_data);
        }

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
            ElementSize elementSize) noexcept
            : m_data(data)
            , m_size(size)
            , m_step(step)
            , m_elementSize(elementSize)
        {}

        bool IsValid() const { return m_data != nullptr; }

        const Word* Data() const { return m_data; }
        uint32_t Size() const { return m_size; }
        uint32_t StepSize() const { return m_step; }
        uint16_t StructDataFieldCount() const;
        uint16_t StructDataWordSize() const { return Tag().StructDataWordSize(); }
        uint16_t StructPointerCount() const { return Tag().StructPointerCount(); }
        ElementSize GetElementSize() const { return m_elementSize; }
        PointerReader Tag() const { HE_ASSERT(m_elementSize == ElementSize::Composite); return PointerReader(m_data - 1); }
        bool IsEmpty() const { return m_size == 0; }

        template <DataType T> T GetDataElement(uint32_t index) const
        {
            HE_ASSERT(IsValid());
            HE_ASSERT(index < Size());
            HE_ASSERT(m_elementSize == ElementSizeOfType<T>::Value);
            return _ReadDataElement<T>(m_data, index, m_step);
        }

        StructReader GetCompositeElement(uint32_t index) const;

        template <typename T> typename T::Reader GetCompositeElement(uint32_t index) const
        {
            return typename T::Reader(GetCompositeElement(index));
        }

        PointerReader GetPointerElement(uint32_t index) const;

    private:
        const Word* m_data{ nullptr };
        uint32_t m_size{ 0 };
        uint32_t m_step{ 0 };
        schema::ElementSize m_elementSize{ ElementSize::Void };
    };

    // --------------------------------------------------------------------------------------------
    class StructReader
    {
    public:
        StructReader() = default;
        StructReader(const Word* data, uint16_t dataWordSize, uint16_t pointerCount) noexcept
            : m_data(data)
            , m_dataWordSize(dataWordSize)
            , m_pointerCount(pointerCount)
            , m_metaWordSize(MetadataWordSize())
        {}

        bool IsValid() const { return m_data != nullptr; }

        const Word* Data() const { return m_data; }
        uint16_t DataWordSize() const { return m_dataWordSize; }
        uint16_t PointerCount() const { return m_pointerCount; }
        uint16_t DataFieldCount() const { HE_ASSERT(IsValid()); return m_dataWordSize == 0 ? 0 : *reinterpret_cast<const uint16_t*>(m_data); }

        // Data fields

        bool HasDataField(uint16_t index) const;

        template <DataType T> T GetDataField(uint32_t dataOffset, T defaultValue = T{}) const
        {
            const uint64_t dataFieldsWordSize = m_dataWordSize - m_metaWordSize;
            return _ReadDataField<T>(DataFields(), dataFieldsWordSize, dataOffset, defaultValue);
        }

        template <DataType T> T TryGetDataField(uint16_t index, uint32_t dataOffset, T defaultValue = T{}) const
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
            if (!IsValid())
                return PointerReader();

            if (index < m_pointerCount) [[likely]]
                return PointerReader(PointerFields() + index);

            return PointerReader();
        }

        // Data Array Fields

        template <DataType T>
        Span<const T> TryGetDataArrayField(uint16_t index, uint32_t dataOffset, uint16_t elementCount, const Word* defaultValue = nullptr) const
        {
            HE_ASSERT(elementCount > 0);

            if (HasDataField(index))
            {
                constexpr uint64_t BitsInType = sizeof(T) * BitsPerByte;
                const uint64_t dataFieldsWordSize = m_dataWordSize - m_metaWordSize;
                if (((dataOffset + elementCount) * BitsInType) <= (dataFieldsWordSize * BitsPerWord)) [[likely]]
                {
                    const T * data = reinterpret_cast<const T*>(DataFields()) + dataOffset;
                    return { data, elementCount };
                }
            }

            const typename List<T>::Reader list = PointerReader(defaultValue).TryGetList<T>();
            return { list.Data(), list.Size() };
        }

        // Pointer Array Fields

        ListReader TryGetPointerArrayField(uint16_t index, uint16_t elementCount, const Word* defaultValue = nullptr) const;

        template <typename T>
        typename List<T>::Reader TryGetPointerArrayField(uint16_t index, uint16_t elementCount, const Word* defaultValue = nullptr) const
        {
            return typename List<T>::Reader(TryGetPointerArrayField(index, elementCount, defaultValue));
        }

        // Operators

        bool operator==(const StructReader& x) const { return m_data == x.m_data; }
        bool operator!=(const StructReader& x) const { return m_data != x.m_data; }

    private:
        uint16_t MetadataWordSize() const
        {
            if (!IsValid() || m_dataWordSize == 0)
                return 0;

            const uint16_t dataFieldCount = *reinterpret_cast<const uint16_t*>(m_data);
            if (dataFieldCount <= 32) [[likely]]
                return 1;

            return 1 + (((dataFieldCount - 32) + (BitsPerWord - 1)) / BitsPerWord);
        }

        const Word* DataFields() const { HE_ASSERT(IsValid()); return m_data + m_metaWordSize; }
        const Word* PointerFields() const { HE_ASSERT(IsValid()); return m_data + m_dataWordSize; }

    private:
        const Word* m_data{ nullptr };

        uint16_t m_dataWordSize{ 0 };
        uint16_t m_pointerCount{ 0 };
        uint16_t m_metaWordSize{ 0 };
    };

    // --------------------------------------------------------------------------------------------
    class Builder
    {
    public:
        explicit Builder(Allocator& allocator = Allocator::GetDefault()) noexcept
            : Builder(256, allocator)
        {}

        explicit Builder(uint32_t initialWordSize, Allocator& allocator = Allocator::GetDefault()) noexcept
            : m_data(allocator)
        {
            Reserve(initialWordSize);
            Clear();
        }

        Builder(const Builder& x) noexcept
            : m_data(x.m_data)
        {}

        Builder(Builder&& x) noexcept
            : m_data(Move(x.m_data))
        {
            x.Clear();
        }

        Builder& operator=(const Builder& x) noexcept
        {
            m_data = x.m_data;
            return *this;
        }

        Builder& operator=(Builder&& x) noexcept
        {
            m_data = Move(x.m_data);
            x.Clear();
            return *this;
        }

        Word* Data() { return m_data.Data(); }
        const Word* Data() const { return m_data.Data(); }
        uint32_t Size() const { return m_data.Size(); }
        uint32_t ByteSize() const { return Size() * BytesPerWord; }

        void Clear()
        {
            m_data.Clear();
            m_data.PushBack(0); // pointer to root
        }

        /// Releases control of the builder's allocated memory and returns ownership to the caller.
        /// The returned memory must be freed by calling @see Allocator::Free using the same
        /// allocator that the vector was constructed with.
        ///
        /// After calling this method the vector is reset to a valid empty state and can be
        /// used again, which creates a new allocation of memory.
        ///
        /// @return The builder's allocated memory.
        Word* Release() { return m_data.Release(); }

        /// Returns a reference to the allocator object used by the vector.
        ///
        /// \return The allocator object this vector uses.
        Allocator& GetAllocator() const { return m_data.GetAllocator(); }

        void Reserve(uint32_t words) { m_data.Reserve(words); }

        void SetRoot(const StructReader& root);
        void SetRoot(const ListReader& root);

        PointerBuilder Root();
        PointerReader Root() const;

        PointerBuilder AddPointer();
        StructBuilder AddStruct(uint16_t dataFieldCount, uint16_t dataWordSize, uint16_t pointerCount);
        ListBuilder AddList(ElementSize elementSize, uint32_t elementCount);
        ListBuilder AddStructList(uint32_t elementCount, uint16_t dataFieldCount, uint16_t dataWordSize, uint16_t pointerCount);
        List<uint8_t>::Builder AddBlob(Span<const uint8_t> data);
        String::Builder AddString(StringView str);

        template <typename T>
        typename T::Builder AddStruct()
        {
            constexpr uint16_t DataFieldCount = T::DataFieldCount;
            constexpr uint16_t DataWordSize = T::DataWordSize;
            constexpr uint16_t PointerCount = T::PointerCount;
            return typename T::Builder(AddStruct(DataFieldCount, DataWordSize, PointerCount));
        }

        template <typename T>
        typename List<T>::Builder AddList(uint32_t elementCount)
        {
            if constexpr (ElementSizeOfType<T>::Value == ElementSize::Composite)
            {
                constexpr uint16_t DataFieldCount = T::DataFieldCount;
                constexpr uint16_t DataWordSize = T::DataWordSize;
                constexpr uint16_t PointerCount = T::PointerCount;
                return typename List<T>::Builder(AddStructList(elementCount, DataFieldCount, DataWordSize, PointerCount));
            }
            else
            {
                return typename List<T>::Builder(AddList(ElementSizeOfType<T>::Value, elementCount));
            }
        }

    private:
        Vector<Word> m_data;
    };

    // --------------------------------------------------------------------------------------------
    class PointerBuilder
    {
    public:
        PointerBuilder() = default;
        PointerBuilder(Builder* builder, uint32_t wordOffset) noexcept
            : m_builder(builder)
            , m_wordOffset(wordOffset)
        {}

        // Common

        bool IsValid() const { return m_builder != nullptr; }

        PointerReader AsReader() const { return PointerReader(Location()); }
        operator PointerReader() const { return AsReader(); }

        Span<uint8_t> AsBytes() { return { reinterpret_cast<uint8_t*>(Location()), BytesPerWord }; }
        Span<const uint8_t> AsBytes() const { return { reinterpret_cast<const uint8_t*>(Location()), BytesPerWord }; }

        Builder* GetBuilder() { return m_builder; }
        const Builder* GetBuilder() const { return m_builder; }

        Word* Location() { return m_builder ? m_builder->Data() + m_wordOffset : nullptr; }
        const Word* Location() const { return m_builder ? m_builder->Data() + m_wordOffset : nullptr; }

        uint32_t WordOffset() const { return m_wordOffset; }

        bool IsNull() const { return !m_builder || (Value().offsetAndKind == 0 && Value().upper32Bits == 0); }
        void SetNull() { Value().offsetAndKind = 0; Value().upper32Bits = 0; }

        bool IsZeroStruct() const { return Value().offsetAndKind == 0xfffffffc && Value().upper32Bits == 0; }
        void SetZeroStruct() { Value().offsetAndKind = 0xfffffffc; Value().upper32Bits = 0; }

        PointerKind Kind() const { return static_cast<PointerKind>(Value().offsetAndKind & 0x03); }
        void SetKind(PointerKind kind) { SetOffsetAndKind(Offset(), kind); }

        int32_t Offset() const { return BitCast<int32_t>(Value().offsetAndKind) >> 2; }
        void SetOffset(int32_t wordOffset) { SetOffsetAndKind(wordOffset, Kind()); }

        const Word* Target() const { return Location() + 1 + Offset(); }
        void SetTarget(const Word* target) { SetTargetAndKind(target, Kind()); }

        void SetOffsetAndKind(int32_t wordOffset, PointerKind kind) { Value().offsetAndKind = (static_cast<uint32_t>(wordOffset) << 2) | (static_cast<uint16_t>(kind) & 0x03); }
        void SetTargetAndKind(const Word* target, PointerKind kind) { SetOffsetAndKind(static_cast<int32_t>(target - Location()) - 1, kind); }

        void Set(const PointerReader& reader);
        void Set(const ListReader& value);
        void Set(const StructReader& value);

        void Copy(const PointerReader& reader);
        void Copy(const ListReader& value);
        void Copy(const StructReader& value);

        // Lists

        ListBuilder TryGetList(ElementSize expectedElementSize) const;

        ElementSize ListElementSize() const { return static_cast<ElementSize>(Value().listRef.elementSizeAndCount & 0x07); }
        void SetListElementSize(ElementSize elementSize) { SetList(elementSize, ListSize()); }

        uint32_t ListSize() const { return Value().listRef.elementSizeAndCount >> 3; }
        void SetListSize(uint32_t size) { SetList(ListElementSize(), size); }

        void SetList(ElementSize elementSize, uint32_t size) { HE_ASSERT(size < 0x1fffffff); Value().listRef.elementSizeAndCount = (size << 3) | static_cast<uint32_t>(elementSize); }

        uint32_t ListCompositeTagSize() const { return Value().offsetAndKind >> 2; }
        void SetListCompositeTagSize(uint32_t size) { HE_ASSERT(size <= 0x3fffffff); Value().offsetAndKind = size << 2; }

        template <typename T> typename List<T>::Builder TryGetList() const;
        String::Builder TryGetString() const;
        Blob::Builder TryGetBlob() const;

        // Structs

        StructBuilder TryGetStruct() const;

        uint16_t StructDataWordSize() const { return Value().structRef.dataWordSize; }
        void SetStructDataWordSize(uint16_t wordSize) { Value().structRef.dataWordSize = wordSize; }

        uint16_t StructPointerCount() const { return Value().structRef.pointerCount; }
        void SetStructPointerCount(uint16_t count) { Value().structRef.pointerCount = count; }

        uint32_t StructWordSize() const { return static_cast<uint32_t>(StructDataWordSize()) + StructPointerCount(); }

        template <typename T>
        typename T::Builder TryGetStruct() const { return typename T::Builder(TryGetStruct()); }

    private:
        const _PtrMem& Value() const { HE_ASSERT(IsValid()); return *reinterpret_cast<const _PtrMem*>(Location()); }
        _PtrMem& Value() { HE_ASSERT(IsValid()); return *reinterpret_cast<_PtrMem*>(Location()); }

    protected:
        schema::Builder* m_builder{ nullptr };
        uint32_t m_wordOffset{ 0 };
    };

    // --------------------------------------------------------------------------------------------
    class ListBuilder
    {
    public:
        ListBuilder() = default;
        ListBuilder(
            Builder* builder,
            uint32_t wordOffset,
            uint32_t size,
            uint32_t step,
            ElementSize elementSize) noexcept
            : m_builder(builder)
            , m_wordOffset(wordOffset)
            , m_size(size)
            , m_step(step)
            , m_structDataFieldCount(0)
            , m_elementSize(elementSize)
        {}
        ListBuilder(
            Builder* builder,
            uint32_t wordOffset,
            uint32_t size,
            uint32_t step,
            uint16_t structDataFieldCount) noexcept
            : m_builder(builder)
            , m_wordOffset(wordOffset)
            , m_size(size)
            , m_step(step)
            , m_structDataFieldCount(structDataFieldCount)
            , m_elementSize(ElementSize::Composite)
        {}

        bool IsValid() const { return m_builder != nullptr; }

        ListReader AsReader() const { return ListReader(Location(), m_size, m_step, m_elementSize); }
        operator ListReader() const { return AsReader(); }

        Span<uint8_t> AsBytes() { return { reinterpret_cast<uint8_t*>(Location()), (m_size * (m_step / BitsPerByte)) }; }
        Span<const uint8_t> AsBytes() const { return { reinterpret_cast<const uint8_t*>(Location()), (m_size * (m_step / BitsPerByte)) }; }

        Builder* GetBuilder() { return m_builder; }
        const Builder* GetBuilder() const { return m_builder; }

        Word* Data() { return Location(); }
        const Word* Data() const { return Location(); }

        Word* Location() { return m_builder ? m_builder->Data() + m_wordOffset : nullptr; }
        const Word* Location() const { return m_builder ? m_builder->Data() + m_wordOffset : nullptr; }

        uint32_t WordOffset() const { return m_wordOffset; }

        uint32_t Size() const { return m_size; }
        uint32_t StepSize() const { return m_step; }
        uint16_t StructDataFieldCount() const { return m_structDataFieldCount; }
        uint16_t StructDataWordSize() const { return Tag().StructDataWordSize(); }
        uint16_t StructPointerCount() const { return Tag().StructPointerCount(); }
        ElementSize GetElementSize() const { return m_elementSize; }
        PointerBuilder Tag() const { HE_ASSERT(m_elementSize == ElementSize::Composite); return PointerBuilder(m_builder, m_wordOffset - 1); }
        bool IsEmpty() const { return m_size == 0; }

        void Copy(const ListReader& reader);

        template <DataType T> T GetDataElement(uint32_t index) const
        {
            HE_ASSERT(IsValid());
            return _ReadDataElement<T>(Data(), index, m_step);
        }

        PointerBuilder GetPointerElement(uint32_t index) const { HE_ASSERT(IsValid()); return PointerBuilder(m_builder, m_wordOffset + index); }

        StructBuilder GetCompositeElement(uint32_t index) const;

        template <typename T> typename T::Builder GetCompositeElement(uint32_t index) const
        {
            HE_ASSERT(IsValid());
            return typename T::Builder(GetCompositeElement(index));
        }

        template <DataType T> void SetDataElement(uint32_t index, T value)
        {
            HE_ASSERT(IsValid());
            HE_ASSERT(m_elementSize == ElementSizeOfType<T>::Value);
            _WriteDataElement<T>(Data(), index, m_step, value);
        }

        void SetPointerElement(uint32_t index, const StructReader& value)
        {
            HE_ASSERT(m_builder);
            HE_ASSERT(m_elementSize == ElementSize::Pointer);
            PointerBuilder(m_builder, m_wordOffset + index).Set(value);
        }

        void SetPointerElement(uint32_t index, const ListReader& value)
        {
            HE_ASSERT(m_builder);
            HE_ASSERT(m_elementSize == ElementSize::Pointer);
            PointerBuilder(m_builder, m_wordOffset + index).Set(value);
        }

    protected:
        schema::Builder* m_builder{ nullptr };
        uint32_t m_wordOffset{ 0 };
        uint32_t m_size{ 0 };
        uint32_t m_step{ 0 };
        uint16_t m_structDataFieldCount{ 0 };
        schema::ElementSize m_elementSize{ ElementSize::Void };
    };

    // --------------------------------------------------------------------------------------------
    class StructBuilder
    {
    public:
        StructBuilder() = default;
        StructBuilder(
            Builder* builder,
            uint32_t wordOffset,
            uint16_t dataFieldCount,
            uint16_t dataWordSize,
            uint16_t pointerCount) noexcept
            : m_builder(builder)
            , m_wordOffset(wordOffset)
            , m_dataFieldCount(dataFieldCount)
            , m_dataWordSize(dataWordSize)
            , m_pointerCount(pointerCount)
            , m_metaWordSize(MetadataWordSize())
        {
            Word* data = DataSection();
            *data |= static_cast<Word>(m_dataFieldCount);

            // If there are data fields then the data word size must be larger than one
            HE_ASSERT(m_dataFieldCount == 0 || (m_dataFieldCount > 0 && m_dataWordSize > 1));

            // The metadata words should always fit within the data section when we have fields
            HE_ASSERT(dataFieldCount == 0 || m_metaWordSize < m_dataWordSize,
                HE_MSG("Data section is not large enough to fit fields and metadata. This is likely a codegen bug."));
        }

        // Common

        bool IsValid() const { return m_builder != nullptr; }

        StructReader AsReader() const { return StructReader(Location(), m_dataWordSize, m_pointerCount); }
        operator StructReader() const { return AsReader(); }

        Span<uint8_t> AsBytes() { return { reinterpret_cast<uint8_t*>(Location()), ((m_dataWordSize + m_pointerCount) * BytesPerWord) }; }
        Span<const uint8_t> AsBytes() const { return { reinterpret_cast<const uint8_t*>(Location()), ((m_dataWordSize + m_pointerCount) * BytesPerWord) }; }

        Builder* GetBuilder() { return m_builder; }
        const Builder* GetBuilder() const { return m_builder; }

        Word* Location() { return m_builder ? m_builder->Data() + m_wordOffset : nullptr; }
        const Word* Location() const { return m_builder ? m_builder->Data() + m_wordOffset : nullptr; }

        uint32_t WordOffset() const { return m_wordOffset; }

        uint16_t DataFieldCount() const { return m_dataFieldCount; }
        uint16_t DataWordSize() const { return m_dataWordSize; }
        uint16_t PointerCount() const { return m_pointerCount; }

        void ClearAllFields()
        {
            ClearDataFields();
            ClearPointerFields();
        }

        void Copy(const StructReader& reader);

        // Data Fields

        bool HasDataField(uint16_t index) const { return AsReader().HasDataField(index); }

        template <DataType T> T GetDataField(uint32_t dataOffset, T defaultValue = T{}) const
        {
            HE_ASSERT(IsValid());
            return AsReader().GetDataField<T>(dataOffset, defaultValue);
        }

        template <DataType T> T TryGetDataField(uint16_t index, uint32_t dataOffset, T defaultValue = T{}) const
        {
            HE_ASSERT(IsValid());
            return AsReader().TryGetDataField<T>(index, dataOffset, defaultValue);
        }

        template <DataType T>
        void SetDataField(uint32_t dataOffset, T value)
        {
            HE_ASSERT(IsValid());
            if constexpr (std::is_same_v<bool, T>)
            {
                HE_ASSERT(dataOffset < (static_cast<uint64_t>(m_dataWordSize - m_metaWordSize) * BitsPerWord));
            }
            else if constexpr (!std::is_same_v<Void, T>)
            {
                HE_ASSERT(((dataOffset + 1) * (sizeof(T) * BitsPerByte)) <= (static_cast<uint64_t>(m_dataWordSize - m_metaWordSize) * BitsPerWord));
            }
            _WriteDataField<T>(DataFields(), dataOffset, value);
        }

        template <DataType T>
        void SetAndMarkDataField(uint16_t index, uint32_t dataOffset, T value)
        {
            if constexpr (!std::is_same_v<Void, T>)
            {
                SetDataField(dataOffset, value);
                MarkHasDataField(index, true);
            }
        }

        void MarkDataField(uint16_t index) { MarkHasDataField(index, true); }

        void ClearDataField(uint16_t index) { MarkHasDataField(index, false); }
        void ClearDataFields()
        {
            Word* data = DataSection();
            MemZero(data, m_metaWordSize * BytesPerWord);
            *data |= static_cast<Word>(m_dataFieldCount);
        }

        template <DataType T>
        typename _ReadDataArrayReturnType<T>::Type GetAndMarkDataArrayField(uint16_t index, uint32_t dataOffset, uint16_t elementCount)
        {
            HE_ASSERT(elementCount > 0);
            typename _ReadDataArrayReturnType<T>::Type ret = _ReadDataArrayField<T>(DataFields(), m_dataWordSize, dataOffset, elementCount);
            if constexpr (!std::is_same_v<decltype(ret), Void>)
            {
                if (!ret.IsEmpty())
                {
                    MarkHasDataField(index, true);
                }
            }
            return ret;
        }

        // Pointer Fields

        bool HasPointerField(uint16_t index) const { return AsReader().HasPointerField(index); }
        PointerBuilder GetPointerField(uint16_t index) const { HE_ASSERT(index < m_pointerCount); return PointerBuilder(m_builder, m_wordOffset + m_dataWordSize + index); }

        void ClearPointerField(uint16_t index) { HE_ASSERT(index < m_pointerCount); PointerSection()[index] = 0; }
        void ClearPointerFields() { MemZero(PointerSection(), m_pointerCount * BytesPerWord); }

        ListBuilder GetPointerArrayField(uint16_t index, uint16_t elementCount) const;

        template <typename T>
        typename List<T>::Builder GetPointerArrayField(uint16_t index, uint16_t elementCount) const
        {
            return typename List<T>::Builder(GetPointerArrayField(index, elementCount));
        }

        // Operators

        bool operator==(const StructBuilder& x) const { return m_builder == x.m_builder && m_wordOffset == x.m_wordOffset; }
        bool operator!=(const StructBuilder& x) const { return m_builder != x.m_builder || m_wordOffset != x.m_wordOffset; }

    protected:
        uint16_t MetadataWordSize() const
        {
            if (!IsValid() || m_dataWordSize == 0)
                return 0;

            if (m_dataFieldCount <= 32) [[likely]]
                return 1;

            return 1 + (((m_dataFieldCount - 32) + (BitsPerWord - 1)) / BitsPerWord);
        }

        Word* DataSection() { HE_ASSERT(IsValid()); return Location(); }
        const Word* DataSection() const { HE_ASSERT(IsValid()); return Location(); }

        Word* PointerSection() { return DataSection() + m_dataWordSize; }
        const Word* PointerSection() const { return DataSection() + m_dataWordSize; }

        Word* DataFields() { return DataSection() + m_metaWordSize; }
        const Word* DataFields() const { return DataSection() + m_metaWordSize; }

        void MarkHasDataField(uint16_t index, bool value)
        {
            HE_ASSERT(IsValid());
            HE_ASSERT(index < m_dataFieldCount);

            if (index <= 32) [[likely]]
            {
                uint32_t* mask = reinterpret_cast<uint32_t*>(DataSection()) + 1;
                *mask = (*mask & ~(1 << index)) | (static_cast<uint8_t>(value) << index);
            }
            else
            {
                index -= 32;
                const uint32_t maskIndex = index / BitsPerWord;
                const uint32_t maskShift = index % BitsPerWord;
                uint64_t* mask = DataSection() + 1 + maskIndex;
                *mask = (*mask & ~(1ull << maskShift)) | (static_cast<Word>(value) << maskShift);
            }
        }

    protected:
        schema::Builder* m_builder{ nullptr };
        uint32_t m_wordOffset{ 0 };
        uint16_t m_dataFieldCount{ 0 };
        uint16_t m_dataWordSize{ 0 };
        uint16_t m_pointerCount{ 0 };
        uint16_t m_metaWordSize{ 0 };
    };

    // --------------------------------------------------------------------------------------------
    class String::Reader : public ListReader
    {
    public:
        Reader() = default;
        explicit Reader(ListReader reader) noexcept : ListReader(reader) {}

        const char* Data() const { return reinterpret_cast<const char*>(ListReader::Data()); }
        uint32_t Size() const { return ListReader::Size() == 0 ? 0 : ListReader::Size() - 1; }

        StringView AsView() const { return StringView{ Data(), Size() }; }
        operator StringView() const { return AsView(); }

        char operator[](uint32_t index) const
        {
            HE_ASSERT(index < Size());
            return Data()[index];
        }

        const char* begin() const { return Data(); }
        const char* end() const { return Data() + Size(); }
    };

    class String::Builder : public ListBuilder
    {
    public:
        Builder() = default;
        explicit Builder(ListBuilder builder) noexcept : ListBuilder(builder) {}

        char* Data() { return reinterpret_cast<char*>(Location()); }
        const char* Data() const { return reinterpret_cast<const char*>(Location()); }

        uint32_t Size() const { return ListBuilder::Size() == 0 ? 0 : ListBuilder::Size() - 1; }

        typename String::Reader AsReader() const { return String::Reader(ListBuilder::AsReader()); }
        operator typename String::Reader() const { return AsReader(); }

        StringView AsView() const { return StringView{ Data(), Size() }; }
        operator StringView() const { return AsView(); }

        char operator[](uint32_t index) const
        {
            HE_ASSERT(index < Size());
            return Data()[index];
        }

        char& operator[](uint32_t index)
        {
            HE_ASSERT(index < Size());
            return Data()[index];
        }

        char* begin() { return Data(); }
        const char* begin() const { return Data(); }

        char* end() { return Data() + Size(); }
        const char* end() const { return Data() + Size(); }
    };

    inline bool operator==(const String::Reader a, const String::Reader b) { return a.Size() == b.Size() && MemEqual(a.Data(), b.Data(), a.Size()); }
    inline bool operator!=(const String::Reader a, const String::Reader b) { return a.Size() != b.Size() || !MemEqual(a.Data(), b.Data(), a.Size()); }

    // --------------------------------------------------------------------------------------------
    template <typename T>
    class ListIterator
    {
    public:
        using ListType = T;
        using ElementType = typename T::ElementType;

        using difference_type   = uint32_t;
        using value_type        = ElementType;
        using pointer           = const value_type*;
        using reference         = const value_type&;
        using iterator_category = std::random_access_iterator_tag;
        using _Unchecked_type   = ListIterator; // Mark iterator as checked.

    public:
        ListIterator() = default;
        ListIterator(const T* list, uint32_t index) : m_list(list), m_index(index) {}

        ElementType operator*() const { return m_list->Get(m_index); }
        ElementType operator->() const { return m_list->Get(m_index); }

        ListIterator& operator++() { ++m_index; return *this; }
        ListIterator operator++(int) { ListIterator x = *this; ++m_index; return x; }
        ListIterator& operator--() { --m_index; return *this; }
        ListIterator operator--(int) { ListIterator x = *this; --m_index; return x; }

        friend size_t operator-(const ListIterator& lhs, const ListIterator& rhs)
        {
            HE_ASSERT(lhs.m_list == rhs.m_list);
            return lhs.m_index - rhs.m_index;
        }

        bool operator==(const ListIterator& x) const { return m_list == x.m_list && m_index == x.m_index; }
        bool operator!=(const ListIterator& x) const { return m_list != x.m_list || m_index != x.m_index; }

    private:
        const T* m_list{ nullptr };
        uint32_t m_index{ 0 };
    };

    // --------------------------------------------------------------------------------------------
    template <typename T>
    class List<T>::Reader : public ListReader
    {
    public:
        using ElementType = typename LayoutTraits<T>::Reader;
        using IteratorType = ListIterator<List<T>::Reader>;

    public:
        Reader() = default;
        explicit Reader(ListReader reader) noexcept : ListReader(reader) {}

        const T* Data() const requires(DataType<T>) { return reinterpret_cast<const T*>(ListReader::Data()); }

        ElementType Get(uint32_t index) const
        {
            HE_ASSERT(index < Size());
            if constexpr (DataType<T>)
                return GetDataElement<T>(index);
            else if constexpr (std::is_same_v<T, String>)
                return GetPointerElement(index).TryGetString();
            else if constexpr (IsSpecialization<T, List>)
                return GetPointerElement(index).TryGetList<typename T::ElementType>();
            else if constexpr (ElementSizeOfType<T>::Value == ElementSize::Composite)
                return GetCompositeElement<T>(index);
            else
                return GetPointerElement(index).TryGetStruct<T>();
        }

        ElementType operator[](uint32_t index) const { return Get(index); }

        IteratorType begin() const { return IteratorType(this, 0); }
        IteratorType end() const { return IteratorType(this, Size()); }
    };

    template <typename T>
    class List<T>::Builder : public ListBuilder
    {
    public:
        using ElementType = typename LayoutTraits<T>::Builder;
        using IteratorType = ListIterator<List<T>::Builder>;

    public:
        Builder() = default;
        explicit Builder(ListBuilder builder) noexcept : ListBuilder(builder) {}

        T* Data() requires(DataType<T>) { return reinterpret_cast<T*>(ListBuilder::Data()); }
        const T* Data() const requires(DataType<T>) { return reinterpret_cast<const T*>(ListBuilder::Data()); }

        ElementType Get(uint32_t index) const
        {
            HE_ASSERT(index < Size());
            if constexpr (DataType<T>)
                return GetDataElement<T>(index);
            else if constexpr (std::is_same_v<T, String>)
                return GetPointerElement(index).TryGetString();
            else if constexpr (IsSpecialization<T, List>)
                return GetPointerElement(index).TryGetList<typename T::ElementType>();
            else if constexpr (ElementSizeOfType<T>::Value == ElementSize::Composite)
                return GetCompositeElement<T>(index);
            else
                return GetPointerElement(index).TryGetStruct<T>();
        }

        void Set(uint32_t index, typename LayoutTraits<T>::Reader reader)
        {
            HE_ASSERT(index < Size());
            if constexpr (DataType<T>)
                return SetDataElement<T>(index, reader);
            else if constexpr (ElementSizeOfType<T>::Value == ElementSize::Composite)
                return GetCompositeElement(index).Copy(reader);
            else
                return SetPointerElement(index, reader);
        }

        typename List<T>::Reader AsReader() const { return List<T>::Reader(ListBuilder::AsReader()); }
        operator typename List<T>::Reader() const { return AsReader(); }

        ElementType operator[](uint32_t index) const { return Get(index); }

        IteratorType begin() const { return IteratorType(this, 0); }
        IteratorType end() const { return IteratorType(this, Size()); }
    };

    // --------------------------------------------------------------------------------------------
    template <typename T>
    inline typename List<T>::Reader PointerReader::TryGetList(const Word* defaultValue) const
    {
        return typename List<T>::Reader(TryGetList(ElementSizeOfType<T>::Value, defaultValue));
    }

    inline String::Reader PointerReader::TryGetString(const Word* defaultValue) const
    {
        return String::Reader(TryGetList(ElementSize::Byte, defaultValue));
    }

    inline Blob::Reader PointerReader::TryGetBlob(const Word* defaultValue) const
    {
        return Blob::Reader(TryGetList(ElementSize::Byte, defaultValue));
    }

    // --------------------------------------------------------------------------------------------
    template <typename T>
    inline typename List<T>::Builder PointerBuilder::TryGetList() const
    {
        return typename List<T>::Builder(TryGetList(ElementSizeOfType<T>::Value));
    }

    inline String::Builder PointerBuilder::TryGetString() const
    {
        return String::Builder(TryGetList(ElementSize::Byte));
    }

    inline Blob::Builder PointerBuilder::TryGetBlob() const
    {
        return Blob::Builder(TryGetList(ElementSize::Byte));
    }

    // --------------------------------------------------------------------------------------------
    template <typename T>
    inline typename T::Reader ReadRoot(const Word* data)
    {
        PointerReader ptr(data);
        return ptr.TryGetStruct<T>();
    }

    // --------------------------------------------------------------------------------------------
    template <typename T>
    struct TypedBuilder
    {
        Builder builder{};

        T::Builder Root()
        {
            PointerBuilder ptr = builder.Root();
            if (ptr.IsNull())
                return builder.AddStruct<T>();

            return ptr.TryGetStruct<T>();
        }

        T::Builder operator*() const { return Root(); }
        T::Builder operator->() const { return Root(); }
    };
}
