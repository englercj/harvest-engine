// Copyright Chad Engler

#include "he/core/string.h"

#include "he/core/ascii.h"
#include "he/core/assert.h"
#include "he/core/hash.h"
#include "he/core/memory_ops.h"
#include "he/core/string_ops.h"
#include "he/core/utils.h"

namespace he
{
    String::String(Allocator& allocator) noexcept
        : m_allocator(allocator)
    {
        SetSizeEmbed(0);
    }

    String::String(const char* str, Allocator& allocator) noexcept
        : String(allocator)
    {
        Append(str);
    }

    String::String(const char* str, uint32_t len, Allocator& allocator) noexcept
        : String(allocator)
    {
        Append(str, len);
    }

    String::String(const String& x, Allocator& allocator) noexcept
        : String(allocator)
    {
        CopyFrom(x);
    }

    String::String(String&& x, Allocator& allocator) noexcept
        : String(allocator)
    {
        MoveFrom(Move(x));
    }

    String::String(const String& x) noexcept
        : String(x.m_allocator)
    {
        CopyFrom(x);
    }

    String::String(String&& x) noexcept
        : String(x.m_allocator)
    {
        MoveFrom(Move(x));
    }

    String::~String() noexcept
    {
        if (!IsEmbedded())
        {
            m_allocator.Free(m_heap.data);
        }
    }

    String& String::operator=(const String& x) noexcept
    {
        CopyFrom(x);
        return *this;
    }

    String& String::operator=(String&& x) noexcept
    {
        MoveFrom(Move(x));
        return *this;
    }

    const char& String::operator[](uint32_t index) const
    {
        HE_ASSERT(index < Size());
        return Data()[index];
    }

    void String::Reserve(uint32_t len)
    {
        // Handle our embedded data moving to the heap
        if (IsEmbedded())
        {
            if (len < EmbedSize)
                return;

            void* mem = m_allocator.Malloc(len + 1);

            const uint32_t size = Size();
            HE_ASSERT(size < len);
            MemCopy(mem, m_embed, size + 1);

            m_heap.data = static_cast<char*>(mem);
            m_heap.capacity = len + 1;
            SetSizeHeap(size);
            return;
        }

        if (len < m_heap.capacity)
            return;

        // Reallocate our heap data
        m_heap.data = static_cast<char*>(m_allocator.Realloc(m_heap.data, len + 1));
        m_heap.capacity = len + 1;
    }

    void String::Resize(uint32_t len, DefaultInitTag)
    {
        Reserve(len);
        SetSize(len);
    }

    void String::Resize(uint32_t len, char c)
    {
        Reserve(len);

        const uint32_t size = Size();

        if (len > size)
        {
            MemSet(Data() + size, c, (len - size));
        }

        SetSize(len);
    }

    void String::Expand(uint32_t len, DefaultInitTag)
    {
        GrowBy(len);
        SetSize(len);
    }

    void String::Expand(uint32_t len, char c)
    {
        GrowBy(len);

        const uint32_t size = Size();

        if (len > size)
        {
            MemSet(Data() + size, c, (len - size));
        }

        SetSize(len);
    }

    void String::ShrinkToFit()
    {
        if (IsEmbedded())
            return;

        const uint32_t size = m_heap.size;

        // If we can fit in the embedded data block, move the string there
        if (size < EmbedSize)
        {
            void* mem = m_heap.data;
            MemCopy(m_embed, mem, size);
            SetSizeEmbed(size);
            m_allocator.Free(mem);
            return;
        }

        // Reallocate our heap data to the smaller size
        m_heap.data = static_cast<char*>(m_allocator.Realloc(m_heap.data, size + 1));
        m_heap.capacity = size + 1;
        SetSizeHeap(size);
    }

    const char& String::Front() const
    {
        HE_ASSERT(!IsEmpty());
        return Data()[0];
    }

    const char& String::Back() const
    {
        HE_ASSERT(!IsEmpty());
        return Data()[Size() - 1];
    }

    uint64_t String::HashCode() const noexcept
    {
        return WyHash::Mem(Data(), Size());
    }

    int32_t String::CompareTo(const char* str, uint32_t len) const
    {
        const uint32_t s0 = Size();
        const uint32_t s1 = len;
        const int32_t result = MemCmp(Data(), str, Min(s0, s1));

        if (result != 0)
            return result;

        if (s0 < s1)
            return -1;

        if (s0 > s1)
            return 1;

        return 0;
    }

