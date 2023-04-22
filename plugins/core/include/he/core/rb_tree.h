// Copyright Chad Engler

// This left-leaning red-black tree implementation was originally based on Jason Evans'
// macro-based implementation, which is used heavily in jemalloc.
// (c) Jason Evans - http://www.canonware.com/rb/
//
// License:
//
// Copyright (C) 2008 Jason Evans <jasone@FreeBSD.org>.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice(s), this list of conditions and the following disclaimer
//    unmodified other than the allowable addition of one or more
//    copyright notices.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice(s), this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace std { struct bidirectional_iterator_tag; }

namespace he
{
    // --------------------------------------------------------------------------------------------
    /// A link that is used in node structures for the left-leaning red-black tree.
    ///
    /// Example usage:
    /// ```cpp
    /// struct MyEntry
    /// {
    ///     uint32_t data;
    ///     RBTreeLink<MyEntry> link;
    /// };
    /// ```
    ///
    /// \tparam T The containing type this link tracks.
    template <typename T>
    struct RBTreeLink
    {
        T* left;
        T* rightRed;
    };

    // --------------------------------------------------------------------------------------------
    /// A left-leaning red-black tree which tracks nodes that are allocated outside the container.
    /// Node structures are user-defined, but must have a \ref RBTreeLink member, a pointer to
    /// which is the second template parameter. Keys are expected to be well-ordered, and support
    /// comparisons with `==` and `<`. Uniqueness of keys is determined by comparing them with `==`.
    ///
    /// \note Entries are guaranteed to be ordered by their key during iteration.
    ///
    /// Example usage:
    /// ```cpp
    /// struct MyEntry
    /// {
    ///     uint32_t key;
    ///     RBTreeLink<MyEntry> link;
    /// };
    /// RBTree<MyEntry, &MyEntry::link, uint32_t, &MyEntry::key> myTree;
    /// ```
    ///
    /// \tparam T The node type for entries in the tree.
    /// \tparam Link Pointer to member data for the link in the node type.
    /// \tparam K The key type for entries in the tree, must be well-ordered.
    /// \tparam Key Pointer to member data for the key in the node type.
    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    class RBTree final
    {
    public:
        using EntryType = T;
        using KeyType = K;
        using TreeType = RBTree<T, Link, K, Key>;

        using Pfn_ClearHandler = void(*)(T*, void*);
        using Pfn_AllocNodeCopy = T*(*)(const T*, void*);

        static constexpr uint32_t MaxDepth = sizeof(void*) << 4;

    public:
        RBTree() = default;
        RBTree(const RBTree&) = delete;
        RBTree(RBTree&& x) : m_root(Exchange(x.m_root, nullptr)) {}

        RBTree& operator=(const RBTree& x) = delete;
        RBTree& operator=(RBTree&& x);

        bool IsEmpty() const { return m_root == nullptr; }
        T* First() const { return First(m_root); }
        T* Last() const { return Last(m_root); }

        T* Next(T* node) const;
        T* Prev(T* node) const;

        void Clear(Pfn_ClearHandler handler = nullptr, void* userData = nullptr);
        T* Find(const K& key) const;

        void Insert(T* node);
        T* Remove(const K& key);

        void CopyFrom(const RBTree& other, Pfn_AllocNodeCopy alloc, void* userData = nullptr);

    private:
        void ClearInternal(T* node, Pfn_ClearHandler handler, void* userData);
        void CopyNode(T* to, T* from, Pfn_AllocNodeCopy alloc, void* userData);

    private:
        struct PathEntry
        {
            T* node;
            int cmp;
        };

        static constexpr uintptr_t RedFlag = 1;
        static constexpr uintptr_t RedMask = ~RedFlag;

        static void Init(T* node);

        static T* Left(const T* node);
        static void SetLeft(T* node, T* left);

        static T* Right(const T* node);
        static void SetRight(T* node, T* right);

        static bool IsRed(const T* node);
        static void SetRed(T* node);
        static void SetBlack(T* node);
        static void SetColor(T* node, bool red);

        static T* RotateLeft(T* node);
        static T* RotateRight(T* node);

        static T* First(T* node);
        static T* Last(T* node);

    private:
        T* m_root{ nullptr };
    };


