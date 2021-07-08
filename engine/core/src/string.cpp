// Copyright Chad Engler

#include "he/core/string.h"

#include "he/core/assert.h"
#include "he/core/memory_ops.h"

#include <cstring>

namespace he
{
    uint32_t String::Length(const char* s)
    {
        return static_cast<uint32_t>(strlen(s));
    }

    uint32_t String::LengthN(const char* s, uint32_t len)
    {
        return static_cast<uint32_t>(strnlen(s, len));
    }

    int32_t String::Compare(const char* a, const char* b)
    {
        return strcmp(a, b);
    }

    int32_t String::CompareN(const char* a, const char* b, uint32_t len)
    {
        return strncmp(a, b, len);
    }

    int32_t String::CompareI(const char* a, const char* b)
    {
    #if HE_COMPILER_MSVC
        return _stricmp(a, b);
    #else
        return strcasecmp(a, b);
    #endif
    }

    int32_t String::CompareNI(const char* a, const char* b, uint32_t len)
    {
    #if HE_COMPILER_MSVC
        return _strnicmp(a, b, len);
    #else
        return strncasecmp(a, b, len);
    #endif
    }

    uint32_t String::Copy(char* dst, uint32_t dstLen, const char* src)
    {
        uint32_t srcLen = Length(src);
        if (dstLen > 0)
        {
            --dstLen;
            uint32_t len = srcLen < dstLen ? srcLen : dstLen;
            MemCopy(dst, src, len);
            dst[len] = '\0';
        }
        return srcLen;
    }

    uint32_t String::CopyN(char* dst, uint32_t dstLen, const char* src, uint32_t srcLen)
    {
        srcLen = LengthN(src, srcLen);
        if (dstLen > 0)
        {
            --dstLen;
            uint32_t len = srcLen < dstLen ? srcLen : dstLen;
            MemCopy(dst, src, len);
            dst[len] = '\0';
        }
        return srcLen;
    }

    uint32_t String::Cat(char* dst, uint32_t dstLen, const char* src)
    {
        uint32_t n = Length(dst);
        dstLen = dstLen > n ? dstLen - n : 0;
        return n + Copy(dst + n, dstLen, src);
    }

    uint32_t String::CatN(char* dst, uint32_t dstLen, const char* src, uint32_t srcLen)
    {
        uint32_t n = Length(dst);
        dstLen = dstLen > n ? dstLen - n : 0;
        return n + CopyN(dst + n, dstLen, src, srcLen);
    }

    const char* Find(const char* str, char search)
    {
        return strchr(str, search);
    }

    char* Find(char* str, char search)
    {
        return strchr(str, search);
    }

    const char* Find(const char* str, const char* search)
    {
        return strstr(str, search);
    }

    char* Find(char* str, const char* search)
    {
        return strstr(str, search);
    }

    String::String(Allocator& allocator)
        : m_allocator(allocator)
    {
        SetSizeEmbed(0);
    }

    String::String(Allocator& allocator, const char* str)
        : String(allocator)
    {
        Append(str);
    }

    String::String(Allocator& allocator, const char* str, uint32_t len)
        : String(allocator)
    {
        Append(str, len);
    }

    String::String(Allocator& allocator, const String& x)
        : String(allocator)
    {
        CopyFrom(x);
    }

    String::String(Allocator& allocator, String&& x)
        : String(allocator)
    {
        MoveFrom(Move(x));
    }

    String::String(const String& x)
        : String(x.m_allocator)
    {
        CopyFrom(x);
    }

    String::String(String&& x)
        : String(x.m_allocator)
    {
        MoveFrom(Move(x));
    }

    String::~String()
    {
        if (!IsEmbedded())
        {
            m_allocator.Free(m_heap.data);
        }
    }

    String& String::operator=(const String& x)
    {
        CopyFrom(x);
        return *this;
    }

    String& String::operator=(String&& x)
    {
        MoveFrom(Move(x));
        return *this;
    }

    char& String::operator[](uint32_t index)
    {
        HE_ASSERT(index < Size());
        return Data()[index];
    }

