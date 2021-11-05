// Copyright Chad Engler

// The buffer utilites assume LE architecture and efficient unaligned memory access.

#pragma once

#include "he/core/assert.h"
#include "he/core/buffer_writer.h"
#include "he/core/compiler.h"
#include "he/core/config.h"
#include "he/core/memory_ops.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/core/vector.h"

#include <concepts>
#include <list>
#include <unordered_map>
#include <unordered_set>

namespace he::schema
{
    // --------------------------------------------------------------------------------------------
    struct BufferOffset { uint32_t val; };

    // --------------------------------------------------------------------------------------------
    template <typename T>
    class RelPointer
    {
    public:
        template <typename U>
        const RelPointer<U>& Cast() const { return *reinterpret_cast<const RelPointer<U>*>(this); }

        uint32_t GetOffset() const { return *reinterpret_cast<const uint32_t*>(m_data); }
        bool IsNull() const { return GetOffset() == 0; }

        const T* Get() const { return reinterpret_cast<const T*>(m_data - GetOffset()); }

        const T* operator->() const { HE_ASSERT(!IsNull()); return Get(); }

        operator bool() const { return !IsNull(); }
        operator RelPointer<void>&() const { return Cast<void>(); }

    private:
        RelPointer(const RelPointer&) = delete;
        RelPointer(RelPointer&&) = delete;

        RelPointer& operator=(const RelPointer&) = delete;
        RelPointer& operator=(RelPointer&&) = delete;

    private:
        uint8_t m_data[1];
    };

    // --------------------------------------------------------------------------------------------
    template <typename T, std::unsigned_integral S = uint32_t>
    class VectorReader
    {
    public:
        using ElementType = T;
        using SizeType = S;

        S Size() const { return *reinterpret_cast<const S*>(m_data); }
        const T* Data() const { return reinterpret_cast<const T*>(m_data + 4); }

        bool IsEmpty() const { return Size() == 0; }

        const T& Get(uint32_t index) const { HE_ASSERT(index < Size()); return Data()[index]; }
        const T& operator[](uint32_t index) const {  return Get(index); }

        const T* begin() const { return Data(); }
        const T* end() const { return Data() + Size(); }

    private:
        VectorReader(const VectorReader&) = delete;
        VectorReader(VectorReader&&) = delete;

        VectorReader& operator=(const VectorReader&) = delete;
        VectorReader& operator=(VectorReader&&) = delete;

    private:
        uint8_t m_data[1];
    };

    // --------------------------------------------------------------------------------------------
    template <typename T>
    class ArrayReader : public VectorReader<T> { };

    template <typename T>
    class SetReader : public VectorReader<T> { };

    template <typename T>
    class ListReader : public VectorReader<T> { };

    class StringReader : public VectorReader<char> { };

    template <typename K, typename V>
    struct MapReaderKV { using KeyType = K; using ValueType = V; K key; V value; };

    template <typename K, typename V>
    class MapReader : public VectorReader<MapReaderKV<K, V>> { using KeyType = K; using ValueType = V; };

    // --------------------------------------------------------------------------------------------
    class UnionReader
    {
    public:
        uint32_t GetTagId() const { return *reinterpret_cast<const uint32_t*>(m_data); }
        bool IsUnset() const { return GetTagId() == 0; }

        template <typename T>
        const T* Get() const
        {
            if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
                return reinterpret_cast<const T*>(m_data + 4);
            else
                return reinterpret_cast<const RelPointer<T>*>(m_data + 4)->Get();
        }

        operator bool() const { return !IsUnset(); }

    private:
        UnionReader(const UnionReader&) = delete;
        UnionReader(UnionReader&&) = delete;

        UnionReader& operator=(const UnionReader&) = delete;
        UnionReader& operator=(UnionReader&&) = delete;

    private:
        uint8_t m_data[1];
    };

    // --------------------------------------------------------------------------------------------
    class StructureReader
    {
    public:
        template <typename T>
        const T* GetFieldPointer(uint32_t fieldId) const
        {
            const RelPointer<void>* field = GetFieldRelPointer(fieldId);
            return field ? field->Cast<T>().Get() : nullptr;
        }

        bool FieldHasValue(uint32_t fieldId) const
        {
            const RelPointer<void>* field = GetFieldRelPointer(fieldId);
            return field != nullptr && !field->IsNull();
        }

    public:
        struct VTableEntry { uint32_t id; uint32_t offset; };
        static_assert(alignof(VTableEntry) == alignof(uint32_t));
        static_assert(sizeof(VTableEntry) == (sizeof(uint32_t) * 2));

        using VTable = VectorReader<VTableEntry, uint16_t>;

