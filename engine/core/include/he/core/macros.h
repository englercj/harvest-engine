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
#define HE_LENGTH_OF(...) static_cast<uint32_t>(sizeof(_heLengthOf(__VA_ARGS__)))
template <typename T, size_t N> char (&_heLengthOf(const T (&)[N]))[N];

/// Performs preprocessor token stringization.
///
/// This handles the preprocessor indirection necessary to evaluate macro parameters.
#define HE_STRINGIFY(x) HE_STRINGIFY_(x)
#define HE_STRINGIFY_(x) #x

/// Concatenate two different tokens together.
///
/// This handles the preprocessor indirection necessary to evaluate macro parameters.
#define HE_CONCAT(a, b) HE_CONCAT_(a, b)
#define HE_CONCAT_(a, b) a ## b
