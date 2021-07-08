// Copyright Chad Engler

#pragma once

/// Ignores its arguments.
///
/// Useful to avoid a parameter unused error, yet keep the parameter name.
#define HE_UNUSED(...) (false ? _heUnused(__VA_ARGS__) : void())
template <typename...Ts> void _heUnused(Ts&&...) { }

/// Returns the length of a fixed array.
///
/// Results are garuanteed to be constexpr.
#define HE_LENGTH_OF(...) static_cast<unsigned int>(sizeof(_heLengthOf(__VA_ARGS__)))
template <typename T, unsigned int N> char (&_heLengthOf(const T (&)[N]))[N];

/// \def HE_STRINGIFY
/// Performs preprocessor token stringization.
///
/// This handles the preprocessor indirection necessary to evaluate macro parameters.
#define HE_STRINGIFY_(x) #x
#define HE_STRINGIFY(x) HE_STRINGIFY_(x)

/// \def HE_CONCAT
/// Concatenate two different tokens together.
///
/// This handles the preprocessor indirection necessary to evaluate macro parameters.
#define HE_CONCAT_(a, b) a ## b
#define HE_CONCAT(a, b) HE_CONCAT_(a, b)

/// Indirection so the preprocessor expands the argument
#define HE_PP_EXPAND(x) x

/// \def HE_PP_COUNT_ARGS(...)
/// Counts the number of arguments in __VA_ARGS__
#if defined(_MSC_VER)
    #define HE_PP_COUNT_ARGS_(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N
    #define HE_PP_COUNT_ARGS_EXPAND_(...) HE_PP_EXPAND(HE_PP_COUNT_ARGS_(__VA_ARGS__, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
    #define HE_PP_COUNT_ARGS_HELPER_(...) unused, __VA_ARGS__
    #define HE_PP_COUNT_ARGS(...) HE_PP_COUNT_ARGS_EXPAND_(HE_PP_COUNT_ARGS_HELPER_(__VA_ARGS__))
#else
    #define HE_PP_COUNT_ARGS_(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N
    #define HE_PP_COUNT_ARGS(...) HE_PP_COUNT_ARGS_(0, ##__VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#endif

/// Expands to the first argument that was passed in
#define HE_PP_FIRST_ARG(x, ...) (x)

/// Expands to the arguments passed in, excluding the first
#define HE_PP_REST_ARGS(x, ...) (__VA_ARGS__)

/// \def HE_PP_FOREACH(m, list)
/// Expands the `m` macro for each element in `list`
#define HE_PP_FOREACH_0(m, list)
#define HE_PP_FOREACH_1(m, list) m list
#define HE_PP_FOREACH_2(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_1(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_3(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_2(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_4(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_3(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_5(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_4(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_6(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_5(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_7(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_6(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_8(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_7(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_9(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_8(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_10(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_9(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_11(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_10(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_12(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_11(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_13(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_12(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_14(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_13(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_15(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_14(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_16(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_15(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_17(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_16(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_18(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_17(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_19(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_18(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH_20(m, list) HE_PP_EXPAND(m HE_PP_FIRST_ARG list) HE_PP_FOREACH_19(m, HE_PP_REST_ARGS list)
#define HE_PP_FOREACH__(n, m, list) HE_PP_FOREACH_##n(m, list)
#define HE_PP_FOREACH_(n, m, list) HE_PP_FOREACH__(n, m, list)
#define HE_PP_FOREACH(m, list) HE_PP_FOREACH_(HE_PP_COUNT_ARGS list, m, list)
