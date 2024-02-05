// Copyright Chad Engler

#include "he/core/buffer_writer.h"

#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/memory_ops.h"
#include "he/core/string_ops.h"
#include "he/core/utils.h"

namespace he
{
    BufferWriter::BufferWriter(Allocator& allocator) noexcept
        : m_allocator(allocator)
        , m_strategy(GrowthStrategy::Factor)
        , m_growth(0.5f)
    {}

    BufferWriter::BufferWriter(GrowthStrategy strategy, float growth, Allocator& allocator) noexcept
        : m_allocator(allocator)
        , m_strategy(strategy)
        , m_growth(growth)
    {}

    BufferWriter::BufferWriter(const BufferWriter& x, Allocator& allocator) noexcept
        : m_allocator(allocator)
    {
        CopyFrom(x);
    }

    BufferWriter::BufferWriter(BufferWriter&& x, Allocator& allocator) noexcept
        : m_allocator(allocator)
    {
        MoveFrom(Move(x));
    }

    BufferWriter::BufferWriter(const BufferWriter& x) noexcept
        : m_allocator(x.m_allocator)
    {
        CopyFrom(x);
    }

    BufferWriter::BufferWriter(BufferWriter&& x) noexcept
        : m_allocator(x.m_allocator)
    {
        MoveFrom(Move(x));
    }

    BufferWriter::~BufferWriter() noexcept
    {
        if (m_data)
        {
            m_allocator.Free(m_data);
        }
    }

    BufferWriter& BufferWriter::operator=(const BufferWriter& x) noexcept
    {
        CopyFrom(x);
        return *this;
    }

    BufferWriter& BufferWriter::operator=(BufferWriter&& x) noexcept
    {
        MoveFrom(Move(x));
        return *this;
    }

    const uint8_t& BufferWriter::operator[](uint32_t index) const
    {
        HE_ASSERT(index < m_size);
        return m_data[index];
    }

    uint32_t BufferWriter::Capacity() const
    {
        return m_capacity;
    }

    uint32_t BufferWriter::Size() const
    {
        return m_size;
    }

    void BufferWriter::Reserve(uint32_t len)
    {
        if (len <= m_capacity)
            return;

        len = Max(MinBytes, len);

        m_data = static_cast<uint8_t*>(m_allocator.Realloc(m_data, len));
        m_capacity = len;
    }

    void BufferWriter::Resize(uint32_t len)
    {
        Reserve(len);
        m_size = len;
    }

    void BufferWriter::ShrinkToFit()
    {
        m_data = static_cast<uint8_t*>(m_allocator.Realloc(m_data, m_size));
        m_capacity = m_size;
    }

    const uint8_t* BufferWriter::Data() const
    {
        return m_data;
    }

    uint8_t* BufferWriter::Release()
    {
        uint8_t* data = m_data;

        m_data = nullptr;
        m_size = 0;
        m_capacity = 0;

        return data;
    }

    void BufferWriter::Clear()
    {
        m_size = 0;
    }

    void BufferWriter::Write(const void* data, uint32_t len)
    {
        GrowBy(len);
        MemCopy(m_data + m_size, data, len);
        m_size += len;
    }

    void BufferWriter::WriteAt(uint32_t offset, const void* data, uint32_t len)
    {
        HE_ASSERT(len < m_size && offset < (m_size - len));
        MemCopy(m_data + offset, data, len);
    }

    void BufferWriter::Write(const char* str)
    {
        const uint32_t len = StrLen(str);
        Write(str, len);
    }

    void BufferWriter::WriteAt(uint32_t offset, const char* str)
    {
        const uint32_t len = StrLen(str);
        WriteAt(offset, str, len);
    }

    void BufferWriter::WriteRepeat(uint8_t byte, uint32_t count)
    {
        GrowBy(count);
        MemSet(m_data + m_size, byte, count);
        m_size += count;
    }

    void BufferWriter::WriteRepeatAt(uint32_t offset, uint8_t byte, uint32_t count)
    {
        HE_ASSERT(count < m_size && offset < (m_size - count));
        MemSet(m_data + offset, byte, count);
    }

    void BufferWriter::GrowBy(uint32_t len)
    {
        HE_ASSERT(len < MaxBytes && m_capacity <= (MaxBytes - len));

        if ((m_size + len) <= m_capacity)
            return;

        Reserve(CalculateGrowth(len));
    }

    uint32_t BufferWriter::CalculateGrowth(uint32_t len) const
    {
        uint32_t growAmount = m_capacity;

        switch (m_strategy)
        {
            case GrowthStrategy::Factor:
                growAmount = static_cast<uint32_t>(m_capacity * m_growth);
                break;
            case GrowthStrategy::Fixed:
                growAmount = static_cast<uint32_t>(m_growth);
                break;
        }

        // If our growth would overflow just assume max elements
        if (m_capacity > (MaxBytes - growAmount))
            return MaxBytes;

        const uint32_t newCapacity = m_capacity + growAmount;

        // If normal growth wouldn't be enough, just use the new size
        if (newCapacity < (m_size + len))
            return m_size + len;

        return newCapacity;
    }

    void BufferWriter::CopyFrom(const BufferWriter& x)
    {
        Reserve(x.m_size);
        MemCopy(m_data, x.m_data, x.m_size);
        m_size = x.m_size;
        m_strategy = x.m_strategy;
        m_growth = x.m_growth;
    }

    void BufferWriter::MoveFrom(BufferWriter&& x)
    {
        // If there are different allocators we have to make our own allocation and move everything over.
        if (&m_allocator != &x.m_allocator)
        {
            CopyFrom(x);
            return;
        }

        if (m_data)
        {
            m_allocator.Free(m_data);
        }

        m_data = Exchange(x.m_data, nullptr);
        m_size = Exchange(x.m_size, 0);
        m_capacity = Exchange(x.m_capacity, 0);
        m_strategy = x.m_strategy;
        m_growth = x.m_growth;
    }

    const char* AsString(BufferWriter::GrowthStrategy x)
    {
        switch (x)
        {
            case BufferWriter::GrowthStrategy::Factor: return "Factor";
            case BufferWriter::GrowthStrategy::Fixed: return "Fixed";
        }

        return "<unknown>";
    }
}