        const VTable& GetVTable() const { return *reinterpret_cast<const VTable*>(m_data); }

        const RelPointer<void>* GetFieldRelPointer(uint32_t fieldId) const
        {
            const VTable& vtable = GetVTable();
            for (const VTableEntry& entry : vtable)
            {
                if (entry.id == fieldId)
                    return reinterpret_cast<const RelPointer<void>*>(&entry.offset);
            }
            return nullptr;
        }

    private:
        StructureReader(const StructureReader&) = delete;
        StructureReader(StructureReader&&) = delete;

        StructureReader& operator=(const StructureReader&) = delete;
        StructureReader& operator=(StructureReader&&) = delete;

    private:
        uint8_t m_data[1];
    };

    // --------------------------------------------------------------------------------------------
    constexpr uint32_t BufferHeaderSize = 12;

    // --------------------------------------------------------------------------------------------
    class BufferReader
    {
    public:
        BufferReader(const void* data, uint32_t size);

        bool Verify(const char (&signature)[4]) const;

        uint32_t GetSignature() const { return *reinterpret_cast<const uint32_t*>(m_data); }
        StringView GetSignatureStr() const { return { reinterpret_cast<const char*>(m_data), 4 }; }

        uint16_t GetVersion() const { return *reinterpret_cast<const uint16_t*>(m_data + 4); }
        uint16_t GetReserved() const { return *reinterpret_cast<const uint16_t*>(m_data + 6); }
        uint32_t GetRootOffset() const { return *reinterpret_cast<const uint32_t*>(m_data + 8); }

        template <std::derived_from<StructureReader> T>
        const T* GetRoot() const
        {
            const uint32_t offset = GetRootOffset();
            return offset ? reinterpret_cast<const T*>(m_data + offset) : nullptr;
        }

    private:
        const uint8_t* const m_data;
        const uint32_t m_size;
    };

    // --------------------------------------------------------------------------------------------
    template <typename T>
    concept InlineBufferType = std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_same_v<T, BufferOffset>;

    class BufferBuilder
    {
    public:
        explicit BufferBuilder(BufferWriter& writer, const char signature[4] = nullptr);

        void SetRoot(BufferOffset root)
        {
            HE_ASSERT(root.val >= BufferHeaderSize);
            m_writer.WriteAt(8, root.val - 8);
        }

        void StartVTable();
        void AddVTableField(uint32_t fieldId, BufferOffset offset);
        BufferOffset EndVTable();

        BufferOffset WriteSequence(const void* data, uint32_t len, uint32_t elementSize);

        void StartSequence();
        void AddSequenceElement(const void* data, uint32_t size);
        BufferOffset EndSequence();

        BufferOffset WriteString(const char* str);
        BufferOffset WriteString(StringView str);

        template <InlineBufferType T>
        void AddSequenceElement(T value)
        {
            HE_ASSERT(m_sequenceStartOffset > 0);
            WriteValue(value);
            ++m_sequenceElementCount;
        }

        template <InlineBufferType K, InlineBufferType V>
        void AddSequenceElement(K key, V value)
        {
            HE_ASSERT(m_sequenceStartOffset > 0);
            WriteValue(key);
            WriteValue(value);
            ++m_sequenceElementCount;
        }

        template <typename T> requires(std::is_arithmetic_v<T>)
        BufferOffset WriteValue(T value)
        {
            HE_ASSERT(m_vtableStartOffset == 0);
            const BufferOffset ret{ m_writer.Size() };
            m_writer.Write(value);
            return ret;
        }

        template <Enum T>
        BufferOffset WriteValue(T value)
        {
            HE_ASSERT(m_vtableStartOffset == 0);
            const BufferOffset ret{ m_writer.Size() };
            m_writer.Write(static_cast<std::underlying_type_t<T>>(value));
            return ret;
        }

        BufferOffset WriteValue(BufferOffset offset)
        {
            HE_ASSERT(m_vtableStartOffset == 0);
            const BufferOffset ret{ m_writer.Size() };
            m_writer.Write(m_writer.Size() - offset.val);
            return ret;
        }

    private:
        BufferWriter& m_writer;

        uint32_t m_vtableStartOffset{ 0 };
        uint32_t m_vtableFieldCount{ 0 };

        uint32_t m_sequenceStartOffset{ 0 };
        uint32_t m_sequenceElementCount{ 0 };
    };

    // --------------------------------------------------------------------------------------------
    // Serialization to a Buffer

    template <typename T>
    BufferOffset ToBuffer(BufferBuilder& builder, const T& value);

