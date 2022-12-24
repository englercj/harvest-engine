// Copyright Chad Engler

namespace he
{
    template <typename T>
    HashTable<T>::HashTable(const HasherType& hash, const EqualType& equal, Allocator& allocator) noexcept
        : m_entries(allocator)
        , m_hash(hash)
        , m_equal(equal)
    {}

    template <typename T>
    HashTable<T>::HashTable(Allocator& allocator) noexcept
        : HashTable(HasherType(), EqualType(), allocator)
    {}

    template <typename T>
    HashTable<T>::HashTable(const HashTable& x, Allocator& allocator) noexcept
        : m_entries(x.m_entries, allocator)
        , m_maxLoadFactor(x.m_maxLoadFactor)
        , m_hash(x.m_hash)
        , m_equal(x.m_equal)
    {
        CopyBuckets(x);
    }

    template <typename T>
    HashTable<T>::HashTable(HashTable&& x, Allocator& allocator) noexcept
        : m_entries(Move(x.m_entries), allocator)
        , m_buckets(Exchange(x.m_buckets, nullptr))
        , m_bucketCount(Exchange(x.m_bucketCount, 0))
        , m_bucketCapacity(Exchange(x.m_bucketCapacity, 0))
        , m_maxLoadFactor(Exchange(x.m_maxLoadFactor, DefaultMaxLoadFactor))
        , m_hash(Exchange(x.m_hash, {}))
        , m_equal(Exchange(x.m_equal, {}))
        , m_shifts(Exchange(x.m_shifts, InitialShifts))
    {}

    template <typename T>
    HashTable<T>::HashTable(const HashTable& x) noexcept
        : HashTable(x, x.GetAllocator())
    {}

    template <typename T>
    HashTable<T>::HashTable(HashTable&& x) noexcept
        : HashTable(Move(x), x.GetAllocator())
    {}

    template <typename T>
    HashTable<T>::~HashTable() noexcept
    {
        GetAllocator().Free(m_buckets);
    }

    template <typename T>
    HashTable<T>& HashTable<T>::operator=(const HashTable& x) noexcept
    {
        if (this != &x)
        {
            DeallocateBuckets();
            m_entries = x.m_entries;
            m_maxLoadFactor = x.m_maxLoadFactor;
            m_hash = x.m_hash;
            m_equal = x.m_equal;
            m_shifts = x.m_shifts;
            CopyBuckets(x);
        }
        return *this;
    }

    template <typename T>
    HashTable<T>& HashTable<T>::operator=(HashTable&& x) noexcept
    {
        if (this != &x)
        {
            DeallocateBuckets();
            m_entries = Move(x.m_entries);
            m_buckets = Exchange(x.m_buckets, nullptr);
            m_bucketCount = Exchange(x.m_bucketCount, 0);
            m_bucketCapacity = Exchange(x.m_bucketCapacity, 0);
            m_maxLoadFactor = Exchange(x.m_maxLoadFactor, DefaultMaxLoadFactor);
            m_hash = Exchange(x.m_hash, {});
            m_equal = Exchange(x.m_equal, {});
            m_shifts = Exchange(x.m_shifts, InitialShifts);
        }
        return *this;
    }

    template <typename T>
    bool HashTable<T>::operator==(const HashTable<T>& x) const
    {
        if (this == &x)
            return true;

        if (Size() != x.Size())
            return false;

        return std::equal(x.Begin(), x.End(), Begin(), End());
    }

    template <typename T>
    void HashTable<T>::SetMaxLoadFactor(float factor)
    {
        m_maxLoadFactor = factor;
        if (m_bucketCount != MaxBucketCount())
        {
            m_bucketCapacity = static_cast<uint32_t>(m_bucketCount * m_maxLoadFactor);
        }
    }

    template <typename T>
    void HashTable<T>::Reserve(uint32_t len)
    {
        len = Min(len, MaxBucketCount());
        m_entries.Reserve(len);

        const uint8_t shifts = CalculateShiftsForSize(Max(len, Size()));
        if (m_bucketCount == 0 || shifts < m_shifts)
        {
            m_shifts = shifts;
            DeallocateBuckets();
            AllocateBucketsFromShift();
            ClearAndFillBucketsFromEntries();
        }
    }

    template <typename T>
    void HashTable<T>::ShrinkToFit()
    {
        const uint8_t shifts = CalculateShiftsForSize(Size());
        if (shifts != m_shifts)
        {
            m_shifts = shifts;
            DeallocateBuckets();
            m_entries.ShrinkToFit();
            AllocateBucketsFromShift();
            ClearAndFillBucketsFromEntries();
        }
    }

