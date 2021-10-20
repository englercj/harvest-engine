// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/buffer_writer.h"
#include "he/core/compiler.h"
#include "he/core/config.h"
#include "he/core/memory_ops.h"
#include "he/core/string_view.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"

#include <concepts>
#include <type_traits>

namespace he::schema
{
    // --------------------------------------------------------------------------------------------
    template <typename T>
    struct Offset
    {
        template <typename U>
        Offset<U> Cast() const { return Offset<U>{ val }; }

        operator Offset<void>() const { return Offset<void>{ val }; }

        uint32_t val;
    };

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
    class Vector
    {
    public:
        S Size() const { return *reinterpret_cast<const S*>(m_data); }
        const T* Data() const { return reinterpret_cast<const T*>(m_data + 4); }

        bool IsEmpty() const { return Size() == 0; }

        const T& Get(uint32_t index) const { HE_ASSERT(index < Size()); return Data()[index]; }
        const T& operator[](uint32_t index) const {  return Get(index); }

        const T* begin() const { return Data(); }
        const T* end() const { return Data() + Size(); }

    private:
        Vector(const Vector&) = delete;
        Vector(Vector&&) = delete;

        Vector& operator=(const Vector&) = delete;
        Vector& operator=(Vector&&) = delete;

    private:
        uint8_t m_data[1];
    };

    // --------------------------------------------------------------------------------------------
    template <typename T>
    class Array : public Vector<T> { };

    template <typename T>
    class Set : public Vector<T> { };

    template <typename T>
    class List : public Vector<T> { };

    class String : public Vector<char> { };

    template <typename K, typename V>
    struct MapKV { K key; V value; };

    template <typename K, typename V>
    class Map : public Vector<MapKV<K, V>> { };

    // --------------------------------------------------------------------------------------------
    class Union
    {
    public:
        uint32_t GetFieldId() const { return *reinterpret_cast<const uint32_t*>(m_data); }
        bool IsUnset() const { return GetFieldId() == 0; }

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
        Union(const Union&) = delete;
        Union(Union&&) = delete;

        Union& operator=(const Union&) = delete;
        Union& operator=(Union&&) = delete;

    private:
        uint8_t m_data[1];
    };

    // --------------------------------------------------------------------------------------------
    class Structure
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

        using VTable = Vector<VTableEntry, uint16_t>;

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
        Structure(const Structure&) = delete;
        Structure(Structure&&) = delete;

        Structure& operator=(const Structure&) = delete;
        Structure& operator=(Structure&&) = delete;

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

        template <std::derived_from<Structure> T>
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
    class BufferBuilder
    {
    public:
        explicit BufferBuilder(BufferWriter& writer);

        void Reserve(uint32_t len) { m_writer.Reserve(len); }

        void WriteHeader(const char (&signature)[4]);

        template <std::derived_from<Structure> T>
        void SetRoot(Offset<T> root)
        {
            HE_ASSERT(root.val >= BufferHeaderSize);
            m_writer.WriteAt(8, root.val - 8);
        }

        void StartVTable();
        void AddVTableField(uint32_t fieldId, Offset<void> offset);
        Offset<void> EndVTable();

        Offset<void> WriteSequence(const void* data, uint32_t len, uint32_t elementSize);

        void StartSequence();
        void AddSequenceElement(const void* data, uint32_t size);
        Offset<void> EndSequence();

        Offset<String> WriteString(const char* str);
        Offset<String> WriteString(StringView str);

        template <typename T>
        void AddSequenceElement(T value)
        {
            HE_ASSERT(m_sequenceStartOffset > 0);
            WriteValue(value);
            ++m_sequenceElementCount;
        }

        template <typename K, typename V>
        void AddSequenceElement(K key, V value)
        {
            HE_ASSERT(m_sequenceStartOffset > 0);
            WriteValue(key);
            WriteValue(value);
            ++m_sequenceElementCount;
        }

        template <typename T> requires(std::is_arithmetic_v<T>)
        Offset<T> WriteValue(T value)
        {
            HE_ASSERT(m_vtableStartOffset == 0);
            const Offset<T> ret{ m_writer.Size() };
            m_writer.Write(value);
            return ret;
        }

        template <Enum T>
        Offset<T> WriteValue(T value)
        {
            HE_ASSERT(m_vtableStartOffset == 0);
            const Offset<T> ret{ m_writer.Size() };
            m_writer.Write(static_cast<std::underlying_type_t<T>>(value));
            return ret;
        }

        template <typename T>
        Offset<T> WriteValue(Offset<T> offset)
        {
            HE_ASSERT(m_vtableStartOffset == 0);
            const Offset<T> ret{ m_writer.Size() };
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
}
