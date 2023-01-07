// Copyright Chad Engler

#pragma once

#include "editor_data.h"

#include "he/core/allocator.h"
#include "he/core/compiler.h"
#include "he/core/type_traits.h"
#include "he/core/utils.h"
#include "he/core/unique_ptr.h"

#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 15
#include "boost/di.hpp"
#include "boost/di/extension/injector.hpp"

#include <type_traits>

namespace di = boost::di;

namespace he::editor
{
    template <typename ScopeType, typename T>
    struct DIUniqueWrapper
    {
        using scope = ScopeType;

        template <typename I> requires(std::is_convertible_v<T, I>)
        inline operator I() const noexcept { return object; }

        inline operator T&&() noexcept { return static_cast<T&&>(object); }

        T object;
    };

    template <typename ScopeType, typename T>
    struct DIUniqueWrapper<ScopeType, T*>
    {
        using scope = ScopeType;

    #if HE_COMPILER_MSVC
        explicit DIUniqueWrapper(T* object) : object(object) {}
    #endif

        template <typename I> requires(std::is_convertible_v<T, I>)
        inline operator I() const noexcept
        {
            UniquePtr<T> ptr{ object };
            return static_cast<T&&>(*ptr);
        }

        template <typename I> requires(std::is_convertible_v<T*, I*>)
        inline operator I*() const noexcept { return object; }

        template <typename I> requires(std::is_convertible_v<T*, const I*>)
        inline operator const I*() const noexcept { return object; }

        template <typename I> requires(std::is_convertible_v<T*, I*>)
        inline operator UniquePtr<I>() const noexcept { return UniquePtr<I>{ object }; }

        T* object{ nullptr };
    };

    class DIUniqueScope
    {
    public:
        template <typename, typename>
        class scope
        {
        public:
            template <typename...>
            using is_referable = std::false_type;

        #define HE_DI_PROVIDER_VAL(T, ProviderType) \
            std::declval<ProviderType>().get(typename ProviderType::config::template memory_traits<T>::type{})

            template <typename T, typename ProviderType>
            using ProviderGetType = decltype(HE_DI_PROVIDER_VAL(T, ProviderType));

            template <typename T, typename ProviderType>
            using WrapperType = decltype(DIUniqueWrapper<DIUniqueScope, ProviderGetType<T, ProviderType>>{ HE_DI_PROVIDER_VAL(T, ProviderType) });

            template <typename T, typename, typename ProviderType>
            static WrapperType<T, ProviderType> try_create(const ProviderType&);

            template <typename T, typename, typename ProviderType>
            auto create(const ProviderType& provider) const
            {
                using memory = typename ProviderType::config::template memory_traits<T>::type;
                using wrapper = DIUniqueWrapper<DIUniqueScope, decltype(provider.get(memory{}))>;
                return wrapper{ provider.get(memory{}) };
            }

        #undef HE_DI_PROVIDER_VAL
        };
    };

    class DIProvider
    {
    public:
        template <typename TInitialization, typename T, typename... TArgs>
        struct is_creatable
        {
            static constexpr auto value = di::concepts::creatable<TInitialization, T, TArgs...>::value;
        };

        template <typename T, typename... TArgs>
        auto get(const di::type_traits::direct&, const di::type_traits::heap&, TArgs&&... args) const
        {
            void* p = Allocator::GetDefault().Malloc<T>(1);
            return new(p) T(Forward<TArgs>(args)...);
        }

        template <typename T, typename... TArgs>
        auto get(const di::type_traits::uniform&, const di::type_traits::heap&, TArgs&&... args) const
        {
            void* p = Allocator::GetDefault().Malloc<T>(1);
            return new(p) T{ Forward<TArgs>(args)... };
        }

        template <typename T, typename... TArgs>
        auto get(const di::type_traits::direct&, const di::type_traits::stack&, TArgs&&... args) const
        {
            return T(Forward<TArgs>(args)...);
        }

        template <typename T, typename... TArgs>
        auto get(const di::type_traits::uniform&, const di::type_traits::stack&, TArgs&&... args) const
        {
            return T{ Forward<TArgs>(args)... };
        }
    };

    struct DIConfig : di::config
    {
        template <class T> struct scope_traits { using type = DIUniqueScope; };
        template <class T> struct scope_traits<T&> { using type = typename di::scopes::singleton; };

        template <class T> struct memory_traits { using type = typename di::type_traits::stack; };
        template <class T> struct memory_traits<T*> { using type = typename di::type_traits::heap; };
        template <class T> struct memory_traits<const T&> { using type = typename memory_traits<T>::type; };
        template <class T> struct memory_traits<UniquePtr<T>> { using type = typename di::type_traits::heap; };

        template <class T> requires(std::is_polymorphic_v<T>)
        struct memory_traits<T> { using type = typename di::type_traits::heap; };

        template <typename InjectorType>
        static auto provider(const InjectorType&) { return DIProvider{}; }
    };

    inline const auto g_appInjector = di::make_injector<DIConfig>();

    template <typename T>
    auto DICreate() -> decltype(g_appInjector.template create<T>())
    {
        return g_appInjector.template create<T>();
    }

    template <typename T>
    UniquePtr<T> DICreateUnique()
    {
        return DICreate<UniquePtr<T>>();
    }
}

BOOST_DI_NAMESPACE_BEGIN
namespace aux
{
    template <class T>
    struct remove_smart_ptr<he::UniquePtr<T>>
    {
        using type = T;
    };

    template <class T>
    struct deref_type<he::UniquePtr<T>>
    {
        using type = remove_qualifiers_t<typename deref_type<T>::type>;
    };
}

namespace type_traits
{
    template <class T, class U>
    struct rebind_traits<he::UniquePtr<T>, U>
    {
        using type = he::UniquePtr<U>;
    };

    template <class T, class TName, class _>
    struct rebind_traits<he::UniquePtr<T>, named<TName, _>>
    {
        using type = named<TName, he::UniquePtr<T>>;
    };
}
BOOST_DI_NAMESPACE_END