    template <typename T>
    template <typename K>
    const typename HashTable<T>::EntryType* HashTable<T>::Find(const K& key) const
    {
        if (IsEmpty()) [[unlikely]]
            return nullptr;

        const uint64_t hash = m_hash(key);
        uint32_t distanceAndFingerprint = DistAndFingerprintFromHash(hash);
        uint32_t bucketIndex = BucketIndexFromHash(hash);
        const Bucket* bucket = m_buckets + bucketIndex;

        // We check a couple buckets outside the loop first because this manual loop unroll
        // results in an improved lookup time on average. This is because the loop is actually
        // the degenerate case where an entry has been overflowed multiple times.
        if (distanceAndFingerprint == bucket->distanceAndFingerprint && m_equal(key, Traits::GetKey(m_entries[bucket->entryIndex])))
        {
            return Begin() + bucket->entryIndex;
        }

        distanceAndFingerprint = DistAdd(distanceAndFingerprint);
        bucketIndex = Next(bucketIndex);
        bucket = m_buckets + bucketIndex;

        if (distanceAndFingerprint == bucket->distanceAndFingerprint && m_equal(key, Traits::GetKey(m_entries[bucket->entryIndex])))
        {
            return Begin() + bucket->entryIndex;
        }

        while (true)
        {
            distanceAndFingerprint = DistAdd(distanceAndFingerprint);
            bucketIndex = Next(bucketIndex);
            bucket = m_buckets + bucketIndex;

            if (distanceAndFingerprint == bucket->distanceAndFingerprint)
            {
                if (m_equal(key, Traits::GetKey(m_entries[bucket->entryIndex])))
                {
                    return Begin() + bucket->entryIndex;
                }
            }
            else if (distanceAndFingerprint > bucket->distanceAndFingerprint)
            {
                return nullptr;
            }
        }
    }

    template <typename T>
    template <typename K>
    const typename HashTable<T>::EntryType& HashTable<T>::Get(const K& key) const
    {
        const EntryType* entry = Find(key);
        HE_ASSERT(entry);
        return *entry;
    }

    template <typename T>
    void HashTable<T>::Adopt(Vector<EntryType>&& entries)
    {
        HE_ASSERT(entries.Size() <= MaxBucketCount());

        const uint8_t shifts = CalculateShiftsForSize(entries.Size());
        const bool needsRealloc = m_bucketCount == 0 || shifts < m_shifts || &entries.GetAllocator() != &m_entries.GetAllocator();

        m_entries = Move(entries);
        if (needsRealloc)
        {
            m_shifts = shifts;
            DeallocateBuckets();
            AllocateBucketsFromShift();
        }
        ClearBuckets();

        // We can't use ClearAndFillBucketsFromEntries() because the incoming entries might not be
        // unique. So loop until we reach the end of the incoming container. Duplicated entries
        // will be replaced with Back().
        uint32_t index = 0;
        while (index != m_entries.Size())
        {
            const KeyType& key = Traits::GetKey(m_entries[index]);
            const uint64_t hash = m_hash(key);
            uint32_t distanceAndFingerprint = DistAndFingerprintFromHash(hash);
            uint32_t bucketIndex = BucketIndexFromHash(hash);

            bool found = false;
            while (true)
            {
                const Bucket& bucket = m_buckets[bucketIndex];
                if (distanceAndFingerprint > bucket.distanceAndFingerprint)
                    break;

                if (distanceAndFingerprint == bucket.distanceAndFingerprint
                    && m_equal(key, Traits::GetKey(m_entries[bucket.entryIndex])))
                {
                    found = true;
                    break;
                }

                distanceAndFingerprint = DistAdd(distanceAndFingerprint);
                bucketIndex = Next(bucketIndex);
            }

            if (found)
            {
                if (index != (m_entries.Size() - 1))
                    m_entries[index] = Move(m_entries.Back());

                m_entries.PopBack();
            }
            else
            {
                PlaceAndShiftUp({ distanceAndFingerprint, index }, bucketIndex);
                ++index;
            }
        }
    }

    template <typename T>
    template <typename K>
    bool HashTable<T>::Erase(K&& key)
    {
        if (IsEmpty())
            return false;

        auto [distanceAndFingerprint, bucketIndex] = NextWhileLess(key);

        while (distanceAndFingerprint == m_buckets[bucketIndex].distanceAndFingerprint
            && !m_equal(key, Traits::GetKey(m_entries[m_buckets[bucketIndex].entryIndex])))
        {
            distanceAndFingerprint = DistAdd(distanceAndFingerprint);
            bucketIndex = Next(bucketIndex);
        }

        if (distanceAndFingerprint != m_buckets[bucketIndex].distanceAndFingerprint)
            return false;

        DoErase(bucketIndex);
        return true;
    }

