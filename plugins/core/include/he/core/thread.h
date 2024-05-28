// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/clock.h"
#include "he/core/invoke.h"
#include "he/core/result.h"
#include "he/core/tuple.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
    /// A pointer to a function that can be run as a thread procedure.
    using Pfn_ThreadProc = void(*)(void*);

    /// Descriptor of how to create a thread.
    struct ThreadDesc
    {
        /// The thread procedure to run.
        Pfn_ThreadProc proc{ nullptr };

        /// Data to pass to the thread procedure.
        void* data{ nullptr };

        /// Size of the stack allocated for this thread. If zero, the system default is used.
        uint32_t stackSize{ 0 };

        /// CPU affinity for the thread. This is a bitfield where each bit represents a CPU core.
        /// Only considered when non-zero. Errors when setting the affinity of the a newly created
        /// thread are ignored.
        uint64_t affinity{ 0 };
    };

    class Thread
    {
    public:
        /// Gets the ID of the current thread.
        ///
        /// \return The ID of the current thread.
        static uint32_t GetId();

        /// Sets the name of the current thread.
        ///
        /// \param[in] name The name to set for the current thread.
        static void SetName(const char* name);

        /// Sleeps the current thread for a given amount of time.
        ///
        /// \param[in] amount The amount of time to sleep the current thread.
        static void Sleep(Duration amount);

        /// Yields the current thread to the next thread in the system's thread scheduler.
        static void Yield();

        /// Helper to run an invocable on a new thread, similar to `std::thread()`. The invocable
        /// and args are copied onto the heap and passed to the new thread.
        ///
        /// \tparam F The type of the invocable to run on the new thread.
        /// \tparam Args The types of the arguments to pass to the invocable.
        /// \param[in] func The invocable to run on the new thread.
        /// \param[in] args... The arguments to pass to the invocable
        /// \return A thread object for the new thread. On failure, the thread object will not be
        ///     joinable.
        template <typename F, class... Args>
        static Thread Run(F&& func, Args&&... args);

    public:
        /// Construct a thread object. Does not create a new thread automatically like `std::thread`.
        /// You must call \ref Start to create and run a new thread.
        Thread() = default;

        /// Copy constructor is deleted.
        Thread(const Thread&) = delete;

        /// Move construct a thread object.
        Thread(Thread&& x) noexcept;

        /// Destructs the thread object. \ref Detach or \ref Join must be called before the
        /// thread object is destructed.
        ~Thread() noexcept;

        /// Move assignment operator.
        Thread& operator=(Thread&& x) noexcept;

        /// Copy assignment operator is deleted.
        Thread& operator=(const Thread&) = delete;

        /// Gets the native OS handle to the thread.
        ///
        /// \return The handle to the thread.
        void* GetNativeHandle() const { return m_handle; }

        /// Checks if the thread is joinable.
        ///
        /// \return True if the thread is joinable, false otherwise.
        bool IsJoinable() const { return m_handle != nullptr; }

        /// Starts the thread, creating and running a new thread.
        ///
        /// \note If this function returns success, then you must eventually call \ref Join or
        /// \ref Detach to cleanup the thread state.
        ///
        /// \param[in] desc The descriptor for how to create the thread.
        /// \return The result of the operation.
        Result Start(const ThreadDesc& desc);

        /// Joins the thread, waiting for it to finish. This function will assert if the thread is
        /// not joinable.
        ///
        /// \note This object can be reused after calling this function. That is, you can call
        /// \ref Start() again to create and run a new thread.
        ///
        /// \return The result of the operation.
        Result Join();

        /// Detaches the thread, allowing it to run independently. This function will assert if
        /// the thread is not joinable.
        ///
        /// \note This object can be reused after calling this function. That is, you can call
        /// \ref Start() again to create and run a new thread.
        ///
        /// \return The result of the operation.
        Result Detach();

        /// Sets the CPU core affinity of the thread.
        ///
        /// \param[in] mask The affinity mask to set for the thread.
        /// \return The result of the operation.
        Result SetAffinity(uint64_t mask);

    private:
        void* m_handle{ nullptr };
    };

    /// A pointer to a function that is run to destroy thread local storage.
    ///
    /// \param[in] data The data stored in the thread local storage.
    using Pfn_TlsDestructor = void(*)(void*);

    /// A thread local storage value.
    class TlsValue
    {
    public:
        /// Constructs an empty thread local storage value. You must call \ref Create to allocate
        // the thread local storage before calling \ref Get or \ref Set.
        TlsValue() = default;

        /// Copy constructor is deleted.
        TlsValue(const TlsValue&) = delete;

        /// Copy assignment operator is deleted.
        TlsValue& operator=(const TlsValue&) = delete;

        /// Move constructor.
        TlsValue(TlsValue&& x) noexcept : m_id(Exchange(x.m_id, InvalidId)) {}

        /// Move assignment operator.
        TlsValue& operator=(TlsValue&& x) noexcept
        {
            Destroy();
            m_id = Exchange(x.m_id, InvalidId);
            return *this;
        }

        /// Destructs the thread local storage value.
        ~TlsValue() noexcept { Destroy(); }

        /// Create the thread local storage.
        ///
        /// \param[in] destroy Optional. A function called when the thread local storage is destroyed.
        Result Create(Pfn_TlsDestructor destroy = nullptr) noexcept;

        /// Destroy the thread local storage. The object is still valid after calling this
        /// function, but you must call \ref Create again before calling \ref Get or \ref Set.
        void Destroy() noexcept;

        /// Checks if the thread local storage is valid.
        ///
        /// \return True if the thread local storage is valid, false otherwise.
        [[nodiscard]] bool IsValid() const noexcept { return m_id != InvalidId; }

        /// Gets the value stored in the thread local storage.
        ///
        /// \return The value stored in the thread local storage.
        [[nodiscard]] void* Get() const noexcept;

        /// Sets the value stored in the thread local storage.
        ///
        /// \param[in] value The value to store in the thread local storage.
        /// \return The result of the operation.
        void Set(void* value) noexcept;

        /// Sets the value stored in the thread local storage.
        ///
        /// \param[in] value The value to store in the thread local storage.
        /// \return Itself.
        TlsValue& operator=(void* value) noexcept { Set(value); return *this; }

    private:
        static uintptr_t InvalidId;

        uintptr_t m_id{ InvalidId };
    };

    /// \internal
    template <typename T, uint32_t... Indices>
    static void _ThreadInvoker(void* data) noexcept
    {
        T& tup = *static_cast<T*>(data);
        Invoke(Move(TupleGet<Indices>(tup))...);
        Allocator::GetDefault().Delete(&tup);
    }

    /// \internal
    template <typename T, uint32_t... Indices>
    [[nodiscard]] static constexpr auto _GetThreadInvoker(IndexSequence<Indices...>) noexcept
    {
        return &_ThreadInvoker<T, Indices...>;
    }

    template <typename F, class... Args>
    inline Thread Thread::Run(F&& func, Args&&... args)
    {
        using DataTuple = Tuple<Decay<F>, Decay<Args>...>;

        DataTuple* copiedData = Allocator::GetDefault().New<DataTuple>(Forward<F>(func), Forward<Args>(args)...);

        ThreadDesc desc;
        desc.proc = _GetThreadInvoker<DataTuple>(MakeIndexSequence<1 + sizeof...(Args)>{});
        desc.data = copiedData;

        Thread t;
        const Result rc = t.Start(desc);

        if (!rc)
        {
            Allocator::GetDefault().Delete(copiedData);
        }

        return t;
    }
}
