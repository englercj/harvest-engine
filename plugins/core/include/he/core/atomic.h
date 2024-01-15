// TODO: Need platform specific implementations, because I can't rely on <atomic> for WASM
// Can probably use compiler intrinsics for this
// e.g. __atomic_fetch_add, __atomic_fetch_sub, __atomic_fetch_and, __atomic_fetch_or, __atomic_fetch_xor
// e.g. __atomic_load, __atomic_store
// e.g. __atomic_exchange
// e.g. __atomic_compare_exchange
// e.g. __atomic_test_and_set, __atomic_clear
// e.g. __atomic_thread_fence
// e.g. __atomic_signal_fence
// e.g. __atomic_always_lock_free, __atomic_is_lock_free
// e.g. __atomic_wait, __atomic_notify
// e.g. __atomic_load_n, __atomic_store_n
// e.g. __atomic_add_fetch, __atomic_sub_fetch, __atomic_and_fetch, __atomic_or_fetch, __atomic_xor_fetch
// e.g. __atomic_exchange_n
// e.g. __atomic_compare_exchange_n
// e.g. __atomic_test_and_set, __atomic_clear
