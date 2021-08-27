// Copyright Chad Engler

#include "he/core/string.h"

#include "he/core/assert.h"
#include "he/core/memory_ops.h"
#include "he/core/utils.h"

#include <cstdlib>
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
        if (dstLen == 0)
            return 0;

        --dstLen;

        const uint32_t srcLen = Length(src);
        const uint32_t len = srcLen < dstLen ? srcLen : dstLen;

        MemCopy(dst, src, len);
        dst[len] = '\0';

        return len;
    }

    uint32_t String::CopyN(char* dst, uint32_t dstLen, const char* src, uint32_t srcLen)
    {
        if (dstLen == 0)
            return 0;

        --dstLen;

        srcLen = LengthN(src, srcLen);
        const uint32_t len = srcLen < dstLen ? srcLen : dstLen;

        MemCopy(dst, src, len);
        dst[len] = '\0';

        return len;
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

    const char* String::Find(const char* str, char search)
    {
        return strchr(str, search);
    }

    char* String::Find(char* str, char search)
    {
        return strchr(str, search);
    }

    const char* String::Find(const char* str, const char* search)
    {
        return strstr(str, search);
    }

    char* String::Find(char* str, const char* search)
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

    int32_t String::CompareTo(const String& x) const
    {
        const uint32_t s0 = Size();
        const uint32_t s1 = x.Size();
        const int32_t result = MemCmp(Data(), x.Data(), Min(s0, s1));

        if (result != 0)
            return result;

        if (s0 < s1)
            return -1;

        if (s0 > s1)
            return 1;

        return 0;
    }

    void String::Insert(uint32_t index, const char* str, uint32_t len)
    {
        HE_ASSERT(str);

        if (len == 0)
            return;

        uint32_t size = Size();

        HE_ASSERT(index <= size);

        GrowBy(len);

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

    char String::PopBack()
    {
        const uint32_t size = Size();

        HE_ASSERT(size > 0);

        const char back = Data()[size - 1];
        SetSize(size - 1);

        return back;
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

        x.SetSizeEmbed(0);
    }

#define HE_FROMSTR_INT_IMPL(T, fn) \
    template <> T String::ToInteger<T>(const char* str, const char** end, int32_t base) { return static_cast<T>(std::fn(str, const_cast<char**>(end), base)); }

    HE_FROMSTR_INT_IMPL(signed char, strtol)
    HE_FROMSTR_INT_IMPL(char, strtol)
    HE_FROMSTR_INT_IMPL(short, strtol)
    HE_FROMSTR_INT_IMPL(int, strtol)
    HE_FROMSTR_INT_IMPL(long, strtol)
    HE_FROMSTR_INT_IMPL(long long, strtoll)

    HE_FROMSTR_INT_IMPL(unsigned char, strtoul)
    HE_FROMSTR_INT_IMPL(unsigned short, strtoul)
    HE_FROMSTR_INT_IMPL(unsigned int, strtoul)
    HE_FROMSTR_INT_IMPL(unsigned long, strtoul)
    HE_FROMSTR_INT_IMPL(unsigned long long, strtoull)

#undef HE_FROMSTR_INT_IMPL

#define HE_FROMSTR_FLT_IMPL(T, fn) \
    template <> T String::ToFloat<T>(const char* str, const char** end) { return std::fn(str, const_cast<char**>(end)); }

    HE_FROMSTR_FLT_IMPL(float, strtof)
    HE_FROMSTR_FLT_IMPL(double, strtod)

#undef HE_FROMSTR_FLT_IMPL
}
