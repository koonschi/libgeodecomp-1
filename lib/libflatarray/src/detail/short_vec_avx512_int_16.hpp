/**
 * Copyright 2015 Kurt Kanzenbach
 * Copyright 2016 Andreas Schäfer
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef FLAT_ARRAY_DETAIL_SHORT_VEC_AVX512_INT_16_HPP
#define FLAT_ARRAY_DETAIL_SHORT_VEC_AVX512_INT_16_HPP

#ifdef __AVX512F__

#include <immintrin.h>
#include <libflatarray/detail/sqrt_reference.hpp>
#include <libflatarray/detail/short_vec_helpers.hpp>
#include <libflatarray/config.h>
#include <iostream>

#ifdef LIBFLATARRAY_WITH_CPP14
#include <initializer_list>
#endif

#ifndef __CUDA_ARCH__

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
class short_vec<int, 16>
{
public:
    static const int ARITY = 16;

    typedef short_vec_strategy::avx512 strategy;

    template<typename _CharT, typename _Traits>
    friend std::basic_ostream<_CharT, _Traits>& operator<<(
        std::basic_ostream<_CharT, _Traits>& __os,
        const short_vec<int, 16>& vec);

    inline
    short_vec(const int data = 0) :
        val1(_mm512_set1_epi32(data))
    {}

    inline
    short_vec(const int *data)
    {
        load(data);
    }

    inline
    short_vec(const __m512i& val1) :
        val1(val1)
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
    short_vec(const sqrt_reference<int, 16>& other);

    inline
    void operator-=(const short_vec<int, 16>& other)
    {
        val1 = _mm512_sub_epi32(val1, other.val1);
    }

    inline
    short_vec<int, 16> operator-(const short_vec<int, 16>& other) const
    {
        return short_vec<int, 16>(
            _mm512_sub_epi32(val1, other.val1));
    }

    inline
    void operator+=(const short_vec<int, 16>& other)
    {
        val1 = _mm512_add_epi32(val1, other.val1);
    }

    inline
    short_vec<int, 16> operator+(const short_vec<int, 16>& other) const
    {
        return short_vec<int, 16>(
            _mm512_add_epi32(val1, other.val1));
    }

    inline
    void operator*=(const short_vec<int, 16>& other)
    {
        val1 = _mm512_mullo_epi32(val1, other.val1);
    }

    inline
    short_vec<int, 16> operator*(const short_vec<int, 16>& other) const
    {
        return short_vec<int, 16>(
            _mm512_mullo_epi32(val1, other.val1));
    }

    inline
    void operator/=(const short_vec<int, 16>& other)
    {
        val1 = _mm512_cvtps_epi32(_mm512_div_ps(_mm512_cvtepi32_ps(val1),
                                                _mm512_cvtepi32_ps(other.val1)));
    }

    inline
    void operator/=(const sqrt_reference<int, 16>& other);

    inline
    short_vec<int, 16> operator/(const short_vec<int, 16>& other) const
    {
        return short_vec<int, 16>(
            _mm512_cvtps_epi32(_mm512_div_ps(_mm512_cvtepi32_ps(val1),
                                             _mm512_cvtepi32_ps(other.val1))));
    }

    inline
    short_vec<int, 16> operator/(const sqrt_reference<int, 16>& other) const;

    inline
    short_vec<int, 16> sqrt() const
    {
        return short_vec<int, 16>(
            _mm512_cvtps_epi32(
                _mm512_sqrt_ps(_mm512_cvtepi32_ps(val1))));
    }

    inline
    void load(const int *data)
    {
        val1 = _mm512_loadu_si512(data);
    }

    inline
    void load_aligned(const int *data)
    {
        SHORTVEC_ASSERT_ALIGNED(data, 64);
        val1 = _mm512_load_epi32(data);
    }

    inline
    void store(int *data) const
    {
        _mm512_storeu_si512(data, val1);
    }

    inline
    void store_aligned(int *data) const
    {
        SHORTVEC_ASSERT_ALIGNED(data, 64);
        _mm512_store_epi32(data, val1);
    }

    inline
    void store_nt(int *data) const
    {
        SHORTVEC_ASSERT_ALIGNED(data, 64);
        _mm512_stream_si512(reinterpret_cast<__m512i *>(data), val1);
    }

    inline
    void gather(const int *ptr, const int *offsets)
    {
        __m512i indices = _mm512_loadu_si512(offsets);
        val1 = _mm512_i32gather_epi32(indices, ptr, 4);
    }

    inline
    void scatter(int *ptr, const int *offsets) const
    {
        __m512i indices = _mm512_loadu_si512(offsets);
        _mm512_i32scatter_epi32(ptr, indices, val1, 4);
    }

private:
    __m512i val1;
};

inline
void operator<<(int *data, const short_vec<int, 16>& vec)
{
    vec.store(data);
}

template<>
class sqrt_reference<int, 16>
{
public:
    template<typename OTHER_CARGO, int OTHER_ARITY>
    friend class short_vec;

    sqrt_reference(const short_vec<int, 16>& vec) :
        vec(vec)
    {}

private:
    short_vec<int, 16> vec;
};

#ifdef __ICC
#pragma warning pop
#endif

inline
short_vec<int, 16>::short_vec(const sqrt_reference<int, 16>& other) :
    val1(
        _mm512_cvtps_epi32(
            _mm512_sqrt_ps(_mm512_cvtepi32_ps(other.vec.val1))))
{}

inline
void short_vec<int, 16>::operator/=(const sqrt_reference<int, 16>& other)
{
    val1 = _mm512_cvtps_epi32(
        _mm512_mul_ps(_mm512_cvtepi32_ps(val1),
                      _mm512_rsqrt14_ps(_mm512_cvtepi32_ps(other.vec.val1))));
}

inline
short_vec<int, 16> short_vec<int, 16>::operator/(const sqrt_reference<int, 16>& other) const
{
    return short_vec<int, 16>(
        _mm512_cvtps_epi32(
            _mm512_mul_ps(_mm512_cvtepi32_ps(val1),
                          _mm512_rsqrt14_ps(_mm512_cvtepi32_ps(other.vec.val1)))));
}

inline
sqrt_reference<int, 16> sqrt(const short_vec<int, 16>& vec)
{
    return sqrt_reference<int, 16>(vec);
}

template<typename _CharT, typename _Traits>
std::basic_ostream<_CharT, _Traits>&
operator<<(std::basic_ostream<_CharT, _Traits>& __os,
           const short_vec<int, 16>& vec)
{
    const int *data1 = reinterpret_cast<const int *>(&vec.val1);
    __os << "["
         << data1[ 0] << ", " << data1[ 1]  << ", " << data1[ 2]  << ", " << data1[ 3] << ", "
         << data1[ 4] << ", " << data1[ 5]  << ", " << data1[ 6]  << ", " << data1[ 7] << ", "
         << data1[ 8] << ", " << data1[ 9]  << ", " << data1[10]  << ", " << data1[11] << ", "
         << data1[12] << ", " << data1[13]  << ", " << data1[14]  << ", " << data1[15]
         << "]";
    return __os;
}

}

#endif
#endif

#endif
