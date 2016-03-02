/**
 * Copyright 2015 Kurt Kanzenbach
 * Copyright 2016 Andreas Schäfer
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef FLAT_ARRAY_DETAIL_SHORT_VEC_SSE_INT_8_HPP
#define FLAT_ARRAY_DETAIL_SHORT_VEC_SSE_INT_8_HPP

#ifdef __SSE2__

#include <emmintrin.h>
#include <libflatarray/detail/sqrt_reference.hpp>
#include <libflatarray/detail/short_vec_helpers.hpp>
#include <libflatarray/config.h>
#include <iostream>

#ifdef __SSE4_1__
#include <smmintrin.h>
#endif

#ifdef LIBFLATARRAY_WITH_CPP14
#include <initializer_list>
#endif

#ifndef __CUDA_ARCH__
#ifndef __AVX2__

namespace LibFlatArray {

template<typename CARGO, int ARITY>
class short_vec;

template<typename CARGO, int ARITY>
class sqrt_reference;

#ifdef __ICC
// disabling this warning as implicit type conversion is exactly our goal here:
#pragma warning push
#pragma warning (disable: 2304)
#endif

template<>
class short_vec<int, 8>
{
public:
    static const int ARITY = 8;

    typedef short_vec_strategy::sse strategy;

    template<typename _CharT, typename _Traits>
    friend std::basic_ostream<_CharT, _Traits>& operator<<(
        std::basic_ostream<_CharT, _Traits>& __os,
        const short_vec<int, 8>& vec);

    inline
    short_vec(const int data = 0) :
        val1(_mm_set1_epi32(data)),
        val2(_mm_set1_epi32(data))
    {}

    inline
    short_vec(const int *data)
    {
        load(data);
    }

    inline
    short_vec(const __m128i& val1, const __m128i& val2) :
        val1(val1),
        val2(val2)
    {}

#ifdef LIBFLATARRAY_WITH_CPP14
    inline
    short_vec(const std::initializer_list<int>& il)
    {
        const int *ptr = static_cast<const int *>(&(*il.begin()));
        load(ptr);
    }
#endif

    inline
    short_vec(const sqrt_reference<int, 8>& other);

    inline
    void operator-=(const short_vec<int, 8>& other)
    {
        val1 = _mm_sub_epi32(val1, other.val1);
        val2 = _mm_sub_epi32(val2, other.val2);
    }

    inline
    short_vec<int, 8> operator-(const short_vec<int, 8>& other) const
    {
        return short_vec<int, 8>(
            _mm_sub_epi32(val1, other.val1),
            _mm_sub_epi32(val2, other.val2));
    }

    inline
    void operator+=(const short_vec<int, 8>& other)
    {
        val1 = _mm_add_epi32(val1, other.val1);
        val2 = _mm_add_epi32(val2, other.val2);
    }

    inline
    short_vec<int, 8> operator+(const short_vec<int, 8>& other) const
    {
        return short_vec<int, 8>(
            _mm_add_epi32(val1, other.val1),
            _mm_add_epi32(val2, other.val2));
    }

#ifdef __SSE4_1__
    inline
    void operator*=(const short_vec<int, 8>& other)
    {
        val1 = _mm_mullo_epi32(val1, other.val1);
        val2 = _mm_mullo_epi32(val2, other.val2);
    }

    inline
    short_vec<int, 8> operator*(const short_vec<int, 8>& other) const
    {
        return short_vec<int, 8>(
            _mm_mullo_epi32(val1, other.val1),
            _mm_mullo_epi32(val2, other.val2));
    }
#else
    inline
    void operator*=(const short_vec<int, 8>& other)
    {
        // see: https://software.intel.com/en-us/forums/intel-c-compiler/topic/288768
        __m128i tmp1 = _mm_mul_epu32(val1, other.val1);
        __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(val1, 4),
                                     _mm_srli_si128(other.val1, 4));
        val1 = _mm_unpacklo_epi32(
            _mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0,0,2,0)),
            _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0,0,2,0)));

        tmp1 = _mm_mul_epu32(val2, other.val2);
        tmp2 = _mm_mul_epu32(_mm_srli_si128(val2, 4),
                             _mm_srli_si128(other.val2, 4));
        val2 = _mm_unpacklo_epi32(
            _mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0,0,2,0)),
            _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0,0,2,0)));
    }

    inline
    short_vec<int, 8> operator*(const short_vec<int, 8>& other) const
    {
        // see: https://software.intel.com/en-us/forums/intel-c-compiler/topic/288768
        __m128i tmp1 = _mm_mul_epu32(val1, other.val1);
        __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(val1, 4),
                                     _mm_srli_si128(other.val1, 4));
        __m128i result1 = _mm_unpacklo_epi32(
            _mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0,0,2,0)),
            _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0,0,2,0)));

        tmp1 = _mm_mul_epu32(val2, other.val2);
        tmp2 = _mm_mul_epu32(_mm_srli_si128(val2, 4),
                             _mm_srli_si128(other.val2, 4));
        __m128i result2 = _mm_unpacklo_epi32(
            _mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0,0,2,0)),
            _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0,0,2,0)));

        return short_vec<int, 8>(result1, result2);
    }
#endif

    inline
    void operator/=(const short_vec<int, 8>& other)
    {
        val1 = _mm_cvtps_epi32(_mm_div_ps(_mm_cvtepi32_ps(val1),
                                          _mm_cvtepi32_ps(other.val1)));
        val2 = _mm_cvtps_epi32(_mm_div_ps(_mm_cvtepi32_ps(val2),
                                          _mm_cvtepi32_ps(other.val2)));
    }

    inline
    void operator/=(const sqrt_reference<int, 8>& other);

    inline
    short_vec<int, 8> operator/(const short_vec<int, 8>& other) const
    {
        return short_vec<int, 8>(
            _mm_cvtps_epi32(_mm_div_ps(_mm_cvtepi32_ps(val1),
                                       _mm_cvtepi32_ps(other.val1))),
            _mm_cvtps_epi32(_mm_div_ps(_mm_cvtepi32_ps(val2),
                                       _mm_cvtepi32_ps(other.val2))));
    }

    inline
    short_vec<int, 8> operator/(const sqrt_reference<int, 8>& other) const;

    inline
    short_vec<int, 8> sqrt() const
    {
        return short_vec<int, 8>(
            _mm_cvtps_epi32(
                _mm_sqrt_ps(_mm_cvtepi32_ps(val1))),
            _mm_cvtps_epi32(
                _mm_sqrt_ps(_mm_cvtepi32_ps(val2))));
    }

    inline
    void load(const int *data)
    {
        val1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(data + 0));
        val2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(data + 4));
    }

    inline
    void load_aligned(const int *data)
    {
        SHORTVEC_ASSERT_ALIGNED(data, 16);
        val1 = _mm_load_si128(reinterpret_cast<const __m128i *>(data + 0));
        val2 = _mm_load_si128(reinterpret_cast<const __m128i *>(data + 4));
    }

    inline
    void store(int *data) const
    {
        _mm_storeu_si128(reinterpret_cast<__m128i *>(data + 0), val1);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(data + 4), val2);
    }

    inline
    void store_aligned(int *data) const
    {
        SHORTVEC_ASSERT_ALIGNED(data, 16);
        _mm_store_si128(reinterpret_cast<__m128i *>(data + 0), val1);
        _mm_store_si128(reinterpret_cast<__m128i *>(data + 4), val2);
    }

    inline
    void store_nt(int *data) const
    {
        SHORTVEC_ASSERT_ALIGNED(data, 16);
        _mm_stream_si128(reinterpret_cast<__m128i *>(data + 0), val1);
        _mm_stream_si128(reinterpret_cast<__m128i *>(data + 4), val2);
    }

#ifdef __SSE4_1__
    inline
    void gather(const int *ptr, const int *offsets)
    {
        val1 = _mm_insert_epi32(val1, ptr[offsets[0]], 0);
        val1 = _mm_insert_epi32(val1, ptr[offsets[1]], 1);
        val1 = _mm_insert_epi32(val1, ptr[offsets[2]], 2);
        val1 = _mm_insert_epi32(val1, ptr[offsets[3]], 3);

        val2 = _mm_insert_epi32(val2, ptr[offsets[4]], 0);
        val2 = _mm_insert_epi32(val2, ptr[offsets[5]], 1);
        val2 = _mm_insert_epi32(val2, ptr[offsets[6]], 2);
        val2 = _mm_insert_epi32(val2, ptr[offsets[7]], 3);
    }

    inline
    void scatter(int *ptr, const int *offsets) const
    {
        ptr[offsets[0]] = _mm_extract_epi32(val1, 0);
        ptr[offsets[1]] = _mm_extract_epi32(val1, 1);
        ptr[offsets[2]] = _mm_extract_epi32(val1, 2);
        ptr[offsets[3]] = _mm_extract_epi32(val1, 3);

        ptr[offsets[4]] = _mm_extract_epi32(val2, 0);
        ptr[offsets[5]] = _mm_extract_epi32(val2, 1);
        ptr[offsets[6]] = _mm_extract_epi32(val2, 2);
        ptr[offsets[7]] = _mm_extract_epi32(val2, 3);
    }
#else
    inline
    void gather(const int *ptr, const int *offsets)
    {
        __m128i i2, i3, i4;
        val1 = _mm_cvtsi32_si128(ptr[offsets[0]]);
        i2   = _mm_cvtsi32_si128(ptr[offsets[1]]);
        i3   = _mm_cvtsi32_si128(ptr[offsets[2]]);
        i4   = _mm_cvtsi32_si128(ptr[offsets[3]]);
        val1 = _mm_unpacklo_epi32(val1, i3);
        i3   = _mm_unpacklo_epi32(i2  , i4);
        val1 = _mm_unpacklo_epi32(val1, i3);

        val2 = _mm_cvtsi32_si128(ptr[offsets[4]]);
        i2   = _mm_cvtsi32_si128(ptr[offsets[5]]);
        i3   = _mm_cvtsi32_si128(ptr[offsets[6]]);
        i4   = _mm_cvtsi32_si128(ptr[offsets[7]]);
        val2 = _mm_unpacklo_epi32(val2, i3);
        i3   = _mm_unpacklo_epi32(i2  , i4);
        val2 = _mm_unpacklo_epi32(val2, i3);
    }

    inline
    void scatter(int *ptr, const int *offsets) const
    {
        ptr[offsets[0]] = _mm_cvtsi128_si32(val1);
        ptr[offsets[1]] = _mm_cvtsi128_si32(_mm_shuffle_epi32(val1, _MM_SHUFFLE(0,3,2,1)));
        ptr[offsets[2]] = _mm_cvtsi128_si32(_mm_shuffle_epi32(val1, _MM_SHUFFLE(1,0,3,2)));
        ptr[offsets[3]] = _mm_cvtsi128_si32(_mm_shuffle_epi32(val1, _MM_SHUFFLE(2,1,0,3)));

        ptr[offsets[4]] = _mm_cvtsi128_si32(val2);
        ptr[offsets[5]] = _mm_cvtsi128_si32(_mm_shuffle_epi32(val2, _MM_SHUFFLE(0,3,2,1)));
        ptr[offsets[6]] = _mm_cvtsi128_si32(_mm_shuffle_epi32(val2, _MM_SHUFFLE(1,0,3,2)));
        ptr[offsets[7]] = _mm_cvtsi128_si32(_mm_shuffle_epi32(val2, _MM_SHUFFLE(2,1,0,3)));
    }
#endif

private:
    __m128i val1;
    __m128i val2;
};

inline
void operator<<(int *data, const short_vec<int, 8>& vec)
{
    vec.store(data);
}

template<>
class sqrt_reference<int, 8>
{
public:
    template<typename OTHER_CARGO, int OTHER_ARITY>
    friend class short_vec;

    sqrt_reference(const short_vec<int, 8>& vec) :
        vec(vec)
    {}

private:
    short_vec<int, 8> vec;
};

#ifdef __ICC
#pragma warning pop
#endif

inline
short_vec<int, 8>::short_vec(const sqrt_reference<int, 8>& other) :
    val1(
        _mm_cvtps_epi32(
            _mm_sqrt_ps(_mm_cvtepi32_ps(other.vec.val1)))),
    val2(
        _mm_cvtps_epi32(
            _mm_sqrt_ps(_mm_cvtepi32_ps(other.vec.val2))))
{}

inline
void short_vec<int, 8>::operator/=(const sqrt_reference<int, 8>& other)
{
    val1 = _mm_cvtps_epi32(
        _mm_mul_ps(_mm_cvtepi32_ps(val1),
                   _mm_rsqrt_ps(_mm_cvtepi32_ps(other.vec.val1))));
    val2 = _mm_cvtps_epi32(
        _mm_mul_ps(_mm_cvtepi32_ps(val2),
                   _mm_rsqrt_ps(_mm_cvtepi32_ps(other.vec.val2))));
}

inline
short_vec<int, 8> short_vec<int, 8>::operator/(const sqrt_reference<int, 8>& other) const
{
    return short_vec<int, 8>(
        _mm_cvtps_epi32(
            _mm_mul_ps(_mm_cvtepi32_ps(val1),
                       _mm_rsqrt_ps(_mm_cvtepi32_ps(other.vec.val1)))),
        _mm_cvtps_epi32(
            _mm_mul_ps(_mm_cvtepi32_ps(val2),
                       _mm_rsqrt_ps(_mm_cvtepi32_ps(other.vec.val2)))));
}

inline
sqrt_reference<int, 8> sqrt(const short_vec<int, 8>& vec)
{
    return sqrt_reference<int, 8>(vec);
}

template<typename _CharT, typename _Traits>
std::basic_ostream<_CharT, _Traits>&
operator<<(std::basic_ostream<_CharT, _Traits>& __os,
           const short_vec<int, 8>& vec)
{
    const int *data1 = reinterpret_cast<const int *>(&vec.val1);
    const int *data2 = reinterpret_cast<const int *>(&vec.val2);
    __os << "["
         << data1[0] << ", " << data1[1]  << ", " << data1[2]  << ", " << data1[3] << ", "
         << data2[0] << ", " << data2[1]  << ", " << data2[2]  << ", " << data2[3]
         << "]";
    return __os;
}

}

#endif
#endif
#endif

#endif