    // --------------------------------------------------------------------------------------------
    /// Base class for containers build atop the left-leaning red-black tree. The traits template
    /// parameter is expected to define types for keys, entry structures, and the RB tree to use.
    /// Keys for the tree are expected to be well-ordered, and support comparisons with `==` and `<`.
    ///
    /// \note Entries are guaranteed to be ordered by their key during iteration.
    ///
    /// \note Pointers to entries are valid for the lifetime of that entry, even if the container
    /// is modified. This is because entries are individually allocated.
    ///
    /// \note Usually you don't want to use this class directly, try using \ref RBTreeSet or
    /// \ref RBTreeMap instead.
    ///
    /// \tparam T The tree container traits.
    template <typename T>
    class RBTreeContainerBase
    {
    public:
        // ----------------------------------------------------------------------------------------
        // Types

        using Traits = T;
        using KeyType = typename T::KeyType;
        using EntryType = typename T::EntryType;
        using TreeType = typename T::TreeType;

        /// An object that provides the result of an insertion operation.
        struct EmplaceResult final
        {
            /// A reference to the newly created, or previously existing, entry.
            EntryType& entry;

            /// True if `entry` refers to a newly constructed entry, and false if `entry` points
            /// to an entry that already existed.
            bool inserted;
        };

        // ----------------------------------------------------------------------------------------
        // Iterator

        template <bool Const>
        class IteratorBase final
        {
        public:
            using ElementType = Conditional<Const, const EntryType, EntryType>;

            using difference_type = uint32_t;
            using value_type = ElementType;
            using container_type = RBTreeContainerBase;
            using iterator_category = std::bidirectional_iterator_tag;
            using _Unchecked_type = IteratorBase; // Mark iterator as checked.

        public:
            IteratorBase() = default;
            IteratorBase(const TreeType& tree) noexcept : m_tree(&tree), m_node(tree.First()) {}

            ElementType& operator*() const { return *m_node; }
            ElementType* operator->() const { return m_node; }

            IteratorBase& operator++() { m_node = m_tree->Next(m_node); return *this; }
            IteratorBase operator++(int) { IteratorBase x(*this); m_node = m_tree->Next(m_node); return x; }

            IteratorBase& operator--() { m_node = m_tree->Prev(m_node); return *this; }
            IteratorBase operator--(int) { IteratorBase x(*this); m_node = m_tree->Prev(m_node); return x; }

            [[nodiscard]] IteratorBase operator+(uint32_t n) const
            {
                IteratorBase x(*this);
                for (uint32_t i = 0; i < n; ++i)
                    x.m_node = x.m_tree->Next(x.m_node);
                return x;
            }

            [[nodiscard]] IteratorBase operator-(uint32_t n) const
            {
                IteratorBase x(*this);
                for (uint32_t i = 0; i < n; ++i)
                    x.m_node = x.m_tree->Prev(x.m_node);
                return x;
            }

            [[nodiscard]] bool operator==(const IteratorBase& x) const { return m_node == x.m_node; }
            [[nodiscard]] bool operator!=(const IteratorBase& x) const { return m_node != x.m_node; }

            [[nodiscard]] explicit operator bool() const { return m_node != nullptr; }

        private:
            const TreeType* m_tree{ nullptr };
            EntryType* m_node{ nullptr };
        };

        using Iterator = IteratorBase<false>;
        using ConstIterator = IteratorBase<true>;

    public:
        // ----------------------------------------------------------------------------------------
        // Construction

        /// Construct an empty container.
        ///
        /// \param allocator Optional. The allocator to use.
        explicit RBTreeContainerBase(Allocator& allocator = Allocator::GetDefault()) noexcept;

        /// Construct a container by copying `x`, and using `allocator` for this container's
        /// allocations.
        ///
        /// \param x The container to copy from.
        /// \param allocator The allocator to use for any allocations.
        RBTreeContainerBase(const RBTreeContainerBase& x, Allocator& allocator) noexcept;

        /// Construct a container by moving `x`, and using `allocator` for this container's
        /// allocations.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param x The container to move from.
        /// \param allocator The allocator to use for any allocations.
        RBTreeContainerBase(RBTreeContainerBase&& x, Allocator& allocator) noexcept;

