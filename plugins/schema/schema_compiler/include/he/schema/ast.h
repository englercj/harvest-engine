// Copyright Chad Engler

#pragma once

#include "he/core/memory_ops.h"
#include "he/core/string_view.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/schema/types.h"

namespace he::schema
{
    struct AstNode;

    struct AstListLink
    {
        void* next{ nullptr };
        void* prev{ nullptr };
    };

    template <typename T>
    class AstList;

    template <typename T>
    class AstListIterator
    {
    public:
        using ListType = AstList<T>;
        using ElementType = T;

        using difference_type = uint32_t;
        using value_type = ElementType;
        using container_type = ListType;
        using _Unchecked_type = AstListIterator; // Mark iterator as checked.

    public:
        AstListIterator() = default;
        AstListIterator(const ListType* list, T* node) : m_list(list), m_node(node) {}

        T& operator*() const { return *m_node; }
        T* operator->() const { return m_node; }

        AstListIterator& operator++() { m_node = m_list->Next(m_node); return *this; }
        AstListIterator operator++(int) { AstListIterator x(m_list, m_node); m_node = m_list->Next(m_node); return x; }
        AstListIterator& operator--() { m_node = m_list->Previous(m_node); return *this; }
        AstListIterator operator--(int) { AstListIterator x(m_list, m_node); m_node = m_list->Previous(m_node); return x; }

        bool operator==(const AstListIterator& x) const { return m_list == x.m_list && m_node == x.m_node; }
        bool operator!=(const AstListIterator& x) const { return m_list != x.m_list || m_node != x.m_node; }

    private:
        const AstList<T>* m_list{ nullptr };
        T* m_node{ nullptr };
    };

    template <typename T>
    class AstList
    {
    public:
        using ElementType = T;

    public:
        AstList() = default;

        bool IsEmpty() const { return m_size == 0; }
        void Clear() { m_head = m_tail = nullptr; m_size = 0; }

        uint32_t Size() const { return m_size; }

        T* Front() const { return m_head; }
        T* Back() const { return m_tail; }
        T* Next(const T* node) const { return node ? static_cast<T*>(node->link.next) : nullptr; }
        T* Previous(const T* node) const { return node ? static_cast<T*>(node->link.prev) : nullptr; }

        AstListIterator<T> begin() const { return AstListIterator<T>(this, m_head); }
        AstListIterator<T> end() const { return AstListIterator<T>(this, nullptr); }

        void PushBack(T* node)
        {
            if (IsEmpty())
            {
                m_head = node;
                m_tail = node;
            }
            else
            {
                node->link.prev = m_tail;
                m_tail->link.next = node;
                m_tail = node;
            }
            ++m_size;
        }

        void PushFront(T* node)
        {
            if (IsEmpty())
            {
                m_head = node;
                m_tail = node;
            }
            else
            {
                node->link.next = m_head;
                m_head->link.prev = node;
                m_head = node;
            }
            ++m_size;
        }

        void Remove(T* node)
        {
            T* prev = static_cast<T*>(node->link.prev);
            T* next = static_cast<T*>(node->link.next);

            if (prev)
                prev->link.next = next;
            else
                m_head = next;

            if (next)
                next->link.prev = prev;
            else
                m_tail = prev;

            --m_size;
        }

        template <typename F>
        T* Find(F&& itr) const
        {
            for (T& node : *this)
            {
                if (itr(node))
                    return &node;
            }
            return nullptr;
        }

    private:
        T* m_head{ nullptr };
        T* m_tail{ nullptr };
        uint32_t m_size{ 0 };
    };

    struct AstFileLocation
    {
        uint32_t line{ 0 };
        uint32_t column{ 0 };
    };

    struct AstExpression
    {
        // All allocations are within the linear allocator, so no need to manage union destruction
        AstExpression() { MemZero(this, sizeof(*this)); }
        ~AstExpression() {}

