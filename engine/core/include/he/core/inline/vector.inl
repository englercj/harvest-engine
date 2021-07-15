// Copyright Chad Engler

namespace he
{
    // Performs value initialization of trivially constructible elements.
    template <typename T, HE_REQUIRES(IsTriviallyConstructible<T>)>
    void _VectorConstruct(T* p, uint32_t n)
    {
        MemZero(p, n * sizeof(T));
    }

    // Performs value initialization of trivially constructible elements.
    template <typename T, typename... Args, HE_REQUIRES(IsTriviallyConstructible<T>)>
    void _VectorConstruct(T* p, uint32_t n, Args&&... args)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            new(p + i) T(Forward<Args>(args)...);
        }
    }

    // Performs construction non-trivially constructible elements.
    template <typename T, typename... Args, HE_REQUIRES(!IsTriviallyConstructible<T>)>
    void _VectorConstruct(T* p, uint32_t n, Args&&... args)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            new(p + i) T(Forward<Args>(args)...);
        }
    }

    // Performs default initialization of trivially constructible elements.
    template <typename T, HE_REQUIRES(IsTriviallyConstructible<T>)>
    void _VectorConstructDefault(T*, uint32_t) {}

    // Performs construction non-trivially constructible elements.
    template <typename T, HE_REQUIRES(!IsTriviallyConstructible<T>)>
    void _VectorConstructDefault(T* p, uint32_t n)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            new(p + i) T;
        }
    }

    // Performs destruction of trivially destructable elements.
    template <typename T, HE_REQUIRES(IsTriviallyDestructible<T>)>
    void _VectorDestruct(T*, uint32_t) {}

    // Performs destruction of non-trivially destructable elements.
    template <typename T, HE_REQUIRES(!IsTriviallyDestructible<T>)>
    void _VectorDestruct(T* p, uint32_t n)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            (p + i)->~T();
        }
    }

    // Performs a copy of trivially copyable elements.
    template <typename T, HE_REQUIRES(IsTriviallyCopyable<T>)>
    void _VectorCopy(T* dst, const T* src, uint32_t n)
    {
        MemCopy(dst, src, n * sizeof(T));
    }

    // Performs a copy of non-trivially copyable elements.
    template <typename T, HE_REQUIRES(!IsTriviallyCopyable<T>)>
    void _VectorCopy(T* dst, const T* src, uint32_t n)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            new(dst + i) T(*(src + i));
        }
    }

    // Performs a move of trivially copyable elements.
    template <typename T, HE_REQUIRES(IsTriviallyCopyable<T>)>
    void _VectorMove(T* dst, T* src, uint32_t n)
    {
        MemCopy(dst, src, n * sizeof(T));
    }

    // Performs a move of non-trivially copyable elements.
    template <typename T, HE_REQUIRES(!IsTriviallyCopyable<T> && IsMoveConstructible<T>)>
    void _VectorMove(T* dst, T* src, uint32_t n)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            new(dst + i) T(Move(*(src + i)));
        }
    }

    // Performs a copy of non-trivially copyable nor movable elements.
    template <typename T, HE_REQUIRES(!IsTriviallyCopyable<T> && !IsMoveConstructible<T>)>
    void _VectorMove(T* dst, T* src, uint32_t n)
    {
        for (uint32_t i = 0; i < n; ++i)
        {
            new(dst + i) T(*(src + i));
        }
    }

    // Reallocates vector memory for trivially copyable types by letting the allocator handle it.
    // This can result in better performing resizes since we may not copy at all.
    template <typename T, HE_REQUIRES(IsTriviallyCopyable<T>)>
    T* _VectorRealloc(Allocator& allocator, T* p, uint32_t size, uint32_t newSize)
    {
        HE_UNUSED(size);
        return static_cast<T*>(allocator.Realloc(p, newSize * sizeof(T)));
    }

    // Performs an allocate and move operation for movable elements.
    template <typename T, HE_REQUIRES(!IsTriviallyCopyable<T> && IsMoveConstructible<T>)>
    T* _VectorRealloc(Allocator& allocator, T* p, uint32_t size, uint32_t newSize)
    {
        T* mem = static_cast<T*>(allocator.Malloc(newSize * sizeof(T)));
        for (uint32_t i = 0; i < size; ++i)
        {
            new(mem + i) T(Move(*(p + i)));
        }
        allocator.Free(p);
        return mem;
    }

    // Performs an allocate and copy operation for copyable elements.
    template <typename T, HE_REQUIRES(!IsTriviallyCopyable<T> && !IsMoveConstructible<T>)>
    T* _VectorRealloc(Allocator& allocator, T* p, uint32_t size, uint32_t newSize)
    {
        T* mem = static_cast<T*>(allocator.Malloc(newSize * sizeof(T)));
        for (uint32_t i = 0; i < size; ++i)
        {
            new(mem + i) T(*(p + i));
        }
        allocator.Free(p);
        return mem;
    }

    template <typename T>
    Vector<T>::Vector(Allocator& allocator)
        : m_allocator(allocator)
    {}

    template <typename T>
    Vector<T>::Vector(Allocator& allocator, const Vector& x)
        : Vector(allocator)
    {
        CopyFrom(x);
    }

    template <typename T>
    Vector<T>::Vector(Allocator& allocator, Vector&& x)
        : Vector(allocator)
    {
        MoveFrom(Move(x));
    }

    template <typename T>
    Vector<T>::Vector(const Vector& x)
        : Vector(x.m_allocator)
    {
        CopyFrom(x);
    }

    template <typename T>
    Vector<T>::Vector(Vector&& x)
        : Vector(x.m_allocator)
    {
        MoveFrom(Move(x));
    }

    template <typename T>
    Vector<T>::~Vector()
    {
        if (m_data)
        {
            m_allocator.Free(m_data);
        }
    }

    template <typename T>
    Vector<T>& Vector<T>::operator=(const Vector& x)
    {
        CopyFrom(x);
        return *this;
    }

    template <typename T>
    Vector<T>& Vector<T>::operator=(Vector&& x)
    {
        MoveFrom(Move(x));
        return *this;
    }

    template <typename T>
    T& Vector<T>::operator[](uint32_t index)
    {
        HE_ASSERT(index < m_size);
        return m_data[index];
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
            _VectorConstructDefault(m_data + m_size, len - m_size);
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
    void Vector<T>::ShrinkToFit()
    {
        m_data = _VectorRealloc(m_allocator, m_data, m_size, m_size);
        m_capacity = m_size;
    }

    template <typename T>
    T* Vector<T>::Data()
    {
        return m_data;
    }

    template <typename T>
    T* Vector<T>::Begin()
    {
        return m_data;
    }

    template <typename T>
    T* Vector<T>::End()
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
        GrowBy(1);

        if (m_size > index)
        {
            _VectorMove(m_data + index + 1, m_data + index, (m_size - index));
        }

        _VectorMove(m_data + index, &element, 1);
        ++m_size;
    }

    template <typename T>
    void Vector<T>::Insert(uint32_t index, const T* begin, const T* end)
    {
        HE_ASSERT(begin && end);

        const uint32_t len = static_cast<uint32_t>(end - begin);

        GrowBy(len);

        if (m_size > index)
        {
            _VectorMove(m_data + index + len, m_data + index, (m_size - index));
        }

        _VectorCopy(m_data + index, begin, len);
        m_size += len;
    }

    template <typename T>
    void Vector<T>::Erase(uint32_t index, uint32_t count)
    {
        if (count == 0)
            return;

        HE_ASSERT(index + count <= m_size);

        _VectorDestruct(m_data + index, count);

        if (index + count < m_size)
        {
            _VectorMove(m_data + index, m_data + index + count, (m_size - count));
        }

        m_size -= count;
    }

    template <typename T>
    T& Vector<T>::PushBack(const T& element)
    {
        GrowBy(1);
        _VectorCopy(m_data + m_size, &element, 1);
        return m_data[m_size++];
    }

    template <typename T>
    T& Vector<T>::PushBack(T&& element)
    {
        GrowBy(1);
        _VectorMove(m_data + m_size, &element, 1);
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
    T& Vector<T>::EmplaceBack(DefaultInitTag)
    {
        GrowBy(1);
        _VectorConstructDefault(m_data + m_size, 1);
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
    void Vector<T>::GrowBy(uint32_t n)
    {
        if ((m_size + n) <= m_capacity)
            return;

        Reserve(CalculateGrowth(n));
    }

    template <typename T>
    uint32_t Vector<T>::CalculateGrowth(uint32_t n)
    {
        // If our growth would overflow just assume max elements
        if (m_capacity > (MaxElements - (m_capacity / 2)))
            return MaxElements;

        const uint32_t newCapacity = m_capacity + (m_capacity / 2);

        // If normal growth wouldn't be enough, just use the new size
        if (newCapacity < (m_size + n))
            return m_size + n;

        return newCapacity;
    }

    template <typename T>
    void Vector<T>::CopyFrom(const Vector& x)
    {
        Clear();

        Reserve(x.m_size);
        _VectorCopy(m_data, x.m_data, x.m_size);
        m_size = x.m_size;
    }

    template <typename T>
    void Vector<T>::MoveFrom(Vector&& x)
    {
        Clear();

        // If there are different allocators we have to make our own allocation and move everything over.
        if (&m_allocator != &x.m_allocator)
        {
            Reserve(x.m_size);
            _VectorMove(m_data, x.m_data, x.m_size);
            m_size = x.m_size;
            x.Clear();
            return;
        }

        // If the allocators match we can steal the allocation from the other object.
        m_data = Exchange(x.m_data, nullptr);
        m_size = Exchange(x.m_size, 0);
        m_capacity = Exchange(x.m_capacity, 0);
    }
}