    template <typename T>
    template <typename K, typename... Args>
    HashTable<T>::EmplaceResult HashTable<T>::Emplace(K&& key, Args&&... args)
    {
        if (IsFull())
            GrowToNextShift();

        const uint64_t hash = m_hash(key);
        uint32_t distanceAndFingerprint = DistAndFingerprintFromHash(hash);
        uint32_t bucketIndex = BucketIndexFromHash(hash);

        while (distanceAndFingerprint <= m_buckets[bucketIndex].distanceAndFingerprint)
        {
            const Bucket& bucket = m_buckets[bucketIndex];
            if (distanceAndFingerprint == bucket.distanceAndFingerprint
                && m_equal(key, Traits::GetKey(m_entries[bucket.entryIndex])))
            {
                return { m_entries[bucket.entryIndex], false };
            }

            distanceAndFingerprint = DistAdd(distanceAndFingerprint);
            bucketIndex = Next(bucketIndex);
        }

        const uint32_t index = m_entries.Size();
        m_entries.EmplaceBack(Forward<K>(key), Forward<Args>(args)...);
        PlaceAndShiftUp({ distanceAndFingerprint, index }, bucketIndex);
        return { m_entries.Back(), true };
    }

    template <typename T>
    uint32_t HashTable<T>::Next(uint32_t bucketIndex) const
    {
        if ((bucketIndex + 1) == m_bucketCount) [[unlikely]]
            return 0;

        return bucketIndex + 1;
    }

    template <typename T>
    constexpr uint32_t HashTable<T>::DistAdd(uint32_t x)
    {
        return x + Bucket::DistanceIncrement;
    }

    template <typename T>
    constexpr uint32_t HashTable<T>::DistSub(uint32_t x)
    {
        return x - Bucket::DistanceIncrement;
    }

    template <typename T>
    constexpr uint32_t HashTable<T>::DistAndFingerprintFromHash(uint64_t hash) const
    {
        return Bucket::DistanceIncrement | (static_cast<uint32_t>(hash) & Bucket::FingerprintMask);
    }

    template <typename T>
    constexpr uint32_t HashTable<T>::BucketIndexFromHash(uint64_t hash) const
    {
        return static_cast<uint32_t>(hash >> m_shifts);
    }

    template <typename T>
    template <typename K>
    HashTable<T>::Bucket HashTable<T>::NextWhileLess(const K& key) const
    {
        const uint64_t hash = m_hash(key);
        uint32_t distanceAndFingerprint = DistAndFingerprintFromHash(hash);
        uint32_t bucketIndex = BucketIndexFromHash(hash);

        while (distanceAndFingerprint < m_buckets[bucketIndex].distanceAndFingerprint)
        {
            distanceAndFingerprint = DistAdd(distanceAndFingerprint);
            bucketIndex = Next(bucketIndex);
        }

        return { distanceAndFingerprint, bucketIndex };
    }

    template <typename T>
    void HashTable<T>::PlaceAndShiftUp(Bucket bucket, uint32_t place)
    {
        while (m_buckets[place].distanceAndFingerprint != 0)
        {
            bucket = Exchange(m_buckets[place], bucket);
            bucket.distanceAndFingerprint = DistAdd(bucket.distanceAndFingerprint);
            place = Next(place);
        }

        m_buckets[place] = bucket;
    }

    template <typename T>
    constexpr uint32_t HashTable<T>::MaxBucketCount()
    {
        constexpr uint32_t shift = (sizeof(uint32_t) * 8) - 1;
        return 1u << shift;
    }

    template <typename T>
    constexpr uint32_t HashTable<T>::CalculateBucketCount(uint8_t shifts)
    {
        return Min(MaxBucketCount(), 1u << (64 - shifts));
    }

    template <typename T>
    constexpr uint8_t HashTable<T>::CalculateShiftsForSize(uint32_t size) const
    {
        uint8_t shifts = InitialShifts;
        while (shifts > 0 && static_cast<uint32_t>(CalculateBucketCount(shifts) * m_maxLoadFactor) < size)
            --shifts;
        return shifts;
    }

    template <typename T>
    void HashTable<T>::CopyBuckets(const HashTable& x)
    {
        if (!IsEmpty())
        {
            m_shifts = x.m_shifts;
            AllocateBucketsFromShift();
            MemCopy(m_buckets, x.m_buckets, sizeof(Bucket) * m_bucketCount);
        }
    }

