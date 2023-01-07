// Copyright Chad Engler

// This HashTable implementation was originally based on Martin Leitner-Ankerl's hash table,
// which was licensed under the MIT license as of December 2022.
// https://github.com/martinus/unordered_dense
//
// License:
//
// MIT License
//
// Copyright (c) 2022 Martin Leitner-Ankerl
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// TODO: Keys are non-const because of L499 where we move into an entry.
// Need to: 1) fix this so hash map entries use a const key, and 2) make sets have a const key

#pragma once

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/hash.h"
#include "he/core/memory_ops.h"
#include "he/core/span.h"
#include "he/core/types.h"
#include "he/core/type_traits.h"
#include "he/core/vector.h"

#include <algorithm>

namespace he
{
    // --------------------------------------------------------------------------------------------
    /// \internal
    struct HashTableBucket
    {
        static constexpr uint32_t DistanceIncrement = 1u << 8;
        static constexpr uint32_t FingerprintMask = DistanceIncrement - 1;

        /// The distance of the entry from the original hashed location (3 most significant bytes)
        /// and a fingerprint of 1 byte of the hash (lowest significant byte)
        uint32_t distanceAndFingerprint;

        /// An index where in the vector the actual data is stored.
        uint32_t entryIndex;
    };

    /// A densely stored hash table based on robin-hood hasing with backwards shift deletion.
    /// Because the entries in the table are stored in a vector it has a lot of the same properties
    /// as a vector, while also providing for fast lookups and insertions based on hashes of keys.
    /// Removals are a bit slower than other strategies because it requires two lookups.
    ///
    /// \note Usually you don't want to use this class directly, try using \ref HashSet or
    /// \ref HashMap instead.
    ///
    /// \tparam T The hash table traits
    template <typename T>
    class HashTable
    {
    public:
        using Traits = T;
        using KeyType = typename T::KeyType;
        using EntryType = typename T::EntryType;
        using HasherType = typename T::HasherType;
        using EqualType = typename T::EqualType;

        /// An object that provides the result of an insertion operation.
        struct EmplaceResult
        {
            /// A reference to the newly created, or previously existing, entry.
            EntryType& entry;

            /// True if `entry` refers to a newly constructed entry, and false if `entry` points
            /// to an entry that already existed.
            bool inserted;
        };

    public:
        // ----------------------------------------------------------------------------------------
        // Construction

        /// Construct an empty table.
        ///
        /// \param allocator Optional. The allocator to use.
        explicit HashTable(
            const HasherType& hash = HasherType(),
            const EqualType& equal = EqualType(),
            Allocator& allocator = Allocator::GetDefault()) noexcept;

        /// Construct an empty table.
        ///
        /// \param allocator Optional. The allocator to use.
        explicit HashTable(Allocator& allocator) noexcept;

        /// Construct a table by copying `x`, and using `allocator` for this table's allocations.
        ///
        /// \param x The table to copy from.
        /// \param allocator The allocator to use for any allocations.
        HashTable(const HashTable& x, Allocator& allocator) noexcept;

        /// Construct a table by moving `x`, and using `allocator` for this table's allocations.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param x The table to move from.
        /// \param allocator The allocator to use for any allocations.
        HashTable(HashTable&& x, Allocator& allocator) noexcept;

        /// Construct a table by copying `x`, using the allocator from `x`.
        ///
        /// \param x The table to copy from.
        HashTable(const HashTable& x) noexcept;

        /// Construct a table by moving `x`, using the allocator from `x`.
        ///
        /// \param x The table to move from.
        HashTable(HashTable&& x) noexcept;

        /// Destructs the table, freeing any memory allocations.
        ~HashTable() noexcept;

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Copy the table `x` into this table.
        ///
        /// \param x The table to copy from.
        HashTable& operator=(const HashTable& x) noexcept;

        /// Move the table `x` into this table.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param x The table to move from.
        HashTable& operator=(HashTable&& x) noexcept;

        /// Checks if this table is equal to another table.
        ///
        /// \param x The table to check against.
        /// \return True if the tables are equal, false otherwise.
        [[nodiscard]] bool operator==(const HashTable<T>& x) const;

        /// Checks if this table is not equal to another table.
        ///
        /// \param x The table to check against.
        /// \return True if the tables are not equal, false otherwise.
        [[nodiscard]] bool operator!=(const HashTable<T>& x) const { return !this->operator==(x); }

