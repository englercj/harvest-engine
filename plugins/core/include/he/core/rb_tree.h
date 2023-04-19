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
#include "he/core/types.h"
#include "he/core/utils.h"

namespace std { struct bidirectional_iterator_tag; }

namespace he
{
    // --------------------------------------------------------------------------------------------
    template <typename T>
    struct RBTreeLink
    {
        T* left;
        T* rightRed;
    };

    // --------------------------------------------------------------------------------------------
    template <typename T, RBTreeLink<T> T::*Link, typename K, K T::*Key>
    class RBTree final
    {
    public:
        using Pfn_ClearHandler = bool(*)(T*, void*);

        static constexpr uint32_t MaxDepth = sizeof(void*) << 4;

        static T* Next(T* node);
        static T* Prev(T* node);

    public:
        RBTree() = default;
        RBTree(const RBTree&) = delete;
        RBTree(RBTree&& x) : m_root(Exchange(x.m_root, nullptr)) {}

        RBTree& operator=(const RBTree&) = delete;
        RBTree& operator=(RBTree&& x);

        bool IsEmpty() { return m_root == nullptr; }
        T* First() const { return First(m_root); }
        T* Last() const { return Last(m_root); }

        void Clear(Pfn_ClearHandler handler = nullptr, void* userData = nullptr);
        T* Find(const K& key) const;

        void Insert(T* node);
        T* Remove(const K& key);

    private:
        void ClearInternal(T* node, Pfn_ClearHandler handler, void* userData);

    private:
        struct PathEntry
        {
            T* node;
            int cmp;
        };

        static constexpr uintptr_t RedFlag = 1;
        static constexpr uintptr_t RedMask = ~RedFlag;

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
    template <typename K, typename V>
    class RBTreeMap final
    {
    public:
        using EntryType = struct Node;
        using KeyType = K;
        using ValueType = V;
        using TreeType = RBTree<Node, &Node::link, KeyType, &Node::key>;

    public:
        struct Node final
        {
            template <typename U>
            explicit Node(U&& k) : key(Forward<U>(k)), value() {}

            template <typename U, typename... Args>
            explicit Node(U&& k, Args&&... args) : key(Forward<U>(k)), value(Forward<Args>(args)...) {}

            K key;
            V value;

        private:
            RBTreeLink<Node> link;
        };

        struct EmplaceResult final
        {
            /// A reference to the newly created, or previously existing, entry.
            Node& entry;

            /// True if `entry` refers to a newly constructed entry, and false if `entry` points
            /// to an entry that already existed.
            bool inserted;
        };

        class Iterator final
        {
        public:
            using ElementType = Node;

            using difference_type = uint32_t;
            using value_type = ElementType;
            using container_type = RBTreeMap;
            using iterator_category = std::bidirectional_iterator_tag;
            using _Unchecked_type = Iterator; // Mark iterator as checked.

        public:
            Iterator() = default;
            Iterator(const TreeType& tree) noexcept : m_tree(&tree), m_node(tree.First()) {}

            Node& operator*() const { return *m_node; }
            Node* operator->() const { return m_node; }

            Iterator& operator++() { m_node = m_tree->Next(m_node); return *this; }
            Iterator operator++(int) { Iterator x(*this); m_node = m_tree->Next(m_node); return x; }

            Iterator& operator--() { m_node = m_tree->Prev(m_node); return *this; }
            Iterator operator--(int) { Iterator x(*this); m_node = m_tree->Prev(m_node); return x; }

            [[nodiscard]] Iterator operator+(uint32_t n) const
            {
                Iterator x(*this);
                for (uint32_t i = 0; i < n; ++i)
                    x.m_node = x.m_tree->Next(x.m_node);
                return x;
            }

            [[nodiscard]] Iterator operator-(uint32_t n) const
            {
                Iterator x(*this);
                for (uint32_t i = 0; i < n; ++i)
                    x.m_node = x.m_tree->Prev(x.m_node);
                return x;
            }

            [[nodiscard]] bool operator==(const Iterator& x) const { return m_node == x.m_node; }
            [[nodiscard]] bool operator!=(const Iterator& x) const { return m_node != x.m_node; }

            [[nodiscard]] explicit operator bool() const { return m_node != nullptr; }

        private:
            const TreeType* m_tree{ nullptr };
            Node* m_node{ nullptr };
        };

    public:
        // ----------------------------------------------------------------------------------------
        // Construction

        /// Construct an empty map.
        ///
        /// \param allocator Optional. The allocator to use.
        explicit RBTreeMap(Allocator& allocator = Allocator::GetDefault()) noexcept;

