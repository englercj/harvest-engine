// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/compiler.h"
#include "he/core/hash_table.h"
#include "he/core/sync.h"
#include "he/core/type_info.h"
#include "he/core/type_traits.h"
#include "he/core/utils.h"
#include "he/core/unique_ptr.h"
#include "he/editor/editor_data.h"

#define BOOST_DI_CFG_CTOR_LIMIT_SIZE 15
#include "boost/di.hpp"
#include "boost/di/extension/injector.hpp"

#include <memory>

namespace di = boost::di;

namespace he::editor
{
    template <typename ScopeType, typename T>
    struct DIUniqueWrapper
    {
        using scope = ScopeType;

        template <typename I> requires(IsConvertible<T, I>)
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

        template <typename I> requires(IsConvertible<T, I>)
        inline operator I() const noexcept
        {
            UniquePtr<T> ptr{ object };
            return static_cast<T&&>(*ptr);
        }

        template <typename I> requires(IsConvertible<T*, I*>)
        inline operator I*() const noexcept { return object; }

        template <typename I> requires(IsConvertible<T*, const I*>)
        inline operator const I*() const noexcept { return object; }

        template <typename I> requires(IsConvertible<T*, I*>)
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

        #define HE_DI_PROVIDER_VAL \
            DeclVal<ProviderType>().get(typename ProviderType::config::template memory_traits<T>::type{})

            template <typename T, typename ProviderType>
            using ProviderGetType = decltype(HE_DI_PROVIDER_VAL);

            template <typename T, typename ProviderType>
            using WrapperType = decltype(DIUniqueWrapper<DIUniqueScope, ProviderGetType<T, ProviderType>>{ HE_DI_PROVIDER_VAL });

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

    class DISharedScope
    {
    public:
        template <typename, typename T>
        class scope
        {
        public:
            using WrapperType = di::wrappers::shared<DISharedScope, T>;

            scope() noexcept = default;

        #if !defined(BOOST_DI_NOT_THREAD_SAFE)
            //<<lock mutex so that move will be synchronized>>
            explicit scope(scope&& x) noexcept : scope(Move(x), LockGuard(x.m_mutex)) {}
            //<<synchronized move constructor>>
            scope(scope&& x, const LockGuard<Mutex>&) noexcept : m_object(Move(x.m_object)) {}
        #endif

            template <typename U, typename>
            using is_referable = typename WrapperType::template is_referable<U>;

            template <typename, typename, typename TProvider>
            static auto try_create(const TProvider& provider)
                -> decltype(WrapperType{ std::shared_ptr<T>{ provider.get() } });

            /**
             * `in(shared)` version
             */
            template <typename, typename, typename TProvider>
            WrapperType create(const TProvider& provider) &
            {
                if (!m_object)
                {
                #if !defined(BOOST_DI_NOT_THREAD_SAFE)
                    LockGuard lock(m_mutex);
                    if (!m_object)
                #endif
                        m_object = std::shared_ptr<T>{ provider.get() };
                }
                return WrapperType{ m_object };
            }

            /**
             * Deduce scope version
             */
            template <typename, typename, typename TProvider>
            WrapperType create(const TProvider& provider) &&
            {
            #if !defined(BOOST_DI_NOT_THREAD_SAFE)
                LockGuard lock(m_mutex);
            #endif
                auto* object = provider.cfg().template Find<T>();
                if (object && *object)
                    return WrapperType{ std::static_pointer_cast<T>(*object) };

                std::shared_ptr<T> value{ provider.get() };
                provider.cfg().template Store<T>(value);
                return WrapperType{ std::static_pointer_cast<T>(value) };
            }

        private:
        #if !defined(BOOST_DI_NOT_THREAD_SAFE)
            Mutex m_mutex;
        #endif
            std::shared_ptr<T> m_object;
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

    class DIConfig
    {
    public:
        DIConfig() = default;
        DIConfig(const DIConfig&) = delete;
        DIConfig& operator=(const DIConfig&) = delete;

        DIConfig(DIConfig&& x) noexcept : m_data(Move(x.m_data)) {}
        DIConfig& operator=(DIConfig&& x) noexcept { m_data = Move(x.m_data); return *this; }

        ~DIConfig() noexcept
        {
            // destruct in reverse
            const auto entries = m_data.Entries();
            for (uint32_t i = entries.Size() - 1; i != static_cast<uint32_t>(-1); --i)
            {
                entries[i].value.reset();
            }
            m_data.Clear();
        }

        template <typename T>
        auto* Find()
        {
            constexpr TypeInfo Info = TypeInfo::Get<T>();
            return m_data.Find(Info);
        }

        template <typename T>
        void Store(std::shared_ptr<T> v)
        {
            constexpr TypeInfo Info = TypeInfo::Get<T>();
            m_data.EmplaceOrAssign(Info, v);
        }

        template <typename T>
        auto provider(T*) noexcept { return DIProvider{}; }

        template <typename T>
        auto policies(T*) noexcept { return di::make_policies(); }

        template <typename T> struct scope_traits { using type = DIUniqueScope; };
        template <typename T> struct scope_traits<T&> { using type = DISharedScope; };

        template <typename T> struct memory_traits { using type = typename di::type_traits::stack; };
        template <typename T> struct memory_traits<T*> { using type = typename di::type_traits::heap; };
        template <typename T> struct memory_traits<const T&> { using type = typename memory_traits<T>::type; };
        template <typename T> struct memory_traits<UniquePtr<T>> { using type = typename di::type_traits::heap; };

        template <typename T> requires(IsPolymorphic<T>)
        struct memory_traits<T> { using type = typename di::type_traits::heap; };

    private:
        HashMap<TypeInfo, std::shared_ptr<void>> m_data{};
    };

    #define HE_DI_MAKE_INJECTOR() di::make_injector<::he::editor::DIConfig>()

    using DIInjectorType = decltype(HE_DI_MAKE_INJECTOR());
    extern const DIInjectorType g_appInjector;

    template <typename T>
    HE_FORCE_INLINE auto DICreate() -> decltype(g_appInjector.template create<T>())
    {
        return g_appInjector.template create<T>();
    }

    template <typename T>
    HE_FORCE_INLINE UniquePtr<T> DICreateUnique()
    {
        return DICreate<UniquePtr<T>>();
    }
}

BOOST_DI_NAMESPACE_BEGIN
namespace aux
{
    template <typename T>
    struct remove_smart_ptr<he::UniquePtr<T>>
    {
        using type = T;
    };

    template <typename T>
    struct deref_type<he::UniquePtr<T>>
    {
        using type = remove_qualifiers_t<typename deref_type<T>::type>;
    };
}

namespace type_traits
{
    template <typename T>
    struct memory_traits<he::UniquePtr<T>>
    {
        using type = heap;
    };

    template <typename T, typename U>
    struct rebind_traits<he::UniquePtr<T>, U>
    {
        using type = he::UniquePtr<U>;
    };

    template <typename T, typename TName, typename _>
    struct rebind_traits<he::UniquePtr<T>, named<TName, _>>
    {
        using type = named<TName, he::UniquePtr<T>>;
    };
}
BOOST_DI_NAMESPACE_END
