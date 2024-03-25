// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"

// Thread Safety Annotations (TSA) that allow for better static analysis of thread safety.
// These annotations are used to inform the compiler about the intended usage of mutexes and
// other synchronization primitives, in order to catch potential threading bugs at compile time.
//
// For clang, these emit Thread Safety Analysis (TSA) annotations.
// See: https://clang.llvm.org/docs/ThreadSafetyAnalysis.html
//
// For MSVC, these emit SAL annotations.
// See: https://docs.microsoft.com/en-us/cpp/code-quality/annotating-locking-behavior

#if HE_COMPILER_CLANG
    #define HE_TSA_CAPABILITY(x)                    __attribute__((capability(x)))
    #define HE_TSA_GUARDED_BY(x)                    __attribute__((guarded_by(x)))
    #define HE_TSA_WRITE_GUARDED_BY(x)
    #define HE_TSA_PT_GUARDED_BY(x)                 __attribute__((pt_guarded_by(x)))

    #define HE_TSA_ACQUIRED_BEFORE(x)               __attribute__((acquired_before(x)))
    #define HE_TSA_ACQUIRED_AFTER(x)                __attribute__((acquired_after(x)))

    #define HE_TSA_REQUIRES(x)                      __attribute__((requires_capability(x)))
    #define HE_TSA_REQUIRES_SHARED(x)               __attribute__((requires_shared_capability(x)))

    #define HE_TSA_ACQUIRE(x)                       __attribute__((acquire_capability(x)))
    #define HE_TSA_ACQUIRE_SHARED(x)                __attribute__((acquire_shared_capability(x)))
    #define HE_TSA_ACQUIRE_RECURSIVE(x)

    #define HE_TSA_RELEASE(x)                       __attribute__((requires_capability(x))) __attribute__((release_capability(x)))
    #define HE_TSA_RELEASE_SHARED(x)                __attribute__((requires_shared_capability(x))) __attribute__((release_shared_capability(x)))
    #define HE_TSA_RELEASE_RECURSIVE(x)             __attribute__((requires_capability(x)))
    #define HE_TSA_RELEASE_GENERIC(x)               __attribute__((release_generic_capability(x)))

    #define HE_TSA_TRY_ACQUIRE(b, x)                __attribute__((try_acquire_capability(b, x)))
    #define HE_TSA_TRY_ACQUIRE_SHARED(b, x)         __attribute__((try_acquire_shared_capability(b, x)))
    #define HE_TSA_TRY_ACQUIRE_RECURSIVE(b, x)

    #define HE_TSA_EXCLUDES(x)                      __attribute__((locks_excluded(x)))

    #define HE_TSA_ASSERTS(x)                        __attribute__((assert_capability(x)))
    #define HE_TSA_ASSERTS_SHARED(x)                 __attribute__((assert_shared_capability(x)))

    #define HE_TSA_SCOPED_CAPABILITY()              __attribute__((scoped_lockable))
    #define HE_TSA_SCOPED_CTOR_ACQUIRE(x, y)        __attribute__((acquire_capability(x)))
    #define HE_TSA_SCOPED_CTOR_ACQUIRE_SHARED(x, y) __attribute__((acquire_shared_capability(x)))
    #define HE_TSA_SCOPED_DTOR_RELEASE(x)           __attribute__((release_capability(*this)))
    #define HE_TSA_SCOPED_DTOR_RELEASE_SHARED(x)    __attribute__((release_shared_capability(*this))))

    #define HE_TSA_IGNORE()                         __attribute__((no_thread_safety_analysis))