    template <typename T>
    bool HashTable<T>::IsFull() const
    {
        return Size() >= m_bucketCapacity;
    }

    template <typename T>
    void HashTable<T>::DeallocateBuckets()
    {
        if (m_buckets)
        {
            GetAllocator().Free(m_buckets);
        }
        m_buckets = nullptr;
        m_bucketCount = 0;
        m_bucketCapacity = 0;
    }

    template <typename T>
    void HashTable<T>::AllocateBucketsFromShift()
    {
        m_bucketCount = CalculateBucketCount(m_shifts);
        m_buckets = GetAllocator().Malloc<Bucket>(m_bucketCount);

        if (m_bucketCount == MaxBucketCount())
        {
            // reached the maximum, make sure we can use each bucket
            m_bucketCapacity = MaxBucketCount();
        }
        else
        {
            m_bucketCapacity = static_cast<uint32_t>(m_bucketCount * m_maxLoadFactor);
        }
    }

    template <typename T>
    void HashTable<T>::ClearBuckets()
    {
        if (m_buckets)
        {
            MemZero(m_buckets, sizeof(Bucket) * m_bucketCount);
        }
    }

    template <typename T>
    void HashTable<T>::ClearAndFillBucketsFromEntries()
    {
        ClearBuckets();
        for (uint32_t i = 0; i < m_entries.Size(); ++i)
        {
            const KeyType& key = Traits::GetKey(m_entries[i]);
            const auto [distanceAndFingerprint, bucketIndex] = NextWhileLess(key);

            // we know for certain that key has not yet been inserted, so no need to check it.
            PlaceAndShiftUp({ distanceAndFingerprint, i }, bucketIndex);
        }
    }

    template <typename T>
    void HashTable<T>::GrowToNextShift()
    {
        HE_ASSERT(m_bucketCapacity != MaxBucketCount());
        --m_shifts;
        DeallocateBuckets();
        AllocateBucketsFromShift();
        ClearAndFillBucketsFromEntries();
    }

    template <typename T>
    void HashTable<T>::DoErase(uint32_t bucketIndex)
    {
        const uint32_t indexToRemove = m_buckets[bucketIndex].entryIndex;

        // shift down until either empty or an element with correct spot is found
        uint32_t nextBucketIndex = Next(bucketIndex);
        while (m_buckets[nextBucketIndex].distanceAndFingerprint >= (Bucket::DistanceIncrement * 2))
        {
            const Bucket& nextBucket = m_buckets[nextBucketIndex];
            m_buckets[bucketIndex] = { DistSub(nextBucket.distanceAndFingerprint), nextBucket.entryIndex };
            bucketIndex = Exchange(nextBucketIndex, Next(nextBucketIndex));
        }
        m_buckets[bucketIndex] = {};

        // update the entries array
        const uint32_t lastIndex = m_entries.Size() - 1;
        if (indexToRemove != lastIndex)
        {
            // no luck, we'll have to replace the value with the last one and update the index accordingly
            EntryType& entry = m_entries[indexToRemove];
            entry = Move(m_entries.Back());

            // update the entryIndex of the moved entry.
            // No need to play the info game, just look until we find the entryIndex
            const KeyType& key = Traits::GetKey(entry);
            const uint64_t hash = m_hash(key);
            bucketIndex = BucketIndexFromHash(hash);

            while (m_buckets[bucketIndex].entryIndex != lastIndex)
                bucketIndex = Next(bucketIndex);

            m_buckets[bucketIndex].entryIndex = indexToRemove;
        }

        m_entries.PopBack();
    }

    template <typename K, typename V, typename H, typename E>
    template <typename U, typename X>
    typename HashMap<K, V, H, E>::EmplaceResult HashMap<K, V, H, E>::EmplaceOrAssign(U&& key, X&& value)
    {
        const EmplaceResult result = Super::Emplace(Forward<U>(key));
        if (!result.inserted)
        {
            result.entry.value = Forward<X>(value);
        }
        return result;
    }

    template <typename K, typename V, typename H, typename E>
    template <typename U>
    typename const HashMap<K, V, H, E>::ValueType* HashMap<K, V, H, E>::Find(const U& key) const
    {
        const EntryType* v = Super::Find(key);
        return v ? &v->value : nullptr;
    }

    template <typename K, typename V, typename H, typename E>
    template <typename U>
    typename HashMap<K, V, H, E>::ValueType* HashMap<K, V, H, E>::Find(const U& key)
    {
        EntryType* v = Super::Find(key);
        return v ? &v->value : nullptr;
    }
}