    String& String::operator+=(const String& str)
    {
        Insert(Size(), str.Data(), str.Size());
        return *this;
    }

    String& String::operator+=(const char* str)
    {
        Insert(Size(), str, Length(str));
        return *this;
    }

    bool String::operator==(const String& x)
    {
        return CompareTo(x) == 0;
    }

    bool String::operator!=(const String& x)
    {
        return CompareTo(x) != 0;
    }

    bool String::operator<(const String& x)
    {
        return CompareTo(x) < 0;
    }

    bool String::operator<=(const String& x)
    {
        return CompareTo(x) <= 0;
    }

    bool String::operator>(const String& x)
    {
        return CompareTo(x) > 0;
    }

    bool String::operator>=(const String& x)
    {
        return CompareTo(x) >= 0;
    }

    bool String::IsEmbedded() const
    {
        return m_embed[EmbedSize - 1] == HeapFlag;
    }

    uint32_t String::Capacity() const
    {
        return (IsEmbedded() ? EmbedSize : m_heap.capacity) - 1;
    }

    uint32_t String::Size() const
    {
        if (IsEmbedded())
        {
            const uint32_t avail = static_cast<uint32_t>(m_embed[EmbedSize - 1]);
            return (EmbedSize - 1) - avail;
        }

        return m_heap.size;
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

    char* String::Data()
    {
        if (IsEmbedded())
            return m_embed;

        return m_heap.data;
    }

    int32_t String::CompareTo(const String& x)
    {
        const uint32_t size = Min(Size(), x.Size());
        const int32_t cmp = MemCmp(Data(), x.Data(), size);

        if (cmp == 0)
            return Size() - x.Size();

        return cmp;
    }

    bool String::EqualTo(const String& x)
    {
        return CompareTo(x) == 0;
    }

    bool String::LessThan(const String& x)
    {
        return CompareTo(x) < 0;
    }

    char* String::Begin()
    {
        return Data();
    }

    char* String::End()
    {
        return Data() + Size();
    }

    void String::Clear()
    {
        SetSize(0);
    }

    void String::Insert(uint32_t index, char c)
    {
        Insert(index, &c, 1);
    }

    void String::Insert(uint32_t index, const char* str)
    {
        Insert(index, str, Length(str));
    }

    void String::Insert(uint32_t index, const char* str, uint32_t len)
    {
        HE_ASSERT(str);

        if (len == 0)
            return;

        const uint32_t size = Size();

        HE_ASSERT(index <= size);

        Reserve(size + len);

        char* data = Data();

        if (size > index)
        {
            MemMove(data + index + len, data + index, (size - index));
        }

        MemCopy(data + index, str, len);
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

    void String::PushBack(char c)
    {
        Insert(Size(), &c, 1);
    }

    void String::PopBack()
    {
        const uint32_t size = Size();

        if (size > 0)
        {
            SetSize(size - 1);
        }
    }

    void String::Append(const String& str)
    {
        Insert(Size(), str.Data(), str.Size());
    }

    void String::Append(const char* str)
    {
        Insert(Size(), str, Length(str));
    }

    void String::Append(const char* str, uint32_t len)
    {
        Insert(Size(), str, len);
    }

    void String::GrowBy(uint32_t n)
    {
        if (n <= Capacity())
            return;

        Reserve(CalculateGrowth(n));
    }

    uint32_t String::CalculateGrowth(uint32_t n)
    {
        const uint32_t size = Size();
        const uint32_t capacity = Capacity();

        // If our growth would overflow just assume max elements
        if (capacity >= (MaxCharacters - (capacity / 2)))
            return MaxCharacters;

        const uint32_t newCapacity = capacity + (capacity / 2);

        // If normal growth wouldn't be enough, just use the new size
        if (newCapacity < (size + n))
            return size + n;

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
        const uint32_t xSize = x.Size();

        Reserve(xSize);
        MemCopy(Data(), x.Data(), xSize);
        SetSize(xSize);
    }

    void String::MoveFrom(String&& x)
    {
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
    }
}
