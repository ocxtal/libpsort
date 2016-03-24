
/**
 * @file arch_util.h
 */
#ifndef _ARCH_UTIL_H_INCLUDED
#define _ARCH_UTIL_H_INCLUDED

#include <emmintrin.h>		/* SSE2 */
#include <smmintrin.h>		/* SSE4.1 */

/* cache line operation */
#define WCR_BUF_SIZE		( 64 )		/** 64 bytes */
#define memcpy_buf(_dst, _src) { \
	__m128i register *_s = (__m128i *)(_src); \
	__m128i register *_d = (__m128i *)(_dst); \
	__m128i xmm0 = _mm_load_si128(_s); \
	_mm_stream_si128(_d, xmm0); \
	__m128i xmm1 = _mm_load_si128(_s + 1); \
	_mm_stream_si128(_d + 1, xmm1); \
	__m128i xmm2 = _mm_load_si128(_s + 2); \
	_mm_stream_si128(_d + 2, xmm2); \
	__m128i xmm3 = _mm_load_si128(_s + 3); \
	_mm_stream_si128(_d + 3, xmm3); \
}

/* 128bit register operation */
#define elem_128_t			__m128i
#define rd_128(_ptr)		( _mm_load_si128((__m128i *)(_ptr)) )
#define wr_128(_ptr, _e)	{ _mm_store_si128((__m128i *)(_ptr), (_e)); }
#define _ex_128(k, h)		( _mm_extract_epi64((elem_128_t)k, h) )
#define ex_128(k, p)		( (((p)>>3 ? _ex_128(k, 1) : _ex_128(k, 0))>>(((p) & 0x07)<<3)) & (WCR_OCC_SIZE-1) )
#define p_128(v)			( _mm_cvtsi64_si128((uint64_t)(v)) )
#define e_128(v)			( (uint64_t)_mm_cvtsi128_si64((__m128i)(v)) )

#endif /* _ARCH_UTIL_H_INCLUDED */
/**
 * end of arch_util.h
 */