        /// Construct a table by copying `x`, and using `allocator` for this table's allocations.
        ///
        /// \param x The table to copy from.
        /// \param allocator The allocator to use for any allocations.
        RBTreeMap(const RBTreeMap& x, Allocator& allocator) noexcept;

        /// Construct a table by moving `x`, and using `allocator` for this table's allocations.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param x The table to move from.
        /// \param allocator The allocator to use for any allocations.
        RBTreeMap(RBTreeMap&& x, Allocator& allocator) noexcept;

        /// Construct a table by copying `x`, using the allocator from `x`.
        ///
        /// \param x The table to copy from.
        RBTreeMap(const RBTreeMap& x) noexcept;

        /// Construct a table by moving `x`, using the allocator from `x`.
        ///
        /// \param x The table to move from.
        RBTreeMap(RBTreeMap&& x) noexcept;

        /// Destructs the table, freeing any memory allocations.
        ~RBTreeMap() noexcept;

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Copy the table `x` into this table.
        ///
        /// \param x The table to copy from.
        RBTreeMap& operator=(const RBTreeMap& x) noexcept;

        /// Move the table `x` into this table.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param x The table to move from.
        RBTreeMap& operator=(RBTreeMap&& x) noexcept;

        /// Checks if this table is equal to another table.
        ///
        /// \param x The table to check against.
        /// \return True if the tables are equal, false otherwise.
        [[nodiscard]] bool operator==(const RBTreeMap& x) const;

        /// Checks if this table is not equal to another table.
        ///
        /// \param x The table to check against.
        /// \return True if the tables are not equal, false otherwise.
        [[nodiscard]] bool operator!=(const RBTreeMap& x) const { return !this->operator==(x); }

        // ----------------------------------------------------------------------------------------
        // Capacity

        /// Checks if this table is empty.
        ///
        /// \return Returns true if this is an empty table.
        [[nodiscard]] bool IsEmpty() const { return m_tree.IsEmpty(); }

        /// The length of the table that is currently stored.
        ///
        /// \return Number of entries in the table.
        [[nodiscard]] uint32_t Size() const { return m_size; }

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Checks if a key is contained within the table.
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
        [[nodiscard]] const ValueType* Find(const U& key) const;

        /// \copydoc Find
        template <typename U>
        [[nodiscard]] ValueType* Find(const U& key) { return const_cast<ValueType*>(const_cast<const RBTreeMap*>(this)->Find(key)); }

        /// Search for an entry by key.
        ///
        /// \note The returned pointer is invalided if a rehash occurs.
        ///
        /// \return A pointer to the found entry, or nullptr if no entry was found.
        template <typename U>
        [[nodiscard]] const ValueType& Get(const U& key) const;

        /// \copydoc Get
        template <typename U>
        [[nodiscard]] ValueType& Get(const U& key) { return const_cast<ValueType&>(const_cast<const RBTreeMap*>(this)->Get(key)); }

        /// Returns a reference to the allocator object used by the table.
        ///
        /// \return The allocator object this table uses.
        [[nodiscard]] Allocator& GetAllocator() const { return m_allocator; }

        // ----------------------------------------------------------------------------------------
        // Iterators

        /// Gets a pointer to the first element in the table.
        ///
        /// \return A pointer to the first element.
        [[nodiscard]] const Iterator* Begin() const { return Iterator(m_tree); }

        /// \copydoc Begin()
        [[nodiscard]] Iterator* Begin() { return Iterator(m_tree); }

        /// Gets a pointer to one past the last element in the table.
        ///
        /// \return A pointer to one past the last element.
        [[nodiscard]] const Iterator* End() const { return Iterator(); }

        /// \copydoc End()
        [[nodiscard]] Iterator* End() { return Iterator(); }

        /// \copydoc Begin()
        [[nodiscard]] const Iterator* begin() const { return Iterator(m_tree); }

        /// \copydoc Begin()
        [[nodiscard]] Iterator* begin() { return Iterator(m_tree); }

        /// \copydoc End()
        [[nodiscard]] const Iterator* end() const { return Iterator(); }

        /// \copydoc End()
        [[nodiscard]] Iterator* end() { return Iterator(); }

        // ----------------------------------------------------------------------------------------
        // Mutators

        /// Sets the size of the table to zero, and destructs elements as necessary.
        /// Does not affect memory allocation.
        void Clear();

        /// Erase an entry from the table using the key of that entry.
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
        void CopyFrom(const RBTreeMap& x);

    private:
        Allocator& m_allocator;
        TreeType m_tree{};
        uint32_t m_size{ 0 };
    };
}

#include "he/core/inline/rb_tree.inl"