        // ----------------------------------------------------------------------------------------
        // Capacity

        /// Checks if this table is empty.
        ///
        /// \return Returns true if this is an empty table.
        [[nodiscard]] bool IsEmpty() const { return m_entries.IsEmpty(); }

        /// The length of the table that is currently stored.
        ///
        /// \return Number of entries in the table.
        [[nodiscard]] uint32_t Size() const { return m_entries.Size(); }

        /// The load factor of the hash table. This is a representation of how "full" the table is,
        /// where 0.0 is empty and 1.0 is completely full. Once this value reached the value
        /// returned by \ref MaxLoadFactor() memory growth and a rehash will occur.
        ///
        /// \return The current load factor.
        [[nodiscard]] float LoadFactor() const { return m_bucketCount ? static_cast<float>(Size()) / m_bucketCount : 0.0f; }

        /// This is the maximum load factor the table will have before an insertion will cause
        /// memory growth and a rehash.
        ///
        /// \return The maximum load factor.
        [[nodiscard]] float MaxLoadFactor() const { return m_maxLoadFactor; }

        /// The maximum load factor of the table before insertion will cause memory growth and a
        /// rehash.
        ///
        /// \param[in] factor The new maximum load factor.
        void SetMaxLoadFactor(float factor);

        /// The number of buckets
        [[nodiscard]] uint32_t BucketCount() const { return m_bucketCount; }

        /// Reserves capacity for `len` entries. May cause a rehash.
        ///
        /// \param len The length of entries to reserve capacity for.
        void Reserve(uint32_t len);

        /// Shrinks the memory allocation to fit the current size of the table.
        ///
        /// This may not free memory if the size of the table is below a minimum size it holds by
        /// default.
        void ShrinkToFit();

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Gets a pointer to the entries array.
        ///
        /// \return A pointer to the entries array.
        [[nodiscard]] const EntryType* Data() const { return m_entries.Data(); }

        /// \copydoc Data
        [[nodiscard]] EntryType* Data() { return m_entries.Data(); }

        /// Checks if a key is contained within the table.
        ///
        /// \return Returns true if the key was found, or false otherwise.
        template <typename K>
        [[nodiscard]] bool Contains(const K& key) const { return Find(key) != nullptr; }

        /// Search for an entry by key.
        ///
        /// \note The returned pointer is invalided if a rehash occurs.
        ///
        /// \return A pointer to the found entry, or nullptr if no entry was found.
        template <typename K>
        [[nodiscard]] const EntryType* Find(const K& key) const;

        /// \copydoc Find
        template <typename K>
        [[nodiscard]] EntryType* Find(const K& key) { return const_cast<EntryType*>(const_cast<const HashTable*>(this)->Find(key)); }

        /// Search for an entry by key.
        ///
        /// \note The returned pointer is invalided if a rehash occurs.
        ///
        /// \return A pointer to the found entry, or nullptr if no entry was found.
        template <typename K>
        [[nodiscard]] const EntryType& Get(const K& key) const;

        /// \copydoc Get
        template <typename K>
        [[nodiscard]] EntryType& Get(const K& key) { return const_cast<EntryType&>(const_cast<const HashTable*>(this)->Get(key)); }

        /// Get a span of all entries in the table.
        ///
        /// \return Span of entries
        Span<const EntryType> Entries() const { return m_entries; }

        /// Get a span of all entries in the table.
        ///
        /// \return Span of entries
        Span<EntryType> Entries() { return m_entries; }

        /// Adopts the vector of entries and discards any ionternally held entries.
        ///
        /// \param data A pointer to the memory to adopt.
        /// \param size Number of valid elements in the adopted memory.
        /// \param capcity Number of allocated elements in the adopted memory.
        void Adopt(Vector<EntryType>&& entries);

        /// Releases the vector of entries and returns ownership to the caller.
        ///
        /// After calling this method the table is reset to a valid empty state and can be used
        /// again, which creates a new allocation of memory.
        ///
        /// \return The vector of table entries.
        [[nodiscard]] Vector<EntryType> Release() { return Move(m_entries); }

        /// Returns a reference to the allocator object used by the table.
        ///
        /// \return The allocator object this table uses.
        [[nodiscard]] Allocator& GetAllocator() const { return m_entries.GetAllocator(); }

        // ----------------------------------------------------------------------------------------
        // Iterators

        /// Gets a pointer to the first element in the table.
        ///
        /// \return A pointer to the first element.
        [[nodiscard]] const EntryType* Begin() const { return m_entries.Begin(); }