#elif HE_COMPILER_MSVC
    #define HE_TSA_CAPABILITY(x)
    #define HE_TSA_GUARDED_BY(x)                    _Guarded_by_(x)
    #define HE_TSA_WRITE_GUARDED_BY(x)              _Write_guarded_by_(x)
    #define HE_TSA_PT_GUARDED_BY(x)

    #define HE_TSA_ACQUIRED_BEFORE(x)
    #define HE_TSA_ACQUIRED_AFTER(x)

    #define HE_TSA_REQUIRES(x)                      _Requires_exclusive_lock_held_(x)
    #define HE_TSA_REQUIRES_SHARED(x)               _Requires_shared_lock_held_(x)

    #define HE_TSA_ACQUIRE(x)                       _Requires_lock_not_held_(x) _Acquires_exclusive_lock_(x) _Acquires_nonreentrant_lock_(x)
    #define HE_TSA_ACQUIRE_SHARED(x)                _Requires_lock_not_held_(x) _Acquires_shared_lock_(x) _Acquires_nonreentrant_lock_(x)
    #define HE_TSA_ACQUIRE_RECURSIVE(x)             _Requires_lock_not_held_(x) _Acquires_exclusive_lock_(x)

    #define HE_TSA_RELEASE(x)                       _Requires_exclusive_lock_held_(x) _Releases_exclusive_lock_(x) _Releases_nonreentrant_lock_(x)
    #define HE_TSA_RELEASE_SHARED(x)                _Requires_shared_lock_held_(x) _Releases_shared_lock_(x) _Releases_nonreentrant_lock_(x)
    #define HE_TSA_RELEASE_RECURSIVE(x)             _Requires_exclusive_lock_held_(x) _Releases_exclusive_lock_(x)
    #define HE_TSA_RELEASE_GENERIC(x)               _Requires_lock_held_(x) _Releases_lock_(x)

    #define HE_TSA_TRY_ACQUIRE(b, x)                _When_(return == b, _Acquires_exclusive_lock_(x)) _When_(return == b, _Acquires_nonreentrant_lock_(x))
    #define HE_TSA_TRY_ACQUIRE_SHARED(b, x)         _When_(return == b, _Acquires_shared_lock_(x)) _When_(return == b, _Acquires_nonreentrant_lock_(x))
    #define HE_TSA_TRY_ACQUIRE_RECURSIVE(b, x)      _When_(return == b, _Acquires_exclusive_lock_(x))

    #define HE_TSA_EXCLUDES(x)                      _Requires_lock_not_held_(x)

    #define HE_TSA_ASSERTS(x)
    #define HE_TSA_ASSERTS_SHARED(x)

    #define HE_TSA_SCOPED_CAPABILITY()
    #define HE_TSA_SCOPED_CTOR_ACQUIRE(x, y)        _Requires_lock_not_held_(x) _Acquires_exclusive_lock_(x) _Acquires_nonreentrant_lock_(x) _Post_satisfies_(&(x) == &(y)) _Post_same_lock_((x), (y))
    #define HE_TSA_SCOPED_CTOR_ACQUIRE_SHARED(x, y) _Requires_lock_not_held_(x) _Acquires_shared_lock_(x) _Acquires_nonreentrant_lock_(x) _Post_satisfies_(&(x) == &(y)) _Post_same_lock_((x), (y))
    #define HE_TSA_SCOPED_DTOR_RELEASE(x)           _Releases_exclusive_lock_(x) _Releases_nonreentrant_lock_(x)
    #define HE_TSA_SCOPED_DTOR_RELEASE_SHARED(x)    _Releases_shared_lock_(x) _Releases_nonreentrant_lock_(x)

    #define HE_TSA_IGNORE()
#else
    #define HE_TSA_CAPABILITY(x)
    #define HE_TSA_SCOPED_CAPABILITY()
    #define HE_TSA_GUARDED_BY(x)
    #define HE_TSA_PT_GUARDED_BY(x)

    #define HE_TSA_ACQUIRED_BEFORE(x)
    #define HE_TSA_ACQUIRED_AFTER(x)

    #define HE_TSA_REQUIRES(x)
    #define HE_TSA_REQUIRES_SHARED(x)

    #define HE_TSA_ACQUIRE(x)
    #define HE_TSA_ACQUIRE_SHARED(x)
    #define HE_TSA_ACQUIRE_RECURSIVE(x)

    #define HE_TSA_RELEASE(x)
    #define HE_TSA_RELEASE_SHARED(x)
    #define HE_TSA_RELEASE_RECURSIVE(x)
    #define HE_TSA_RELEASE_GENERIC(x)

    #define HE_TSA_TRY_ACQUIRE(b, x)
    #define HE_TSA_TRY_ACQUIRE_SHARED(b, x)
    #define HE_TSA_TRY_ACQUIRE_RECURSIVE(b, x)

    #define HE_TSA_EXCLUDES(x)

    #define HE_TSA_ASSERT_CAPABILITY(x)
    #define HE_TSA_ASSERT_SHARED_CAPABILITY(x)

    #define HE_TSA_SCOPED_CAPABILITY()
    #define HE_TSA_SCOPED_CTOR_ACQUIRE(x, y)
    #define HE_TSA_SCOPED_CTOR_ACQUIRE_SHARED(x, y)
    #define HE_TSA_SCOPED_DTOR_RELEASE(x)
    #define HE_TSA_SCOPED_DTOR_RELEASE_SHARED(x)

    #define HE_TSA_IGNORE()