        /// Construct a container by copying `x`, using the allocator from `x`.
        ///
        /// \param x The container to copy from.
        RBTreeContainerBase(const RBTreeContainerBase& x) noexcept;

        /// Construct a container by moving `x`, using the allocator from `x`.
        ///
        /// \param x The container to move from.
        RBTreeContainerBase(RBTreeContainerBase&& x) noexcept;

        /// Destructs the container, freeing any memory allocations.
        ~RBTreeContainerBase() noexcept;

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Copy the container `x` into this container.
        ///
        /// \param x The container to copy from.
        RBTreeContainerBase& operator=(const RBTreeContainerBase& x) noexcept;

        /// Move the container `x` into this container.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param x The container to move from.
        RBTreeContainerBase& operator=(RBTreeContainerBase&& x) noexcept;

        /// Checks if this container is equal to another container.
        ///
        /// \param x The container to check against.
        /// \return True if the containers are equal, false otherwise.
        [[nodiscard]] bool operator==(const RBTreeContainerBase& x) const;

        /// Checks if this container is not equal to another container.
        ///
        /// \param x The container to check against.
        /// \return True if the containers are not equal, false otherwise.
        [[nodiscard]] bool operator!=(const RBTreeContainerBase& x) const { return !this->operator==(x); }

        // ----------------------------------------------------------------------------------------
        // Capacity

        /// Checks if this container is empty.
        ///
        /// \return Returns true if this is an empty container.
        [[nodiscard]] bool IsEmpty() const { return m_tree.IsEmpty(); }

        /// The length of the container that is currently stored.
        ///
        /// \return Number of entries in the container.
        [[nodiscard]] uint32_t Size() const { return m_size; }

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Checks if a key is contained within the container.
        ///
        /// \return Returns true if the key was found, or false otherwise.
        template <typename U>
        [[nodiscard]] bool Contains(const U& key) const { return Find(key) != nullptr; }

        /// Search for an entry by key.
        ///
        /// \note The returned pointer is invalided if a rehash occurs.
        ///
        /// \return A pointer to the found entry, or nullptr if no entry was found.
        template <typename U>
        [[nodiscard]] const EntryType* Find(const U& key) const;

        /// \copydoc Find
        template <typename U>
        [[nodiscard]] EntryType* Find(const U& key) { return const_cast<EntryType*>(const_cast<const RBTreeContainerBase*>(this)->Find(key)); }

        /// Search for an entry by key.
        ///
        /// \note The returned pointer is invalided if a rehash occurs.
        ///
        /// \return A pointer to the found entry, or nullptr if no entry was found.
        template <typename U>
        [[nodiscard]] const EntryType& Get(const U& key) const;

        /// \copydoc Get
        template <typename U>
        [[nodiscard]] EntryType& Get(const U& key) { return const_cast<EntryType&>(const_cast<const RBTreeContainerBase*>(this)->Get(key)); }

        /// Returns a reference to the allocator object used by the container.
        ///
        /// \return The allocator object this container uses.
        [[nodiscard]] Allocator& GetAllocator() const { return m_allocator; }

        // ----------------------------------------------------------------------------------------
        // Iterators

        /// Gets a pointer to the first element in the container.
        ///
        /// \return A pointer to the first element.
        [[nodiscard]] ConstIterator Begin() const { return ConstIterator(m_tree); }

        /// \copydoc Begin()
        [[nodiscard]] Iterator Begin() { return Iterator(m_tree); }

        /// Gets a pointer to one past the last element in the container.
        ///
        /// \return A pointer to one past the last element.
        [[nodiscard]] ConstIterator End() const { return ConstIterator(); }

        /// \copydoc End()
        [[nodiscard]] Iterator End() { return Iterator(); }

        /// \copydoc Begin()
        [[nodiscard]] ConstIterator begin() const { return ConstIterator(m_tree); }

        /// \copydoc Begin()
        [[nodiscard]] Iterator begin() { return Iterator(m_tree); }

        /// \copydoc End()
        [[nodiscard]] ConstIterator end() const { return ConstIterator(); }

        /// \copydoc End()
        [[nodiscard]] Iterator end() { return Iterator(); }

        // ----------------------------------------------------------------------------------------
        // Mutators

        /// Sets the size of the container to zero, and destructs elements as necessary.
        /// Does not affect memory allocation.
        void Clear();

