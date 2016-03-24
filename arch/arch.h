
/**
 * @fn util.h
 *
 * @brief architecture-dependent utils selector
 */
#ifndef _UTIL_H_INCLUDED
#define _UTIL_H_INCLUDED

#include <stdint.h>

#ifdef __x86_64__
#  include "x86_64/arch_util.h"
#endif

#ifdef AARCH64
#endif

#ifdef PPC64
#endif

#ifndef _ARCH_UTIL_H_INCLUDED
#  error "No SIMD environment detected. Check CFLAGS."
#endif

/* elem_t and move definitions */
#define _rd(p)				( *(p) )
#define _wr(p, k)			{ *(p) = (k); }
#define _ex(k, p)			( ((k)>>((p)*8)) & (WCR_OCC_SIZE-1) )
#define _p(v)				( (elem_t)(v) )
#define _e(v)				( (uint64_t)(v) )

#endif /* _UTIL_H_INCLUDED */
/**
 * end of util.h
 */
