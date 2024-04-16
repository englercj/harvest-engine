// Copyright Chad Engler

#include "he/core/math.h"

#include "he/core/compiler.h"
#include "he/core/config.h"
#include "he/core/enum_ops.h"
#include "he/core/simd.h"

#if defined(HE_PLATFORM_API_WASM)
    #include "he/core/wasm/lib_core.wasm.h"
#else
    #include <cmath>
#endif

namespace he
{
    // TODO: Calling to JS for the wasm math functions is slow. We should implement these functions in C++.
    // For example: http://gruntthepeon.free.fr/ssemath/

    // --------------------------------------------------------------------------------------------
    float Sin(float x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return static_cast<float>(heWASM_Sin(x));
    #else
        return std::sinf(x);
    #endif
    }

    double Sin(double x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return heWASM_Sin(x);
    #else
        return std::sin(x);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    float Asin(float x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return static_cast<float>(heWASM_Asin(x));
    #else
        return std::asinf(x);
    #endif
    }

    double Asin(double x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return heWASM_Asin(x);
    #else
        return std::asin(x);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    float Cos(float x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return static_cast<float>(heWASM_Cos(x));
    #else
        return std::cosf(x);
    #endif
    }

    double Cos(double x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return heWASM_Cos(x);
    #else
        return std::cos(x);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    float Acos(float x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return static_cast<float>(heWASM_Acos(x));
    #else
        return std::acosf(x);
    #endif
    }

    double Acos(double x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return heWASM_Acos(x);
    #else
        return std::acos(x);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    float Tan(float x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return static_cast<float>(heWASM_Tan(x));
    #else
        return std::tanf(x);
    #endif
    }

    double Tan(double x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return heWASM_Tan(x);
    #else
        return std::tan(x);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    float Atan(float x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return static_cast<float>(heWASM_Atan(x));
    #else
        return std::atanf(x);
    #endif
    }

    double Atan(double x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return heWASM_Atan(x);
    #else
        return std::atan(x);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    float Atan2(float y, float x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return static_cast<float>(heWASM_Atan2(y, x));
    #else
        return std::atan2f(y, x);
    #endif
    }

    double Atan2(double y, double x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return heWASM_Atan2(y, x);
    #else
        return std::atan2(y, x);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    void SinCos(float x, float& s, float& c) noexcept
    {
        // TODO: Optimize
        s = Sin(x);
        c = Cos(x);
    }

    void SinCos(double x, double& s, double& c) noexcept
    {
        // TODO: Optimize
        s = Sin(x);
        c = Cos(x);
    }

    // --------------------------------------------------------------------------------------------
    float Exp(float x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return static_cast<float>(heWASM_Exp(x));
    #else
        return std::expf(x);
    #endif
    }

    double Exp(double x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return heWASM_Exp(x);
    #else
        return std::exp(x);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    float Log(float x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return static_cast<float>(heWASM_Log(x));
    #else
        return std::logf(x);
    #endif
    }

    double Log(double x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return heWASM_Log(x);
    #else
        return std::log(x);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    float Log2(float x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return static_cast<float>(heWASM_Log2(x));
    #else
        return std::log2f(x);
    #endif
    }

    double Log2(double x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return heWASM_Log2(x);
    #else
        return std::log2(x);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    float Log10(float x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return static_cast<float>(heWASM_Log10(x));
    #else
        return std::log10f(x);
    #endif
    }

    double Log10(double x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return heWASM_Log10(x);
    #else
        return std::log10(x);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    float Log1p(float x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return static_cast<float>(heWASM_Log1p(x));
    #else
        return std::log1pf(x);
    #endif
    }

    double Log1p(double x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return heWASM_Log1p(x);
    #else
        return std::log1p(x);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    float Pow(float base, float exp) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return static_cast<float>(heWASM_Pow(base, exp));
    #else
        return std::powf(base, exp);
    #endif
    }

    double Pow(double base, double exp) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return heWASM_Pow(base, exp);
    #else
        return std::pow(base, exp);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    float Fmod(float x, float y) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return static_cast<float>(heWASM_Fmod(x, y));
    #else
        return std::fmodf(x, y);
    #endif
    }

    double Fmod(double x, double y) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        return heWASM_Fmod(x, y);
    #else
        return std::fmod(x, y);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    template <>
    const char* EnumTraits<FpClass>::ToString(FpClass x) noexcept
    {
        switch (x)
        {
            case FpClass::Normal: return "Normal";
            case FpClass::Subnormal: return "Subnormal";
            case FpClass::Zero: return "Zero";
            case FpClass::Infinite: return "Infinite";
            case FpClass::Nan: return "Nan";
        }

        return "<unknown>";
    }
}