        enum class Kind
        {
            Unknown,

            Array,          ///< Fixed length array type
            Blob,           ///< Blob value (0x"ab cd ef")
            Float,          ///< Floating-point value
            Generic,        ///< Generic parameter name
            List,           ///< Dynamic length list type
            Identifier,     ///< Any identifier
            Namespace,      ///< Namespace
            Sequence,       ///< List or array value ([ 1, 2, 3 ])
            SignedInt,      ///< Signed integer value
            String,         ///< String value ("string")
            Tuple,          ///< Tuple value ({ x = 1, y = 2 })
            UnsignedInt,    ///< Unsigned integer value
            QualifiedName,  ///< Identifiers/Generics separated by dots (A.B<T>.C)
        };

        Kind kind;
        AstFileLocation location;

        union
        {
            StringView blob;
            double floatingPoint;
            StringView identifier;
            StringView namespaceName;
            AstList<AstExpression> sequence;
            int64_t signedInt;
            StringView string;
            AstList<struct AstTupleParam> tuple;
            uint64_t unsignedInt;

            struct
            {
                AstExpression* elementType;
                AstExpression* size;
            } array;

            struct
            {
                StringView name;
                AstList<AstExpression> params;
            } generic;

            struct
            {
                AstExpression* elementType;
            } list;

            struct
            {
                AstList<AstExpression> names;
            } qualified;
        };

        AstListLink link;
    };

    struct AstAttribute
    {
        AstExpression name{};
        AstExpression value{};
        AstFileLocation location{};
        AstListLink link{};
    };

    struct AstTypeParam
    {
        StringView name{};
        AstFileLocation location{};
        AstListLink link{};
    };

    struct AstTupleParam
    {
        StringView name{};
        AstExpression value{};
        AstFileLocation location{};
        AstListLink link{};
    };

    struct AstMethodParams
    {
        // All allocations are within the linear allocator, so no need to manage union destruction
        AstMethodParams() { MemZero(this, sizeof(*this)); }
        ~AstMethodParams() {}

        enum class Kind
        {
            Type,
            Fields,
        };

        Kind kind;
        bool stream;

        union
        {
            AstExpression type;
            AstList<AstNode> fields;
        };
    };

    struct AstNode
    {
        // All allocations are within the linear allocator, so no need to manage union destruction
        AstNode() { MemZero(this, sizeof(*this)); }
        ~AstNode() {}

        enum class Kind
        {
            Alias,
            Attribute,
            Constant,
            Enum,
            Enumerator,
            Field,
            File,
            Group,
            Interface,
            Method,
            Struct,
            Union,
        };

        Kind kind;
        StringView name;
        StringView docComment;
        AstFileLocation location;

        TypeId id;
        AstNode* parent;

        AstList<AstTypeParam> typeParams;
        AstList<AstAttribute> attributes;
        AstList<AstNode> children;

        union
        {
            struct
            {
                AstExpression target;
            } alias;

            struct
            {
                AstExpression type;

                bool targetsAttribute;
                bool targetsConstant;
                bool targetsEnum;
                bool targetsEnumerator;
                bool targetsField;
                bool targetsFile;
                bool targetsInterface;
                bool targetsMethod;
                bool targetsParameter;
                bool targetsStruct;
            } attribute;

            struct
            {
                AstExpression type;
                AstExpression value;
            } constant;

            struct
            {
                AstExpression type;
                AstExpression defaultValue;
            } field;

            struct
            {
                AstExpression nameSpace;
                AstList<AstExpression> imports;
            } file;

            struct
            {
                AstExpression super;
            } interface;

            struct
            {
                AstMethodParams params;
                AstMethodParams results;
            } method;
        };

        AstListLink link;
    };

    struct AstFile
    {
        AstFile() : root(), allocator(sizeof(AstNode) * 128) {}

        AstNode root;
        LinearPageAllocator allocator;
    };
}