#endif

/// \def HE_TSA_CAPABILITY
/// Class attribute which specifies that objects of the class can be used as a capability.
/// The string argument specifies the kind of capability in error messages, e.g. "mutex".
///
/// \param[in] x The string name of the capability.

/// \def HE_TSA_GUARDED_BY
/// Data member attribute which declares that the data is protected by the given capability.
/// Read operations on the data require shared access, while write operations require exclusive
/// access.
///
/// \param[in] x The name of the data that is guarded by the capability.

/// \def HE_TSA_WRITE_GUARDED_BY
/// Data member attribute which declares that the data is protected by the given capability.
/// Read operations on the data do not require access, while write operations require exclusive
/// access.
///
/// \param[in] x The name of the data that is guarded by the capability.

/// \def HE_TSA_PT_GUARDED_BY
/// Similar to \ref HE_TSA_GUARDED_BY, but is intended for use on pointers and smart pointers.
/// There is no constraint on the member itself, but the data that it points to is protected
/// by the given capability.
///
/// \param[in] x The name of the pointer to data that is guarded by the capability.

/// \def HE_TSA_ACQUIRED_BEFORE
/// Data member attribute, specifically declarations of mutexes or other capabilities.
/// This attribute enforces a particular order in which the mutexes must be acquired, in order
/// to prevent deadlock.
///
/// \param[in] x The name of the capability that must be acquired before this one.

/// \def HE_TSA_ACQUIRED_AFTER
/// Data member attribute, specifically declarations of mutexes or other capabilities.
/// This attribute enforces a particular order in which the mutexes must be acquired, in order
/// to prevent deadlock.
///
/// \param[in] x The name of the capability that must be acquired after this one.

/// \def HE_TSA_REQUIRES
/// Function attribute declaring that the calling thread must have exclusive access to the given
/// capability. The capability must be held on entry to the function, and must still be held on
/// exit.
///
/// \param[in] x The capability that is required.

/// \def HE_TSA_REQUIRES_SHARED
/// Function attribute declaring that the calling thread must have shared access to the given
/// capability. The capability must be held on entry to the function, and must still be held on
/// exit.
///
/// \param[in] x The capability that is required.

/// \def HE_TSA_ACQUIRE
/// Function attribute declaring that a function acquires a capability, but does not release it.
/// The given capability must not be held on entry, and will be held exclusively on exit.
///
/// \param[in] x The name of the capability that is being acquired.

/// \def HE_TSA_ACQUIRE_SHARED
/// Function attribute declaring that the function acquires a capability, but does not release it.
/// The given capability must not be held on entry, and will be held as shared on exit.
///
/// \param[in] x The name of the capability that is being acquired.

/// \def HE_TSA_ACQUIRE_RECURSIVE
/// Function attribute declaring that a function acquires a capability, but does not release it.
/// The given capability may be held on entry, and will be held exclusively on exit.
///
/// \param[in] x The name of the capability that is being acquired.

/// \def HE_TSA_RELEASE
/// Function attribute declaring that the function releases the given capability. The capability
/// must be held exclusively on entry, and will no longer be held on exit.
///
/// \param[in] x The name of the capability that is being released.

/// \def HE_TSA_RELEASE_SHARED
/// Function attribute declaring that the function releases the given capability. The capability
/// must be held as shared on entry, and will no longer be held on exit.
///
/// \param[in] x The name of the capability that is being released.

/// \def HE_TSA_RELEASE_RECURSIVE
/// Function attribute declaring that the function releases the given capability. The capability
/// must be held exclusively on entry, and may no longer be held on exit.
///
/// \param[in] x The name of the capability that is being released.

/// \def HE_TSA_RELEASE_GENERIC
/// Function attribute declaring that the function releases the given capability. The capability
/// must be held exclusively or as shared on entry, and will no longer be held on exit.
///
/// \param[in] x The name of the capability that is being released.