    int32_t String::CompareToI(const char* str, uint32_t len) const
    {
        const uint32_t s0 = Size();
        const uint32_t s1 = len;

        const char* a = Data();
        const char* b = str;

        len = Min(s0, s1);

        int32_t result = 0;
        for (; len > 0; --len, ++a, ++b)
        {
            const char al = ToLower(*a);
            const char bl = ToLower(*b);
            if (al != bl)
            {
                result = al < bl ? -1 : 1;
                break;
            }
        }

        if (result != 0)
            return result;

        if (s0 < s1)
            return -1;

        if (s0 > s1)
            return 1;

        return 0;
    }

    void String::Insert(uint32_t index, const char* begin, const char* end)
    {
        if (end > begin)
        {
            Insert(index, begin, static_cast<uint32_t>(end - begin));
        }
    }

    void String::Insert(uint32_t index, const char* str, uint32_t len)
    {
        if (len == 0)
            return;

        uint32_t size = Size();

        HE_ASSERT(str);
        HE_ASSERT(index <= size);

        GrowBy(len);

        char* data = Data();

        if (size > index)
        {
            MemMove(data + index + len, data + index, (size - index));
        }

        // `str` could technically be within our buffer, so need to use MemMove
        MemMove(data + index, str, len);
        SetSize(size + len);
    }

    void String::Erase(uint32_t index, uint32_t count)
    {
        if (count == 0)
            return;

        const uint32_t size = Size();

        HE_ASSERT(index + count <= size);

        if (index + count < size)
        {
            char* data = Data();
            MemMove(data + index, data + index + count, (size - count));
        }

        SetSize(size - count);
    }

    char String::PopBack()
    {
        const uint32_t size = Size();

        HE_ASSERT(size > 0);

        const char back = Data()[size - 1];
        SetSize(size - 1);

        return back;
    }

    char String::PopFront()
    {
        const uint32_t size = Size();

        HE_ASSERT(size > 0);

        char* data = Data();
        const char front = data[0];

        MemMove(data, data + 1, size - 1);
        SetSize(size - 1);

        return front;
    }

    void String::GrowBy(uint32_t len)
    {
        const uint32_t size = Size();
        const uint32_t capacity = Capacity();

        HE_ASSERT(len < MaxHeapCharacters && capacity <= (MaxHeapCharacters - len));

        if ((size + len) <= capacity)
            return;

        Reserve(CalculateGrowth(len, size, capacity));
    }

    uint32_t String::CalculateGrowth(uint32_t len, uint32_t size, uint32_t capacity) const
    {
        // If our growth would overflow just assume max elements
        if (capacity >= (MaxHeapCharacters - (capacity / 2)))
            return MaxHeapCharacters;

        const uint32_t newCapacity = capacity + (capacity / 2);

        // If normal growth wouldn't be enough, just use the new size
        if (newCapacity < (size + len))
            return size + len;

        return newCapacity;
    }

    void String::SetSize(uint32_t size)
    {
        if (IsEmbedded())
            SetSizeEmbed(size);
        else
            SetSizeHeap(size);
    }

    void String::SetSizeEmbed(uint32_t size)
    {
        HE_ASSERT(size < EmbedSize);
        m_embed[size] = '\0';
        m_embed[EmbedSize - 1] = (EmbedSize - 1) - static_cast<char>(size);
    }

    void String::SetSizeHeap(uint32_t size)
    {
        HE_ASSERT(size < m_heap.capacity);
        m_heap.data[size] = '\0';
        m_heap.size = size;
        m_embed[EmbedSize - 1] = HeapFlag;
    }

    void String::CopyFrom(const String& x)
    {
        if (this == &x)
            return;

        const uint32_t xSize = x.Size();

        Reserve(xSize);
        MemCopy(Data(), x.Data(), xSize);
        SetSize(xSize);
    }

    void String::MoveFrom(String&& x)
    {
        if (this == &x)
            return;

        // If there are different allocators or the other object is embedded, we just have to copy.
        if (&m_allocator != &x.m_allocator || x.IsEmbedded())
        {
            CopyFrom(x);
            return;
        }

        // Need to free our heap allocation if there is one
        if (!IsEmbedded())
        {
            m_allocator.Free(m_heap.data);
        }

        // Steal the heap from the other string
        m_heap.data = Exchange(x.m_heap.data, nullptr);
        m_heap.size = Exchange(x.m_heap.size, 0);
        m_heap.capacity = Exchange(x.m_heap.capacity, 0);
        SetSizeHeap(m_heap.size);

        x.SetSizeEmbed(0);
    }
}