        /// \copydoc Begin()
        [[nodiscard]] EntryType* Begin() { return m_entries.Begin(); }

        /// Gets a pointer to one past the last element in the table.
        ///
        /// \return A pointer to one past the last element.
        [[nodiscard]] const EntryType* End() const { return m_entries.End(); }

        /// \copydoc End()
        [[nodiscard]] EntryType* End() { return m_entries.End(); }

        /// \copydoc Begin()
        [[nodiscard]] const EntryType* begin() const { return m_entries.begin(); }

        /// \copydoc Begin()
        [[nodiscard]] EntryType* begin() { return m_entries.begin(); }

        /// \copydoc End()
        [[nodiscard]] const EntryType* end() const { return m_entries.end(); }

        /// \copydoc End()
        [[nodiscard]] EntryType* end() { return m_entries.end(); }

        // ----------------------------------------------------------------------------------------
        // Mutators

        /// Sets the size of the table to zero, and destructs elements as necessary.
        /// Does not affect memory allocation.
        void Clear() { m_entries.Clear(); ClearBuckets(); }

        /// Erase an entry from the table using the key of that entry.
        ///
        /// \param[in] key The of the entry to erase.
        /// \return True if an entry was erased, false otherwise.
        template <typename K>
        bool Erase(K&& key);

        /// Constructs an entry in-place if the key does not exist, does nothing if the key exists.
        /// If additional arguments are passed beyond the key, they'll be forwarded to the
        /// constructor of the value type *only if* a new entry is inserted. If an existing entry
        /// is found, then the arguments are untouched.
        ///
        /// The result object returned contains a reference to the newly created entry if one was
        /// created, or the existing entry if one was found. Also holds a bool that is true when a
        /// new entry was created.
        ///
        /// \param[in] key The key of the entry to construct in-place.
        /// \param[in] args... Additional arguments used to construct the entry in-place. For a
        ///     HashMap these are forwarded to the value's constructor.
        /// \return The result of the insertion.
        template <typename K, typename... Args>
        EmplaceResult Emplace(K&& key, Args&&... args);

    protected:
        // 2^(32-m_shift) number of buckets
        static constexpr uint8_t InitialShifts = 64 - 3;
        static constexpr float DefaultMaxLoadFactor = 0.8f;

        using Bucket = HashTableBucket;
        static_assert(std::is_trivially_destructible_v<Bucket>);
        static_assert(std::is_trivially_copyable_v<Bucket>);

        [[nodiscard]] uint32_t Next(uint32_t bucketIndex) const;

        [[nodiscard]] static constexpr uint32_t DistAdd(uint32_t x);
        [[nodiscard]] static constexpr uint32_t DistSub(uint32_t x);

        [[nodiscard]] constexpr uint32_t DistAndFingerprintFromHash(uint64_t hash) const;
        [[nodiscard]] constexpr uint32_t BucketIndexFromHash(uint64_t hash) const;

        template <typename K>
        [[nodiscard]] Bucket NextWhileLess(const K& key) const;

        void PlaceAndShiftUp(Bucket bucket, uint32_t place);

        [[nodiscard]] static constexpr uint32_t MaxBucketCount();
        [[nodiscard]] static constexpr uint32_t CalculateBucketCount(uint8_t shifts);
        [[nodiscard]] constexpr uint8_t CalculateShiftsForSize(uint32_t size) const;

        void CopyBuckets(const HashTable& x);
        [[nodiscard]] bool IsFull() const;

        void DeallocateBuckets();
        void AllocateBucketsFromShift();
        void ClearBuckets();
        void ClearAndFillBucketsFromEntries();
        void GrowToNextShift();

        void DoErase(uint32_t bucketIndex);

        template <typename K>
        [[nodiscard]] uint32_t FindIndex(const K& key, uint32_t& distanceAndFingerprint, uint32_t& bucketIndex) const;

    protected:
        friend class HashTableTestAttorney;

        Vector<EntryType> m_entries{};
        Bucket* m_buckets{ nullptr };
        uint32_t m_bucketCount{ 0 };
        uint32_t m_bucketCapacity{ 0 };
        float m_maxLoadFactor{ DefaultMaxLoadFactor };
        HasherType m_hash{};
        EqualType m_equal{};
        uint8_t m_shifts{ InitialShifts };
    };