    // Serialization of arithmetic, enum, and offset types.
    template <InlineBufferType T>
    BufferOffset ToBuffer(BufferBuilder& builder, const T& value)
    {
        return builder.WriteValue(value);
    }

    // Serialization of vectors of inline values.
    template <InlineBufferType T>
    BufferOffset ToBuffer(BufferBuilder& builder, const Vector<T>& value)
    {
        return builder.WriteSequence(value.Data(), value.Size(), sizeof(T));
    }

    // Serialization of vectors of object values.
    template <typename T>
    BufferOffset ToBuffer(BufferBuilder& builder, const Vector<T>& value)
    {
        Vector<BufferOffset> offsets(Allocator::GetTemp());
        offsets.Reserve(value.Size());
        for (const T& item : value)
        {
            offsets.PushBack(ToBuffer(builder, item));
        }
        return ToBuffer(builder, offsets);
    }

    // Serialization of fixed size arrays of inline values.
    template <typename T> requires(std::is_array_v<T> && InlineBufferType<ArrayElementType<T>>)
    BufferOffset ToBuffer(BufferBuilder& builder, const T& value)
    {
        constexpr uint32_t N = HE_LENGTH_OF(value);
        return builder.WriteSequence(value, N, sizeof(ArrayElementType<T>));
    }

    // Serialization of fixed size arrays of object values.
    template <typename T> requires(std::is_array_v<T>)
    BufferOffset ToBuffer(BufferBuilder& builder, const T& value)
    {
        constexpr uint32_t N = HE_LENGTH_OF(value);
        BufferOffset offsets[N];
        for (uint32_t i = 0; i < N; ++i)
        {
            offsets[i] = ToBuffer(builder, value[i]);
        }
        return ToBuffer(builder, offsets);
    }

    // Serialization of lists of inline values.
    template <InlineBufferType T>
    BufferOffset ToBuffer(BufferBuilder& builder, const std::list<T>& value)
    {
        builder.StartSequence();
        for (const T& item : value)
        {
            builder.AddSequenceElement(item);
        }
        return builder.EndSequence();
    }

    // Serialization of lists of object values.
    template <typename T>
    BufferOffset ToBuffer(BufferBuilder& builder, const std::list<T>& value)
    {
        Vector<BufferOffset> offsets(Allocator::GetTemp());
        offsets.Reserve(value.size());
        for (const T& item : value)
        {
            offsets.PushBack(ToBuffer(builder, item));
        }
        return ToBuffer(builder, offsets);
    }

    // Serialization of maps of inline values.
    template <InlineBufferType K, InlineBufferType V>
    BufferOffset ToBuffer(BufferBuilder& builder, const std::unordered_map<K, V>& value)
    {
        builder.StartSequence();
        for (auto&& it : value)
        {
            builder.AddSequenceElement(it.first, it.second);
        }
        return builder.EndSequence();
    }

    // Serialization of maps of object values.
    template <InlineBufferType K, typename V>
    BufferOffset ToBuffer(BufferBuilder& builder, const std::unordered_map<K, V>& value)
    {
        Vector<BufferOffset> offsets(Allocator::GetTemp());
        offsets.Reserve(value.size());
        for (auto&& it : value)
        {
            offsets.PushBack(ToBuffer(builder, it.second));
        }

        uint32_t i = 0;
        builder.StartSequence();
        for (auto&& it : value)
        {
            builder.AddSequenceElement(it.first, offsets[i++]);
        }
        return builder.EndSequence();
    }

    // Serialization of sets of inline values.
    template <InlineBufferType T>
    BufferOffset ToBuffer(BufferBuilder& builder, const std::unordered_set<T>& value)
    {
        builder.StartSequence();
        for (const T& item : value)
        {
            builder.AddSequenceElement(item);
        }
        return builder.EndSequence();
    }

    // Serialization of sets of object values.
    template <typename T>
    BufferOffset ToBuffer(BufferBuilder& builder, const std::unordered_set<T>& value)
    {
        Vector<BufferOffset> offsets(Allocator::GetTemp());
        offsets.Reserve(value.size());
        for (const T& item : value)
        {
            offsets.PushBack(ToBuffer(builder, item));
        }
        return ToBuffer(builder, offsets);
    }

    // Serialization of pointers.
    template <typename T>
    BufferOffset ToBuffer(BufferBuilder& builder, const UniquePtr<T>& value)
    {
        if (!value)
            return BufferOffset{ 0 };
        return ToBuffer(builder, *value);
    }

    // Serialization of strings.
    template <>
    inline BufferOffset ToBuffer(BufferBuilder& builder, const String& value)
    {
        return builder.WriteString(value);
    }
}
