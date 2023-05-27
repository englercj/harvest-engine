// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/concepts.h"
#include "he/core/limits.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
    template <typename T> struct Hasher;

    template <typename... T>
    class Variant
    {
        static_assert(sizeof...(T) > 0, "Variants cannot be empty.");
        static_assert((!IsReference<T> && ...), "Variants cannot contain references.");
        static_assert((!IsVoid<T> && ...), "Variants cannot contain void.");
        static_assert((!IsArray<T> && ...), "Variants cannot contain arrays.");

    public:
        static constexpr uint32_t Size = sizeof...(T);

        using ElementList = TypeList<T...>;
        using IndexType = Conditional<Size < Limits<uint8_t>::Max, uint8_t, Conditional<Size < Limits<uint16_t>::Max, uint16_t, uint32_t>>;

        template <uint32_t Index>
        using ElementType = TypeListElement<Index, ElementList>;

        static constexpr IndexType InvalidIndex = Limits<IndexType>::Max;

    public:
        constexpr Variant() noexcept : m_index(InvalidIndex) {}

        template <uint32_t Index, typename... Args>
        constexpr Variant(IndexConstant<Index>, Args&&... args) noexcept
            : m_index(Index)
        {
            Create<Index>(Forward<Args>(args)...);
        }

        constexpr Variant(const Variant& other) noexcept
            : m_index(other.m_index)
        {
            if (IsValid())
            {
                Visit(_CopyConstructVisitor{ other });
            }
        }

        constexpr Variant(Variant&& other) noexcept
            : m_index(other.m_index)
        {
            if (IsValid())
            {
                Visit(_MoveConstructVisitor{ other });
            }
        }

        ~Variant() noexcept
        {
            Clear();
        }

        constexpr Variant& operator=(const Variant& x)
        {
            Clear();
            m_index = x.m_index;
            Visit(_CopyAssignVisitor{ x });
            return *this;
        }

        constexpr Variant& operator=(Variant&& x)
        {
            Clear();
            m_index = x.m_index;
            Visit(_MoveAssignVisitor{ x });
            return *this;
        }

        // TODO: Assignment once we have the type list utils merged that enable it
        // template <AnyOf<T...> U>
        // constexpr Variant& operator=(U&& value)
        // {
        // }

        constexpr bool IsValid() const { return m_index != InvalidIndex; }
        constexpr uint32_t Index() const { return m_index; }

        constexpr void Clear()
        {
            if (IsValid())
            {
                Visit(_DestructVisitor{});
                m_index = InvalidIndex;
            }
        }

        template <uint32_t Index, typename... Args>
        constexpr ElementType<Index>& Emplace(Args&&... args)
        {
            Clear();
            m_index = Index;
            return *Create<Index>(Forward<Args>(args)...);
        }

        template <uint32_t Index>
        [[nodiscard]] constexpr const ElementType<Index>& Get() const&
        {
            static_assert(Index < Size, "Index out of range.");
            HE_ASSERT(m_index == Index);
            return *reinterpret_cast<const ElementType<Index>*>(m_storage.data);
        }

        template <uint32_t Index>
        [[nodiscard]] constexpr ElementType<Index>& Get() &
        {
            return const_cast<ElementType<Index>&>(const_cast<const Variant*>(this)->Get<Index>());
        }

        template <uint32_t Index>
        [[nodiscard]] constexpr ElementType<Index>&& Get() &&
        {
            return Move(const_cast<ElementType<Index>&>(const_cast<const Variant*>(this)->Get<Index>()));
        }

        template <uint32_t Index>
        [[nodiscard]] constexpr const ElementType<Index>* TryGet() const
        {
            static_assert(Index < Size, "Index out of range.");
            return m_index == Index ? reinterpret_cast<const ElementType<Index>*>(m_storage.data) : nullptr;
        }

        template <uint32_t Index>
        [[nodiscard]] constexpr ElementType<Index>* TryGet()
        {
            return const_cast<ElementType<Index>*>(const_cast<const Variant*>(this)->TryGet<Index>());
        }

        template <typename V>
        constexpr decltype(auto) Visit(V&& visitor)
        {
            HE_ASSERT(IsValid());
            return VisitInternal(visitor, MakeIndexSequence<Size>{});
        }

        [[nodiscard]] constexpr bool operator==(const Variant& x) const { return m_index == x.m_index && Visit(_CompareVisitor<EqualTo<>>{ x }); }
        [[nodiscard]] constexpr bool operator!=(const Variant& x) const { return m_index != x.m_index || Visit(_CompareVisitor<NotEqualTo<>>{ x }); }
        [[nodiscard]] constexpr bool operator<(const Variant& x) const { return m_index < x.m_index || (m_index == x.m_index && Visit(_CompareVisitor<LessThan<>>{ x })); }
        [[nodiscard]] constexpr bool operator<=(const Variant& x) const { return m_index < x.m_index || (m_index == x.m_index && Visit(_CompareVisitor<LessThanOrEqual<>>{ x })); }
        [[nodiscard]] constexpr bool operator>(const Variant& x) const { return m_index > x.m_index || (m_index == x.m_index && Visit(_CompareVisitor<GreaterThan<>>{ x })); }
        [[nodiscard]] constexpr bool operator>=(const Variant& x) const { return m_index > x.m_index || (m_index == x.m_index && Visit(_CompareVisitor<GreaterThanOrEqual<>>{ x })); }

        [[nodiscard]] constexpr uint64_t HashCode() const { return IsValid() ? Visit(_HashVisitor{}) : 0; }

    private:
        template <typename V, uint32_t Index>
        constexpr decltype(auto) VisitInternal(V&& visitor, IndexSequence<Index>)
        {
            HE_ASSERT(m_index == Index);
            return visitor(Get<Index>(), IndexConstant<Index>{});
        }

        template <typename V, uint32_t Index, uint32_t... I>
        constexpr decltype(auto) VisitInternal(V&& visitor, IndexSequence<Index, I...>)
        {
            if (m_index == Index)
                return visitor(Get<Index>(), IndexConstant<Index>{});
            return VisitInternal<V, I...>(visitor, IndexSequence<I...>{});
        }

        template <uint32_t Index, typename... Args>
        ElementType<Index>* Create(Args&&... args)
        {
            static_assert(Index < Size, "Index out of range.");
            HE_ASSERT(!IsValid());
            return new (m_storage.data) ElementType<Index>(Forward<Args>(args)...);
        }

    private:
        template <typename Op>
        struct _CompareVisitor
        {
            const Variant& other;

            template <typename T, uint32_t Index>
            [[nodiscard]] constexpr bool operator()(const T& value, IndexConstant<Index>)
            {
                return Op{}(value, other.template Get<Index>());
            }
        };

        struct _HashVisitor
        {
            template <typename T, uint32_t Index>
            [[nodiscard]] constexpr uint64_t operator()(const T& value, IndexConstant<Index>)
            {
                return Hasher<T>{}(value);
            }
        };

        struct _DestructVisitor
        {
            template <typename T, uint32_t Index>
            [[nodiscard]] constexpr void operator()(T& value, IndexConstant<Index>)
            {
                if constexpr (!IsTriviallyDestructible<T>)
                {
                    value.~T();
                }
            }
        };

        struct _CopyConstructVisitor
        {
            const Variant& other;

            template <typename T, uint32_t Index>
            [[nodiscard]] constexpr void operator()(T& value, IndexConstant<Index>)
            {
                new (&value) ElementType<Index>(other.template Get<Index>());
            }
        };

        struct _CopyAssignVisitor
        {
            const Variant& other;

            template <typename T, uint32_t Index>
            [[nodiscard]] constexpr void operator()(T& value, IndexConstant<Index>)
            {
                value = other.template Get<Index>();
            }
        };

        struct _MoveConstructVisitor
        {
            Variant& other;

            template <typename T, uint32_t Index>
            [[nodiscard]] constexpr void operator()(T& value, IndexConstant<Index>)
            {
                new (&value) ElementType<Index>(Move(other).template Get<Index>());
            }
        };

        struct _MoveAssignVisitor
        {
            Variant& other;

            template <typename T, uint32_t Index>
            [[nodiscard]] constexpr void operator()(T& value, IndexConstant<Index>)
            {
                value = Move(other).template Get<Index>();
            }
        };

    private:
        static constexpr uint32_t MaxSize = static_cast<uint32_t>(Max(sizeof(T)...));
        static constexpr uint32_t MaxAlign = static_cast<uint32_t>(Max(alignof(T)...));

        IndexType m_index{ 0 };
        AlignedStorage<MaxSize, MaxAlign> m_storage;
    };
}