    // --------------------------------------------------------------------------------------------
    /// Traits used by the \ref HashTable to implement a \ref HashSet
    ///
    /// \internal
    template <typename T, typename H, typename E>
    struct HashSetTraits
    {
        using KeyType = T;
        using HasherType = H;
        using EqualType = E;
        using EntryType = KeyType;

        [[nodiscard]] static constexpr const KeyType& GetKey(const EntryType& entry)
        {
            return entry;
        }
    };

    /// A dense hash set based on \ref HashTable.
    ///
    /// \tparam T The type of data in this set.
    /// \tparam H Optional. The type of the key hasher functor for this set.
    /// \tparam E Optional. The type of the key equality functor for this set.
    template <typename T, typename H = Hasher<T>, typename E = EqualTo<T>>
    class HashSet : public HashTable<HashSetTraits<T, H, E>>
    {
    public:
        using Super = HashTable<HashSetTraits<T, H, E>>;
        using Traits = typename Super::Traits;
        using KeyType = typename Super::KeyType;
        using EntryType = typename Super::EntryType;
        using HasherType = typename Super::HasherType;
        using EqualType = typename Super::EqualType;
        using InsertResult = typename Super::EmplaceResult;

        using ValueType = T;

    public:
        InsertResult Insert(KeyType&& key) { return Super::Emplace(Move(key)); }
        InsertResult Insert(const KeyType& key) { return Super::Emplace(key); }

    private:
        using Super::Emplace;
    };

    // --------------------------------------------------------------------------------------------
    /// A \ref HashMap entry that is stored in the \ref HashTable implementation.
    template <typename K, typename V>
    struct HashMapEntry
    {
        template <typename U>
        explicit HashMapEntry(U&& k) : key(Forward<U>(k)), value() {}

        template <typename U, typename... Args>
        explicit HashMapEntry(U&& k, Args&&... args) : key(Forward<U>(k)), value(Forward<Args>(args)...) {}

        /// The key for this map entry.
        K key;

        /// The value for this map entry.
        V value;
    };

    /// Traits used by the \ref HashTable to implement a \ref HashMap
    ///
    /// \internal
    template <typename K, typename V, typename H, typename E>
    struct HashMapTraits
    {
        using KeyType = K;
        using HasherType = H;
        using EqualType = E;
        using EntryType = HashMapEntry<K, V>;

        [[nodiscard]] static constexpr const KeyType& GetKey(const EntryType& entry)
        {
            return entry.key;
        }
    };

    /// A dense hash map based on \ref HashTable.
    ///
    /// \tparam K The type of keys in this map.
    /// \tparam V The type of values in the map.
    /// \tparam H Optional. The type of the key hasher functor for this map.
    /// \tparam E Optional. The type of the key equality functor for this map.
    template <typename K, typename V, typename H = Hasher<K>, typename E = EqualTo<K>>
    class HashMap : public HashTable<HashMapTraits<K, V, H, E>>
    {
    public:
        using Super = HashTable<HashMapTraits<K, V, H, E>>;
        using Traits = typename Super::Traits;
        using KeyType = typename Super::KeyType;
        using EntryType = typename Super::EntryType;
        using HasherType = typename Super::HasherType;
        using EqualType = typename Super::EqualType;
        using EmplaceResult = typename Super::EmplaceResult;

        using ValueType = V;

    public:
        // pull in some functions from HashTable
        using Super::Super;
        using Super::Emplace;

        /// Accesses or inserts an entry with the specified `key`.
        ///
        /// \param[in] key The key of the entry to find or insert.
        /// \return A reference to the entry.
        template <typename U>
        ValueType& operator[](U&& key) { return Emplace(Forward<U>(key)).entry.value; }

        /// \copydoc HashTable::Emplace
        template <typename U, typename X>
        EmplaceResult EmplaceOrAssign(U&& key, X&& value);

        /// \copydoc HashTable::Find
        template <typename U>
        [[nodiscard]] const ValueType* Find(const U& key) const;

        /// \copydoc HashTable::Find
        template <typename U>
        [[nodiscard]] ValueType* Find(const U& key);

        /// \copydoc HashTable::Get
        template <typename U>
        [[nodiscard]] const ValueType& Get(const U& key) const { return Super::Get(key).value; }

        /// \copydoc HashTable::Get
        template <typename U>
        [[nodiscard]] ValueType& Get(const U& key) { return Super::Get(key).value; }
    };
}

#include "he/core/inline/hash_table.inl"
