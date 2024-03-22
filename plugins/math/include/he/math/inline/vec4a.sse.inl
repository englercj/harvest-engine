// Copyright Chad Engler

#include "he/core/math.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Conversion

    inline Vec4a MakeVec4a(const Vec4a& x, const Vec4a& y)
    {
        __m128 zero = _mm_setzero_ps();
        __m128 a = _mm_unpacklo_ps(x, y);
        return _mm_movelh_ps(a, zero);
    }

    inline Vec4a MakeVec4a(const Vec4a& x, const Vec4a& y, const Vec4a& z)
    {
        constexpr float AllBits = BitCast<float>(0xffffffff);
        constexpr Vec4a Vec4a_Select1110{ AllBits, AllBits, AllBits, 0 };

        __m128 a = _mm_unpacklo_ps(x, y);
        __m128 b = _mm_movelh_ps(a, z);
        return _mm_and_ps(b, Vec4a_Select1110);
    }

    inline Vec4a MakeVec4a(const Vec4a& x, const Vec4a& y, const Vec4a& z, const Vec4a& w)
    {
        __m128 a = _mm_unpacklo_ps(x, y);
        __m128 b = _mm_unpacklo_ps(z, w);
        return _mm_movelh_ps(a, b);
    }

    template <> inline Vec4a MakeVec4a(const Vec2<float>& v)
    {
        __m128d loaded = _mm_load_sd(reinterpret_cast<const double*>(&v));
        return _mm_castpd_ps(loaded);
    }

    template <> inline Vec4a MakeVec4a(const Vec2<int32_t>& v)
    {
        __m128d loaded = _mm_load_sd(reinterpret_cast<const double*>(&v));
        __m128i cast = _mm_castpd_si128(loaded);
        return _mm_cvtepi32_ps(cast);
    }

    template <> inline Vec4a MakeVec4a(const Vec2<uint32_t>& v)
    {
        __m128d loaded = _mm_load_sd(reinterpret_cast<const double*>(&v));
        __m128i cast = _mm_castpd_si128(loaded);
        return _mm_cvtepi32_ps(cast);
    }

    template <> inline Vec4a MakeVec4a(const Vec3<float>& v)
    {
        __m128 low = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(&v)));
        __m128 high = _mm_load_ss(&v.z);
        return _mm_movelh_ps(low, high);
    }

    template <> inline Vec4a MakeVec4a(const Vec3<int32_t>& v)
    {
        __m128d low = _mm_load_sd(reinterpret_cast<const double*>(&v));
        __m128i high = _mm_cvtsi32_si128(v.z);
        __m128i combined = _mm_castps_si128(_mm_movelh_ps(_mm_castpd_ps(low), _mm_castsi128_ps(high)));
        return _mm_cvtepi32_ps(combined);
    }

    template <> inline Vec4a MakeVec4a(const Vec3<uint32_t>& v)
    {
        __m128d low = _mm_load_sd(reinterpret_cast<const double*>(&v));
        __m128i high = _mm_cvtsi32_si128(v.z);
        __m128i combined = _mm_castps_si128(_mm_movelh_ps(_mm_castpd_ps(low), _mm_castsi128_ps(high)));
        return _mm_cvtepi32_ps(combined);
    }

    template <> inline Vec4a MakeVec4a(const Vec4<float>& v)
    {
        return _mm_loadu_ps(&v.x);
    }

    template <> inline Vec4a MakeVec4a(const Vec4<int32_t>& v)
    {
        __m128i loaded = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&v));
        return _mm_cvtepi32_ps(loaded);
    }

    template <> inline Vec4a MakeVec4a(const Vec4<uint32_t>& v)
    {
        __m128i loaded = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&v));
        return _mm_cvtepi32_ps(loaded);
    }

    inline Vec4a SplatZero()
    {
        return _mm_setzero_ps();
    }

    inline Vec4a Splat(float x)
    {
        return _mm_set_ps1(x);
    }

    inline Vec4a SplatX(const Vec4a& v)
    {
        return _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
    }

    inline Vec4a SplatY(const Vec4a& v)
    {
        return _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
    }

    inline Vec4a SplatZ(const Vec4a& v)
    {
        return _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
    }

    inline Vec4a SplatW(const Vec4a& v)
    {
        return _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3));
    }

    // --------------------------------------------------------------------------------------------
    // Component Access

    inline float GetComponent(const Vec4a& v, size_t i)
    {
        HE_ASSERT(i < 4);
    #if HE_COMPILER_MSVC
        return v.v.m128_f32[i];
    #else
        return v.v[i];
    #endif
    }

    inline Vec4a SetComponent(const Vec4a& v, size_t i, float s)
    {
        HE_ASSERT(i < 4);
        Vec4a r = v;
    #if HE_COMPILER_MSVC
        r.v.m128_f32[i] = s;
    #else
        r.v[i] = s;
    #endif
        return r;
    }

    inline float GetX(const Vec4a& v)
    {
        return _mm_cvtss_f32(v);
    }

    inline float GetY(const Vec4a& v)
    {
        return _mm_cvtss_f32(_mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)));
    }

    inline float GetZ(const Vec4a& v)
    {
        return _mm_cvtss_f32(_mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)));
    }

    inline float GetW(const Vec4a& v)
    {
        return _mm_cvtss_f32(_mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3)));
    }

    inline Vec4a SetX(const Vec4a& v, float s)
    {
        __m128 a = _mm_set_ss(s);
        return _mm_move_ss(v, a);
    }

    inline Vec4a SetY(const Vec4a& v, float s)
    {
    #if HE_SIMD_SSE4_1
        __m128 temp = _mm_set_ss(s);
        return _mm_insert_ps(v, temp, 0x10);
    #else
        __m128 a = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 2, 0, 1));
        __m128 b = _mm_move_ss(a, _mm_set_ss(s));
        return _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 2, 0, 1));
    #endif
    }

    inline Vec4a SetZ(const Vec4a& v, float s)
    {
    #if HE_SIMD_SSE4_1
        __m128 temp = _mm_set_ss(s);
        return _mm_insert_ps(v, temp, 0x20);
    #else
        __m128 a = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 0, 1, 2));
        __m128 b = _mm_move_ss(a, _mm_set_ss(s));
        return _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 1, 2));
    #endif
    }

    inline Vec4a SetW(const Vec4a& v, float s)
    {
    #if HE_SIMD_SSE4_1
        __m128 temp = _mm_set_ss(s);
        return _mm_insert_ps(v, temp, 0x30);
    #else
        __m128 a = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 2, 1, 3));
        __m128 b = _mm_move_ss(a, _mm_set_ss(s));
        return _mm_shuffle_ps(b, b, _MM_SHUFFLE(0, 2, 1, 3));
    #endif
    }

    inline Vec4a Load(const float* p)
    {
        return _mm_load_ps(p);
    }

    inline Vec4a LoadU(const float* p)
    {
        return _mm_loadu_ps(p);
    }

    inline void Store(float* p, const Vec4a& v)
    {
        _mm_store_ps(p, v);
    }

    inline void StoreU(float* p, const Vec4a& v)
    {
        _mm_storeu_ps(p, v);
    }

    // --------------------------------------------------------------------------------------------
    // Classification

    inline bool IsNan(const Vec4a& v)
    {
        __m128 cmp = _mm_cmpunord_ps(v, v);
        //__m128 cmp = _mm_cmpneq_ps(v, v);
        return _mm_movemask_ps(cmp) != 0;
    }

    inline bool IsNan3(const Vec4a& v)
    {
        __m128 cmp = _mm_cmpunord_ps(v, v);
        //__m128 cmp = _mm_cmpneq_ps(v, v);
        return (_mm_movemask_ps(cmp) & 7) != 0;
    }

    inline bool IsInfinite(const Vec4a& v)
    {
        __m128 a = _mm_and_ps(v, Vec4a_AbsMask);
        __m128 b = _mm_cmpeq_ps(a, Vec4a_Infinity);
        return _mm_movemask_ps(b) != 0;
    }

    inline bool IsInfinite3(const Vec4a& v)
    {
        __m128 a = _mm_and_ps(v, Vec4a_AbsMask);
        __m128 b = _mm_cmpeq_ps(a, Vec4a_Infinity);
        return (_mm_movemask_ps(b) & 7) != 0;
    }

    // --------------------------------------------------------------------------------------------
    // Arithmetic

    inline Vec4a Negate(const Vec4a& v)
    {
        return _mm_xor_ps(_mm_set_ps1(-0.0f), v);
    }

    inline Vec4a Add(const Vec4a& a, const Vec4a& b)
    {
        return _mm_add_ps(a, b);
    }

    inline Vec4a Add(const Vec4a& a, float b)
    {
        return _mm_add_ps(a, _mm_set_ps1(b));
    }

    inline Vec4a Sub(const Vec4a& a, const Vec4a& b)
    {
        return _mm_sub_ps(a, b);
    }

    inline Vec4a Sub(const Vec4a& a, float b)
    {
        return _mm_sub_ps(a, _mm_set_ps1(b));
    }

    inline Vec4a Mul(const Vec4a& a, const Vec4a& b)
    {
        return _mm_mul_ps(a, b);
    }

    inline Vec4a Mul(const Vec4a& a, float b)
    {
        return _mm_mul_ps(a, _mm_set_ps1(b));
    }

    inline Vec4a MulAdd(const Vec4a& a, const Vec4a& b, const Vec4a& add)
    {
    #if HE_SIMD_FMA3
        return _mm_fmadd_ps(a, b, add);
    #else
        return _mm_add_ps(_mm_mul_ps(a, b), add);
    #endif
    }

    inline Vec4a Div(const Vec4a& a, const Vec4a& b)
    {
        return _mm_div_ps(a, b);
    }

    inline Vec4a Div(const Vec4a& a, float b)
    {
        return _mm_div_ps(a, _mm_set_ps1(b));
    }

    inline Vec4a Lerp(const Vec4a& a, const Vec4a& b, float t)
    {
        __m128 l = _mm_sub_ps(b, a);
        __m128 s = _mm_set_ps1(t);
        return MulAdd(l, s, a);
    }

    inline Vec4a SmoothStep(const Vec4a& a, const Vec4a& b, const Vec4a& t)
    {
        __m128 te = _mm_sub_ps(t, a);
        __m128 ee = _mm_sub_ps(b, a);
        __m128 v = _mm_div_ps(te, ee);

        __m128 zero = _mm_setzero_ps();
        __m128 one = _mm_set_ps1(1.0f);

        __m128 c = _mm_max_ps(zero, _mm_min_ps(v, one));

        __m128 three = _mm_set_ps1(3.0f);
        return _mm_mul_ps(_mm_mul_ps(c, c), _mm_sub_ps(three, _mm_add_ps(c, c)));
    }

    inline Vec4a Abs(const Vec4a& v)
    {
        return _mm_andnot_ps(_mm_set_ps1(-0.f), v);
    }

    inline Vec4a Rcp(const Vec4a& v)
    {
        return _mm_div_ps(_mm_set_ps1(1.0f), v);
    }

    inline Vec4a RcpSafe(const Vec4a& v)
    {
        __m128 t = _mm_div_ps(_mm_set_ps1(1.0f), v);
        __m128 p = _mm_cmpgt_ps(Abs(v), _mm_set_ps1(Limits<float>::ZeroSafe));

    #if HE_SIMD_SSE4_1
        return _mm_blendv_ps(Vec4a_Zero, t, p);
    #else
        return _mm_or_ps(_mm_and_ps(p, t), _mm_andnot_ps(p, Vec4a_Zero));
    #endif
    }

    inline Vec4a Sqrt(const Vec4a& v)
    {
        return _mm_sqrt_ps(v);
    }

    inline Vec4a Rsqrt(const Vec4a& v)
    {
        return _mm_div_ps(_mm_set_ps1(1.0f), _mm_sqrt_ps(v));
    }

    // --------------------------------------------------------------------------------------------
    // Selection

    inline Vec4a Min(const Vec4a& a, const Vec4a& b)
    {
        return _mm_min_ps(a, b);
    }

    inline Vec4a Max(const Vec4a& a, const Vec4a& b)
    {
        return _mm_max_ps(a, b);
    }

    inline Vec4a Clamp(const Vec4a& v, const Vec4a& lo, const Vec4a& hi)
    {
        return _mm_max_ps(lo, _mm_min_ps(v, hi));
    }

    inline float HMin(const Vec4a& v)
    {
    #if HE_SIMD_SSE3
        __m128 swp0 = _mm_movehdup_ps(v);
    #else
        __m128 swp0 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1));
    #endif
        __m128 min0 = _mm_min_ps(v, swp0);
        __m128 swp1 = _mm_movehl_ps(swp0, min0);
        __m128 min1 = _mm_min_ss(min0, swp1);
        return _mm_cvtss_f32(min1);
    }

    inline float HMax(const Vec4a& v)
    {
    #if HE_SIMD_SSE3
        __m128 swp0 = _mm_movehdup_ps(v);
    #else
        __m128 swp0 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1));
    #endif
        __m128 max0 = _mm_max_ps(v, swp0);
        __m128 swp1 = _mm_movehl_ps(swp0, max0);
        __m128 max1 = _mm_max_ss(max0, swp1);
        return _mm_cvtss_f32(max1);
    }

    inline float HAdd(const Vec4a& v)
    {
    #if HE_SIMD_SSE3
        __m128 add0 = _mm_hadd_ps(v, v);
        __m128 add1 = _mm_hadd_ps(add0, add0);
        return _mm_cvtss_f32(add1);
    #else
        __m128 swp0 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1));
        __m128 add0 = _mm_add_ps(v, swp0);
        __m128 swp1 = _mm_movehl_ps(swp0, add0);
        __m128 add1 = _mm_add_ss(add0, swp1);
        return _mm_cvtss_f32(add1);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    // Geometric

    inline __m128 DotVec(const Vec4a& a, const Vec4a& b)
    {
    #if HE_SIMD_SSE4_1
        return _mm_dp_ps(a, b, 0xff);
    #elif HE_SIMD_SSE3
        __m128 mul0 = _mm_mul_ps(a, b);
        __m128 add0 = _mm_hadd_ps(mul0, mul0);
        return _mm_hadd_ps(add0, add0);
    #else
        __m128 ab = _mm_mul_ps(a, b);
        __m128 swp0 = _mm_shuffle_ps(ab, ab, _MM_SHUFFLE(2, 3, 0, 1));
        __m128 add0 = _mm_add_ps(ab, swp0);
        __m128 swp1 = _mm_shuffle_ps(add0, add0, _MM_SHUFFLE(1, 0, 3, 2));
        return _mm_add_ps(add0, swp1);
    #endif
    }

    inline __m128 Dot3Vec(const Vec4a& a, const Vec4a& b)
    {
    #if HE_SIMD_SSE4_1
        return _mm_dp_ps(a, b, 0x7f);
    #elif HE_SIMD_SSE3
        constexpr float AllBits = BitCast<float>(0xffffffff);
        constexpr Vec4a Vec4a_Select1110{ AllBits, AllBits, AllBits, 0 };

        __m128 mul0 = _mm_mul_ps(a, b);
        __m128 xyz0 = _mm_and_ps(vTemp, Vec4a_Select1110);
        __m128 add0 = _mm_hadd_ps(xyz0, xyz0);
        return _mm_hadd_ps(add0, add0);
    #else
        __m128 ab = _mm_mul_ps(a, b);
        __m128 swp0 = _mm_shuffle_ps(ab, ab, _MM_SHUFFLE(2, 1, 2, 1));
        __m128 add0 = _mm_add_ss(ab, swp0);
        __m128 swp1 = _mm_shuffle_ps(swp0, swp0, _MM_SHUFFLE(1, 1, 1, 1));
        return _mm_add_ss(add0, swp1);
    #endif
    }

    inline float Dot(const Vec4a& a, const Vec4a& b)
    {
        return _mm_cvtss_f32(DotVec(a, b));
    }

    inline float Dot3(const Vec4a& a, const Vec4a& b)
    {
        return _mm_cvtss_f32(Dot3Vec(a, b));
    }

    inline Vec4a Cross(const Vec4a& a, const Vec4a& b)
    {
        // From: http://threadlocalmutex.com/?p=8
        __m128 ayzx = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));
        __m128 byzx = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
        __m128 c = _mm_sub_ps(_mm_mul_ps(a, byzx), _mm_mul_ps(ayzx, b));
        return _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1));
    }

    inline Vec4a Normalize(const Vec4a& v)
    {
        __m128 dot = DotVec(v, v);
        __m128 rsqr = _mm_div_ss(_mm_set_ss(1.0f), _mm_sqrt_ss(dot));
        __m128 swp0 = _mm_shuffle_ps(rsqr, rsqr, _MM_SHUFFLE(0, 0, 0, 0));
        return _mm_mul_ps(swp0, v);
    }

    inline Vec4a Normalize3(const Vec4a& v)
    {
        __m128 dot = Dot3Vec(v, v);
        __m128 rsqr = _mm_div_ss(_mm_set_ss(1.0f), _mm_sqrt_ss(dot));
        __m128 swp0 = _mm_shuffle_ps(rsqr, rsqr, _MM_SHUFFLE(0, 0, 0, 0));
        return _mm_mul_ps(swp0, v);
    }

    // --------------------------------------------------------------------------------------------
    // Comparison

    inline Vec4a Lt(const Vec4a& a, const Vec4a& b)
    {
        return _mm_cmplt_ps(a, b);
    }

    inline Vec4a Le(const Vec4a& a, const Vec4a& b)
    {
        return _mm_cmple_ps(a, b);
    }

    inline Vec4a Gt(const Vec4a& a, const Vec4a& b)
    {
        return _mm_cmpgt_ps(a, b);
    }

    inline Vec4a Ge(const Vec4a& a, const Vec4a& b)
    {
        return _mm_cmpge_ps(a, b);
    }

    inline Vec4a Eq(const Vec4a& a, const Vec4a& b)
    {
        return _mm_cmpeq_ps(a, b);
    }

    inline Vec4a Ne(const Vec4a& a, const Vec4a& b)
    {
        return _mm_cmpneq_ps(a, b);
    }

    inline Vec4a EqInt(const Vec4a& a, const Vec4a& b)
    {
        return _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(a), _mm_castps_si128(b)));
    }

    inline bool Any(const Vec4a& cmp)
    {
        return _mm_movemask_ps(cmp) != 0;
    }

    inline bool Any3(const Vec4a& cmp)
    {
        return (_mm_movemask_ps(cmp) & 7) != 0;
    }

    inline bool All(const Vec4a& cmp)
    {
        return _mm_movemask_ps(cmp) == 0x0f;
    }

    inline bool All3(const Vec4a& cmp)
    {
        return (_mm_movemask_ps(cmp) & 7) == 7;
    }
}
