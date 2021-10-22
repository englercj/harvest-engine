// Copyright Chad Engler

#include "he/schema/buffer.h"

#include "he/core/string.h"

namespace he::schema
{
    BufferReader::BufferReader(const void* data, uint32_t size)
        : m_data(static_cast<const uint8_t*>(data))
        , m_size(size)
    {}

    bool BufferReader::Verify(const char(&signature)[4]) const
    {
        if (m_size < BufferHeaderSize)
            return false;

        if (!MemEqual(signature, m_data, 4))
            return false;

        if (GetVersion() != 1)
            return false;

        const uint32_t rootOffset = GetRootOffset();
        if (rootOffset != 0 && rootOffset < 4)
            return false;

        // Minimum size to fit the header and an empty vtable of a structure
        if (rootOffset > 0 && m_size < (BufferHeaderSize + 2))
            return false;

        return true;
    }

    BufferBuilder::BufferBuilder(BufferWriter& writer)
        : m_writer(writer)
    {}

    void BufferBuilder::WriteHeader(const char (&signature)[4])
    {
        HE_ASSERT(m_writer.Size() == 0);
        m_writer.Write(signature, 4);
        m_writer.Write(uint16_t(1));
        m_writer.Write(uint16_t(0));
        m_writer.Write(uint32_t(0));
    }

    void BufferBuilder::StartVTable()
    {
        HE_ASSERT(m_writer.Size() >= BufferHeaderSize);
        HE_ASSERT(m_vtableStartOffset == 0);
        m_vtableStartOffset = m_writer.Size();
        m_writer.Write(uint32_t(0)); // Placeholder for the v-table field count
    }

    void BufferBuilder::AddVTableField(uint32_t fieldId, Offset<void> offset)
    {
        HE_ASSERT(m_vtableStartOffset > 0);
        HE_ASSERT(offset.val < m_vtableStartOffset);

        // A zero offset is invalid and means the field is not set. Just skip it in the vtable.
        if (offset.val == 0)
            return;

        m_writer.Write(fieldId);
        m_writer.Write(m_writer.Size() - offset.val);
        ++m_vtableFieldCount;
    }

    Offset<void> BufferBuilder::EndVTable()
    {
        HE_ASSERT(m_vtableStartOffset > 0);

        const uint32_t offset = m_vtableStartOffset;
        m_vtableStartOffset = 0;

        m_writer.WriteAt(offset, m_vtableFieldCount);
        m_vtableFieldCount = 0;

        return { offset };
    }

    Offset<void> BufferBuilder::WriteSequence(const void* data, uint32_t len, uint32_t elementSize)
    {
        HE_ASSERT(m_vtableStartOffset == 0);
        const Offset<void> ret{ m_writer.Size() };
        m_writer.Write(len);
        m_writer.Write(data, len * elementSize);
        return ret;
    }

    void BufferBuilder::StartSequence()
    {
        HE_ASSERT(m_vtableStartOffset == 0);
        HE_ASSERT(m_sequenceStartOffset == 0);
        m_sequenceStartOffset = m_writer.Size();
        m_writer.Write(uint32_t(0)); // Placeholder for the sequence element count
    }

    void BufferBuilder::AddSequenceElement(const void* data, uint32_t size)
    {
        HE_ASSERT(m_sequenceStartOffset > 0);
        m_writer.Write(data, size);
        ++m_sequenceElementCount;
    }

    Offset<void> BufferBuilder::EndSequence()
    {
        HE_ASSERT(m_sequenceStartOffset > 0);

        const uint32_t offset = m_sequenceStartOffset;
        m_sequenceStartOffset = 0;

        m_writer.WriteAt(offset, m_sequenceElementCount);
        m_sequenceElementCount = 0;

        return { offset };
    }

    Offset<String> BufferBuilder::WriteString(const char* str)
    {
        const uint32_t len = he::String::Length(str);
        const Offset<void> ret = WriteSequence(str, len, 1);
        m_writer.Write('\0');
        return ret.Cast<String>();
    }

    Offset<String> BufferBuilder::WriteString(StringView str)
    {
        const Offset<void> ret = WriteSequence(str.Data(), str.Size(), 1);
        m_writer.Write('\0');
        return ret.Cast<String>();
    }
}
