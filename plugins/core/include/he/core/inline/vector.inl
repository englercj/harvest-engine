// Copyright Chad Engler

// Forward declare placement new. Doing this instead of including <new> has a major impact
// on reducing compile times, espcially since this file is included everywhere.
[[nodiscard]] inline void* __cdecl operator new(size_t, void* ptr) noexcept;

namespace he
{
    // Performs value initialization of trivially constructible elements.
    template <typename T> requires(IsTriviallyConstructible<T>)
    void _VectorConstruct(T* p, uint32_t n)
    {
        MemZero(p, n * sizeof(T));
    }

    // Performs value initialization of trivially constructible elements.
    template <typename T, typename... Args> requires(IsTriviallyConstructible<T>)
    void _VectorConstruct(T* p, uint32_t n, Args&&... args)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            ::new(p + i) T(Forward<Args>(args)...);
        }
    }

    // Performs construction non-trivially constructible elements.
    template <typename T, typename... Args> requires(!IsTriviallyConstructible<T>)
    void _VectorConstruct(T* p, uint32_t n, Args&&... args)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            ::new(p + i) T(Forward<Args>(args)...);
        }
    }

    // Performs default initialization of trivially constructible elements.
    template <typename T> requires(IsTriviallyConstructible<T>)
    void _VectorConstruct_DefaultInit(T*, uint32_t)
    { }

    // Performs construction non-trivially constructible elements.
    template <typename T> requires(!IsTriviallyConstructible<T>)
    void _VectorConstruct_DefaultInit(T* p, uint32_t n)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            ::new(p + i) T;
        }
    }

    // Performs destruction of trivially destructable elements.
    template <typename T> requires(IsTriviallyDestructible<T>)
    void _VectorDestruct(T*, uint32_t)
    { }

    // Performs destruction of non-trivially destructable elements.
    template <typename T> requires(!IsTriviallyDestructible<T>)
    void _VectorDestruct(T* p, uint32_t n)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            (p + i)->~T();
        }
    }

    // Performs a copy of trivially copyable elements.
    template <typename T> requires(IsTriviallyCopyable<T>)
    void _VectorCopyConstruct(T* dst, const T* src, uint32_t n)
    {
        MemCopy(dst, src, n * sizeof(T));
    }

    // Performs a copy of non-trivially copyable elements.
    template <typename T> requires(!IsTriviallyCopyable<T>)
    void _VectorCopyConstruct(T* dst, const T* src, uint32_t n)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            ::new(dst + i) T(src[i]);
        }
    }

    // Performs a copy of trivially copyable elements.
    template <typename T> requires(IsTriviallyCopyable<T>)
    void _VectorCopyAssign(T* dst, const T* src, uint32_t n)
    {
        MemCopy(dst, src, n * sizeof(T));
    }

    // Performs a copy of non-trivially copyable elements.
    template <typename T> requires(!IsTriviallyCopyable<T>)
    void _VectorCopyAssign(T* dst, const T* src, uint32_t n)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            dst[i] = src[i];
        }
    }

    // Performs a move of trivially copyable elements.
    template <typename T> requires(IsTriviallyCopyable<T>)
    void _VectorMoveConstruct(T* dst, T* src, uint32_t n)
    {
        MemMove(dst, src, n * sizeof(T));
    }

    // Performs a move of non-trivially copyable elements.
    template <typename T> requires(!IsTriviallyCopyable<T> && IsMoveConstructible<T>)
    void _VectorMoveConstruct(T* dst, T* src, uint32_t n)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            ::new(dst + i) T(Move(src[i]));
        }
    }

    // Performs a move of non-trivially copyable elements.
    template <typename T> requires(!IsTriviallyCopyable<T> && !IsMoveConstructible<T>)
    void _VectorMoveConstruct(T* dst, T* src, uint32_t n)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            ::new(dst + i) T(src[i]);
        }
    }

    // Performs a move of trivially copyable elements.
    template <typename T> requires(IsTriviallyCopyable<T>)
    void _VectorMoveAssign(T* dst, T* src, uint32_t n)
    {
        MemMove(dst, src, n * sizeof(T));
    }

    // Performs a move of non-trivially copyable elements.
    template <typename T> requires(!IsTriviallyCopyable<T> && IsMoveAssignable<T>)
    void _VectorMoveAssign(T* dst, T* src, uint32_t n)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            dst[i] = Move(src[i]);
        }
    }

    // Performs a copy of non-trivially copyable elements.
    template <typename T> requires(!IsTriviallyCopyable<T> && !IsMoveAssignable<T>)
    void _VectorMoveAssign(T* dst, T* src, uint32_t n)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            dst[i] = src[i];
        }
    }

    // Performs a move of trivially copyable elements.
    template <typename T> requires(IsTriviallyCopyable<T>)
    void _VectorReverseMoveAssign(T* dst, T* src, uint32_t n)
    {
        MemMove(dst, src, n * sizeof(T));
    }

    // Performs a move of non-trivially copyable elements. Starting from the last and iterating to the first.
    template <typename T> requires(!IsTriviallyCopyable<T> && IsMoveAssignable<T>)
    void _VectorReverseMoveAssign(T* dst, T* src, uint32_t n)
    {
        T* srcEnd = src + n;
        T* dstEnd = dst + n;

        while (dst != dstEnd)
        {
            *(--dstEnd) = Move(*(--srcEnd));
        }
    }

    // Performs a copy of non-trivially copyable elements. Starting from the last and iterating to the first.
    template <typename T> requires(!IsTriviallyCopyable<T> && !IsMoveAssignable<T>)
    void _VectorReverseMoveAssign(T* dst, T* src, uint32_t n)
    {
        const T* srcEnd = src + n;
        T* dstEnd = dst + n;

        while (dst != dstEnd)
        {
            *(--dstEnd) = *(--srcEnd);
        }
    }

    // Reallocates vector memory for trivially copyable types by letting the allocator handle it.
    // This can result in better performing resizes since we may not copy at all.
    template <typename T> requires(IsTriviallyCopyable<T>)
    T* _VectorRealloc(Allocator& allocator, T* p, uint32_t size, uint32_t newSize)
    {
        HE_UNUSED(size);
        return static_cast<T*>(allocator.Realloc(p, newSize * sizeof(T), alignof(T)));
    }

    // Performs an allocate and move operation for non-trivially copyable elements.
    template <typename T> requires(!IsTriviallyCopyable<T> && IsMoveConstructible<T>)
    T* _VectorRealloc(Allocator& allocator, T* p, uint32_t size, uint32_t newSize)
    {
        T* mem = allocator.Malloc<T>(newSize);
        for (uint32_t i = 0; i < size; ++i)
        {
            ::new(mem + i) T(Move(p[i]));
            p[i].~T();
        }
        allocator.Free(p);
        return mem;
    }

    // Performs an allocate and copy operation for non-trivially copyable elements.
    template <typename T> requires(!IsTriviallyCopyable<T> && !IsMoveConstructible<T>)
    T* _VectorRealloc(Allocator& allocator, T* p, uint32_t size, uint32_t newSize)
    {
        T* mem = allocator.Malloc<T>(newSize);
        for (uint32_t i = 0; i < size; ++i)
        {
            ::new(mem + i) T(p[i]);
            p[i].~T();
        }
        allocator.Free(p);
        return mem;
    }

    template <typename T, typename U> requires(IsTriviallyCopyable<T> && IsTriviallyCopyable<U>)
    bool _VectorEqual(const T* a, const U* b, uint32_t len)
    {
        return MemCmp(a, b, len) == 0;
    }

    template <typename T, typename U> requires(!IsTriviallyCopyable<T> || !IsTriviallyCopyable<U>)
    bool _VectorEqual(const T* a, const U* b, uint32_t len)
    {
        for (uint32_t i = 0; i < len; ++i)
        {
            if (*a != *b)
                return false;
        }

        return true;
    }

    template <typename T>
    Vector<T>::Vector(Allocator& allocator) noexcept
        : m_allocator(allocator)
    {}

    template <typename T>
    Vector<T>::Vector(const Vector& x, Allocator& allocator) noexcept
        : Vector(allocator)
    {
        CopyFrom(x);
    }

    template <typename T>
    Vector<T>::Vector(Vector&& x, Allocator& allocator) noexcept
        : Vector(allocator)
    {
        MoveFrom(Move(x));
    }

    template <typename T>
    Vector<T>::Vector(const Vector& x) noexcept
        : Vector(x.m_allocator)
    {
        CopyFrom(x);
    }

    template <typename T>
    Vector<T>::Vector(Vector&& x) noexcept
        : Vector(x.m_allocator)
    {
        MoveFrom(Move(x));
    }

    template <typename T>
    Vector<T>::~Vector() noexcept
    {
        Destroy();
    }

    template <typename T>
    Vector<T>& Vector<T>::operator=(const Vector& x) noexcept
    {
        CopyFrom(x);
        return *this;
    }

    template <typename T>
    Vector<T>& Vector<T>::operator=(Vector&& x) noexcept
    {
        MoveFrom(Move(x));
        return *this;
    }

    template <typename T>
    const T& Vector<T>::operator[](uint32_t index) const
    {
        HE_ASSERT(index < m_size);
        return m_data[index];
    }

    template <typename T>
    template <typename U> requires(IsConvertible<U(*)[], T(*)[]>)
    bool Vector<T>::operator==(const Vector<U>& x) const
    {
        if (this == &x)
            return true;

        if (m_size != x.m_size)
            return false;

        return _VectorEqual(m_data, x.m_data, m_size);
    }

    template <typename T>
    uint32_t Vector<T>::Capacity() const
    {
        return m_capacity;
    }

    template <typename T>
    uint32_t Vector<T>::Size() const
    {
        return m_size;
    }

    template <typename T>
    void Vector<T>::Reserve(uint32_t len)
    {
        if (len <= m_capacity)
            return;

        len = Max(MinElements, len);

        m_data = _VectorRealloc(m_allocator, m_data, m_size, len);
        m_capacity = len;
    }

    template <typename T>
    void Vector<T>::Resize(uint32_t len, DefaultInitTag)
    {
        Reserve(len);

        if (len > m_size)
        {
            _VectorConstruct_DefaultInit(m_data + m_size, len - m_size);
        }
        else if (len < m_size)
        {
            _VectorDestruct(m_data + len, (m_size - len));
        }

        m_size = len;
    }

    template <typename T>
    template <typename... Args>
    void Vector<T>::Resize(uint32_t len, Args&&... args)
    {
        Reserve(len);

        if (len > m_size)
        {
            _VectorConstruct(m_data + m_size, len - m_size, Forward<Args>(args)...);
        }
        else if (len < m_size)
        {
            _VectorDestruct(m_data + len, (m_size - len));
        }

        m_size = len;
    }

    template <typename T>
    void Vector<T>::Expand(uint32_t len, DefaultInitTag)
    {
        GrowBy(len);
        _VectorConstruct_DefaultInit(m_data + m_size, len);
        m_size += len;
    }

    template <typename T>
    template <typename... Args>
    void Vector<T>::Expand(uint32_t len, Args&&... args)
    {
        GrowBy(len);
        _VectorConstruct(m_data + m_size, len, Forward<Args>(args)...);
        m_size += len;
    }

    template <typename T>
    void Vector<T>::ShrinkToFit()
    {
        m_data = _VectorRealloc(m_allocator, m_data, m_size, m_size);
        m_capacity = m_size;
    }

    template <typename T>
    const T* Vector<T>::Data() const
    {
        return m_data;
    }

    template <typename T>
    void Vector<T>::Adopt(T* data, uint32_t size, uint32_t capacity)
    {
        Destroy();
        m_data = data;
        m_size = size;
        m_capacity = capacity;
    }

    template <typename T>
    T* Vector<T>::Release()
    {
        m_size = 0;
        m_capacity = 0;
        return Exchange(m_data, nullptr);
    }

    template <typename T>
    const T* Vector<T>::Begin() const
    {
        return m_data;
    }

    template <typename T>
    const T* Vector<T>::End() const
    {
        return m_data + m_size;
    }

    template <typename T>
    void Vector<T>::Clear()
    {
        _VectorDestruct(m_data, m_size);
        m_size = 0;
    }

    template <typename T>
    void Vector<T>::Insert(uint32_t index, const T& element)
    {
        Insert(index, &element, &element + 1);
    }

    template <typename T>
    void Vector<T>::Insert(uint32_t index, T&& element)
    {
        HE_ASSERT(index <= m_size);

        GrowBy(1);

        if (index == m_size)
        {
            _VectorMoveConstruct(m_data + m_size, &element, 1);
        }
        else
        {
            _VectorMoveConstruct(m_data + m_size, m_data + m_size - 1, 1);
            if (m_size > 0)
            {
                _VectorReverseMoveAssign(m_data + index + 1, m_data + index, m_size - index - 1);
                _VectorMoveAssign(m_data + index, &element, 1);
            }
        }

        ++m_size;
    }

    template <typename T>
    void Vector<T>::Insert(uint32_t index, const T* begin, const T* end)
    {
        HE_ASSERT(index <= m_size);
        HE_ASSERT(begin && end);

        const uint32_t len = static_cast<uint32_t>(end - begin);

        GrowBy(len);

        if (index == m_size)
        {
            _VectorCopyConstruct(m_data + m_size, begin, len);
        }
        else
        {
            const uint32_t shiftCount = m_size - index;
            const uint32_t shiftConstruct = Min(shiftCount, len);
            const uint32_t shiftAssign = shiftCount - shiftConstruct;

            const uint32_t newSize = m_size + len;

            _VectorMoveConstruct(
                m_data + newSize - shiftConstruct,
                m_data + m_size - shiftConstruct,
                shiftConstruct);

            if (shiftAssign > 0)
            {
                _VectorReverseMoveAssign(
                    m_data + newSize - shiftConstruct - shiftAssign,
                    m_data + m_size - shiftConstruct - shiftAssign,
                    shiftAssign);
            }

            const uint32_t assignCount = m_size - index;
            const uint32_t constructCount = len - assignCount;

            _VectorCopyAssign(m_data + index, begin, assignCount);

            if (constructCount > 0)
            {
                _VectorCopyConstruct(m_data + index + assignCount, begin + assignCount, constructCount);
            }
        }

        m_size += len;
    }

    template <typename T>
    void Vector<T>::Erase(uint32_t index, uint32_t count)
    {
        if (count == 0)
            return;

        HE_ASSERT((index + count) <= m_size);

        const uint32_t moveCount = m_size - count - index;
        if (moveCount > 0)
        {
            _VectorMoveAssign(m_data + index, m_data + index + count, moveCount);
        }

        m_size -= count;
        _VectorDestruct(m_data + m_size, count);
    }

    template <typename T>
    void Vector<T>::EraseUnordered(uint32_t index, uint32_t count)
    {
        if (count == 0)
            return;

        HE_ASSERT((index + count) <= m_size);

        // Move over as many tail elements as we can into the erased elements
        const uint32_t tailCount = m_size - count - index;
        const uint32_t moveCount = Min(tailCount, count);
        if (moveCount > 0)
        {
            _VectorMoveAssign(m_data + index, m_data + m_size - moveCount, moveCount);
        }

        m_size -= count;
        _VectorDestruct(m_data + m_size, count);
    }

    template <typename T>
    T& Vector<T>::PushBack(const T& element)
    {
        GrowBy(1);
        _VectorCopyConstruct(m_data + m_size, &element, 1);
        return m_data[m_size++];
    }

    template <typename T>
    T& Vector<T>::PushBack(T&& element)
    {
        GrowBy(1);
        _VectorMoveConstruct(m_data + m_size, &element, 1);
        return m_data[m_size++];
    }

    template <typename T>
    T& Vector<T>::PushFront(const T& element)
    {
        GrowBy(1);
        if (m_size > 0)
        {
            _VectorMoveConstruct(m_data + m_size, m_data + m_size - 1, 1);
        }

        if (m_size > 1)
        {
            _VectorReverseMoveAssign(m_data + 1, m_data, m_size - 1);
        }

        _VectorCopyAssign(m_data, &element, 1);
        return m_data[m_size++];
    }

    template <typename T>
    T& Vector<T>::PushFront(T&& element)
    {
        GrowBy(1);
        if (m_size > 0)
        {
            _VectorMoveConstruct(m_data + m_size, m_data + m_size - 1, 1);
        }

        if (m_size > 1)
        {
            _VectorReverseMoveAssign(m_data + 1, m_data, m_size - 1);
        }

        _VectorMoveAssign(m_data, &element, 1);
        return m_data[m_size++];
    }

    template <typename T>
    void Vector<T>::PopBack()
    {
        HE_ASSERT(m_size > 0);
        --m_size;
        _VectorDestruct(m_data + m_size, 1);
    }

    template <typename T>
    void Vector<T>::PopFront()
    {
        HE_ASSERT(m_size > 0);
        --m_size;
        _VectorMoveAssign(m_data, m_data + 1, m_size);
        _VectorDestruct(m_data + m_size, 1);
    }

    template <typename T>
    T& Vector<T>::EmplaceBack(DefaultInitTag)
    {
        GrowBy(1);
        _VectorConstruct_DefaultInit(m_data + m_size, 1);
        return m_data[m_size++];
    }

    template <typename T>
    T& Vector<T>::EmplaceBack()
    {
        GrowBy(1);
        _VectorConstruct(m_data + m_size, 1);
        return m_data[m_size++];
    }

    template <typename T>
    template <typename... Args>
    T& Vector<T>::EmplaceBack(Args&&... args)
    {
        GrowBy(1);
        _VectorConstruct(m_data + m_size, 1, Forward<Args>(args)...);
        return m_data[m_size++];
    }

    template <typename T>
    void Vector<T>::GrowBy(uint32_t len)
    {
        HE_ASSERT(len < MaxElements && m_capacity <= (MaxElements - len));

        if ((m_size + len) <= m_capacity)
            return;

        Reserve(CalculateGrowth(len));
    }

    template <typename T>
    uint32_t Vector<T>::CalculateGrowth(uint32_t len) const
    {
        // If our growth would overflow just assume max elements
        if (m_capacity > (MaxElements - (m_capacity / 2)))
            return MaxElements;

        const uint32_t newCapacity = m_capacity + (m_capacity / 2);

        // If normal growth wouldn't be enough, just use the new size
        if (newCapacity < (m_size + len))
            return m_size + len;

        return newCapacity;
    }

    template <typename T>
    void Vector<T>::CopyFrom(const Vector& x)
    {
        Clear();
        Reserve(x.m_size);
        _VectorCopyConstruct(m_data, x.m_data, x.m_size);
        m_size = x.m_size;
    }

    template <typename T>
    void Vector<T>::MoveFrom(Vector&& x)
    {
        if (this == &x)
            return;

        Clear();

        // If there are different allocators we have to make our own allocation and move everything over.
        if (&m_allocator != &x.m_allocator)
        {
            Reserve(x.m_size);
            _VectorMoveConstruct(m_data, x.m_data, x.m_size);
            m_size = x.m_size;
            x.Clear();
            return;
        }

        if (m_data)
        {
            m_allocator.Free(m_data);
        }

        m_data = Exchange(x.m_data, nullptr);
        m_size = Exchange(x.m_size, 0);
        m_capacity = Exchange(x.m_capacity, 0);
    }

    template <typename T>
    void Vector<T>::Destroy()
    {
        if (m_data)
        {
            Clear();
            m_allocator.Free(m_data);
        }
    }
}