/// \def HE_TSA_TRY_ACQUIRE
/// Function attribute declaring that the function tries to acquire the given capability, and
/// returns a boolean value indicating success or failure. The first argument must be true or
/// false, to specify which return value indicates success, and the remaining arguments are
/// interpreted in the same way as \ref HE_TSA_ACQUIRE.
///
/// Because the analysis doesn't support conditional locking, a capability is treated as acquired
/// after the first branch on the return value of a try-acquire function.
///
/// \param[in] b The return value which indicates whether the acquisition was successful.
/// \param[in] x The name of the capability that is being acquired.

/// \def HE_TSA_TRY_ACQUIRE_SHARED
/// Function attribute declaring that the function tries to acquire the given capability, and
/// returns a boolean value indicating success or failure. The first argument must be true or
/// false, to specify which return value indicates success, and the remaining arguments are
/// interpreted in the same way as \ref HE_TSA_ACQUIRE_SHARED.
///
/// Because the analysis doesn't support conditional locking, a capability is treated as acquired
/// after the first branch on the return value of a try-acquire function.
///
/// \param[in] b The return value which indicates whether the acquisition was successful.
/// \param[in] x The name of the capability that is being acquired.

/// \def HE_TSA_TRY_ACQUIRE_RECURSIVE
/// Function attribute declaring that the function tries to acquire the given capability, and
/// returns a boolean value indicating success or failure. The first argument must be true or
/// false, to specify which return value indicates success.
///
/// Because the analysis doesn't support conditional locking, a capability is treated as acquired
/// after the first branch on the return value of a try-acquire function.
///
/// \param[in] b The return value which indicates whether the acquisition was successful.
/// \param[in] x The name of the capability that is being acquired.

/// \def HE_TSA_EXCLUDES
/// Function attribute declaring that the caller must not hold the given capability.
/// This annotation is used to prevent deadlock. Many mutex implementations are not re-entrant,
/// so deadlock can occur if the function acquires the mutex a second time.
///
/// \param[in] x The name of the capability that is being excluded.

/// \def HE_TSA_ASSERTS
/// Function attribute which asserts the calling thread already holds a capability exclusively.
/// For example, by performing a run-time test and terminating if the capability is not held.
/// Presence of this annotation causes the analysis to assume the capability is held after calls
/// to the annotated function.
///
/// \param[in] x The name of the capability to assert is held.

/// \def HE_TSA_ASSERTS_SHARED
/// Function attribute which asserts the calling thread already holds a capability as shared.
/// For example, by performing a run-time test and terminating if the capability is not held.
/// Presence of this annotation causes the analysis to assume the capability is held after calls
/// to the annotated function.
///
/// \param[in] x The name of the capability to assert is held.

/// \def HE_TSA_SCOPED_CAPABILITY
/// Class attribute declaring that the class implements RAII-style locking, in which a capability
/// is acquired in the constructor, and released in the destructor. Such classes require special
/// handling because the constructor and destructor refer to the capability via different names.

/// \def HE_TSA_SCOPED_CTOR_ACQUIRE
/// Constructor function attribute declaring that the constructor acquires a capability, and that
/// the capability is stored in a member variable. The capability must not be held on entry, and
/// will be held exclusively when construction completes.
///
/// \param[in] x The name of the parameter the capability is passed in as.
/// \param[in] y The name of the member variable the capability is stored in.

/// \def HE_TSA_SCOPED_CTOR_ACQUIRE_SHARED
/// Constructor function attribute declaring that the constructor acquires a capability, and that
/// the capability is stored in a member variable. The capability must not be held on entry, and
/// will be held as shared when construction completes.
///
/// \param[in] x The name of the parameter the capability is passed in as.
/// \param[in] y The name of the member variable the capability is stored in.

/// \def HE_TSA_SCOPED_DTOR_RELEASE
/// Destructor function attribute declaring that the destructor releases a capability. The
/// capability must be held exclusively on entry, and will no longer be held when destruction
/// completes.
///
/// \param[in] x The name of the member variable the capability is stored in.

/// \def HE_TSA_SCOPED_DTOR_RELEASE_SHARED
/// Destructor function attribute declaring that the destructor releases a capability. The
/// capability must be held as shared on entry, and will no longer be held when destruction
/// completes.
///
/// \param[in] x The name of the member variable the capability is stored in.

/// \def HE_TSA_IGNORE
/// Function attribute which turns off thread safety checking for that method. It provides an
/// escape hatch for functions which are either deliberately thread-unsafe, or are thread-safe
/// but too complicated for the analysis to understand.