        /// Erase an entry from the container using the key of that entry.
        ///
        /// \param[in] key The of the entry to erase.
        /// \return True if an entry was erased, false otherwise.
        template <typename U>
        bool Erase(const U& key);

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
        template <typename U, typename... Args>
        EmplaceResult Emplace(U&& key, Args&&... args);

    private:
        void CopyFrom(const RBTreeContainerBase& x);
        void MoveFrom(RBTreeContainerBase&& x);

    private:
        Allocator& m_allocator;
        TreeType m_tree{};
        uint32_t m_size{ 0 };
    };

    // --------------------------------------------------------------------------------------------
    /// Traits used by the \ref RBTreeContainerBase to implement an \ref RBTreeMap
    ///
    /// \internal
    template <typename K, typename V>
    struct RBTreeMapTraits
    {
        struct Node final
        {
            template <typename U>
            explicit Node(U&& k) : key(Forward<U>(k)), value() {}

            template <typename U, typename... Args>
            explicit Node(U&& k, Args&&... args) : key(Forward<U>(k)), value(Forward<Args>(args)...) {}

            K key;
            V value;

        private:
            friend RBTreeMapTraits;
            RBTreeLink<Node> link;
        };

        using KeyType = K;
        using EntryType = Node;
        using TreeType = RBTree<Node, &Node::link, KeyType, &Node::key>;
    };

    /// An associative map that maintains strong ordering of entries by using a left-leaning
    /// red-black tree for lookups. Keys are expected to be well-ordered, and support comparisons
    /// with `==` and `<`. Uniqueness of keys is determined by comparing them with `==`.
    ///
    /// \note Entries are guaranteed to be ordered by their key during iteration.
    ///
    /// \note Pointers to entries are valid for the lifetime of that entry, even if the container
    /// is modified. This is because entries are individually allocated.
    ///
    /// \tparam K The type of keys in this map.
    /// \tparam V The type of values in the map.
    template <typename K, typename V>
    class RBTreeMap final : public RBTreeContainerBase<RBTreeMapTraits<K, V>>
    {
    public:
        using Super = RBTreeContainerBase<RBTreeMapTraits<K, V>>;
        using Traits = typename Super::Traits;
        using KeyType = typename Super::KeyType;
        using EntryType = typename Super::EntryType;
        using EmplaceResult = typename Super::EmplaceResult;

        using ValueType = V;

    public:
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

    // --------------------------------------------------------------------------------------------
    /// Traits used by the \ref RBTreeContainerBase to implement an \ref RBTreeMap
    ///
    /// \internal
    template <typename T>
    struct RBTreeSetTraits
    {
        struct Node final
        {
            template <typename U>
            explicit Node(U&& v) : value(Forward<U>(v)) {}

            T value;

        private:
            friend RBTreeSetTraits;
            RBTreeLink<Node> link;
        };

        using KeyType = T;
        using EntryType = Node;
        using TreeType = RBTree<Node, &Node::link, KeyType, &Node::value>;
    };

    /// An set of unique entries that maintains strong ordering of entries by using a left-leaning
    /// red-black tree for lookups. Values are expected to be well-ordered, and support comparisons
    /// with `==` and `<`. Uniqueness is determined by comparing values with `==`.
    ///
    /// \note Entries are guaranteed to be ordered during iteration.
    ///
    /// \note Pointers to entries are valid for the lifetime of that entry, even if the container
    /// is modified. This is because entries are individually allocated.
    ///
    /// \tparam T The type data to store in the set.
    template <typename T>
    class RBTreeSet final : public RBTreeContainerBase<RBTreeSetTraits<T>>
    {
    public:
        using Super = RBTreeContainerBase<RBTreeSetTraits<T>>;
        using Traits = typename Super::Traits;
        using KeyType = typename Super::KeyType;
        using EntryType = typename Super::EntryType;
        using InsertResult = typename Super::EmplaceResult;

        using ValueType = T;

    public:
        using Super::Super;

        InsertResult Insert(KeyType&& key) { return Super::Emplace(Move(key)); }
        InsertResult Insert(const KeyType& key) { return Super::Emplace(key); }

    private:
        using Super::Emplace;
    };
}

#include "he/core/inline/rb_tree.inl"
