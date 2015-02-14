
/**
 * @file wcradix.c
 *
 * @brief write-combining radix sort implementation.
 *
 * @detail
 * 
 *
 */
#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <smmintrin.h>
#define HAVE_SSE4				1

#define BFR_OCC_SIZE			( 1<<8 )	/** 8bit */
#define BFR_BUF_SIZE			( 64 )		/** 64 elements */

/**
 * default variable types
 */
struct elem {
	unsigned long key;
	unsigned long val;
};
#define elem_t					struct elem
#define ulong_t 				unsigned long
#define uchar_t 				unsigned char 

#if 0
/**
 * if you want to use the function with a type smaller than __m128i, uncomment the defines below.
 */
#define READ(ptr)				( *(ptr) )
#define WRITE(ptr, key)			{ *(ptr) = key; }
#define EXTRACT(key, pos)		( (key)>>(pos) )
#endif

#define READ(p)				( *(p) )
#define WRITE(p, k)			{ *(p) = (k); }
#define EXTRACT(k, p)		( ((k).key)>>((p)*8) & (BFR_OCC_SIZE-1) )

/**
 * @fn aligned_malloc
 *
 * @brief an wrapper of posix_memalign function
 */
void *aligned_malloc(size_t size, size_t align)
{
	void *ptr;
	posix_memalign(&ptr, align, size);
	return(ptr);
}

/**
 * @fn wcradiximpl
 *
 * @brief internal implementation of write-combining radix sort
 *
 * @param[in] arr : a pointer to the array to be sorted.
 * @param[in] len : the length of the array (arr).
 * @param[in] from : the digit to be sorted from.
 * @param[in] to : the digit to be sorted to.
 */
int wcradiximpl(elem_t *arr, ulong_t len, int from, int to)
{
	#ifdef _OPENMP
		int threads = 4; //omp_get_num_threads();
	#else
		int threads = 1;
	#endif
	size_t arr_size = sizeof(elem_t)  * len,
		   wb_size  = sizeof(elem_t)  * threads * BFR_OCC_SIZE * BFR_BUF_SIZE,
		   nwb_size = sizeof(uchar_t) * threads * BFR_OCC_SIZE,
		   occ_size = sizeof(ulong_t) * threads * BFR_OCC_SIZE;
	int mem_size = arr_size + wb_size + nwb_size + occ_size;
	elem_t *src, *dest,
		   (*wb)[BFR_OCC_SIZE][BFR_BUF_SIZE];
	uchar_t (*nwb)[BFR_OCC_SIZE];
	ulong_t (*occ)[BFR_OCC_SIZE];
	void *ptr = NULL, *swap;

	if(from >= to) { return 1; }
	if((ptr = (void *)aligned_malloc(mem_size, 16)) == NULL) {
		return 1;
	}
	src = (elem_t *)ptr;		/** will be swaped in at the beginning of the for loop */
	dest = (elem_t *)arr;
	wb = (elem_t (*)[BFR_OCC_SIZE][BFR_BUF_SIZE])((void *)src     + arr_size);
	nwb = (uchar_t (*)[BFR_OCC_SIZE])            ((void *)wb      + wb_size);
	occ = (ulong_t (*)[BFR_OCC_SIZE])            ((void *)nwb     + nwb_size);

#pragma omp parallel num_threads(threads)
	{
		#ifdef _OPENMP
			int core  = omp_get_thread_num();
		#else
			int core = 0;
		#endif
		int i, j, d;
		int begin = (len * core) / threads,
			end   = (core == threads-1) ? len : (len * (core+1)) / threads;
		ulong_t sum, tmp, n, p;
		elem_t key;

		for(d = from; d < to; d++) {
			memset(&occ[core], 0, occ_size / threads);
			for(i = begin; i < end; i++) {
				occ[core][EXTRACT(READ(dest + i), d)]++;
			}
			#pragma omp barrier		/** join */
			#pragma omp single		/** executed on one core */
			{
				swap = src; src = dest; dest = swap;
				for(sum = 0, i = 0; i < BFR_OCC_SIZE; i++) {
					for(j = 0; j < threads; j++) {
						tmp = occ[j][i]; occ[j][i] = sum; sum += tmp;
					}
				}
			}
			#pragma omp barrier		/** join */
			/** sort the n-th digit; parallel section */
			memset(&nwb[core], 0, nwb_size / threads);
			for(i = begin; i < end; i++) {
				key = READ(src + i); n = EXTRACT(key, d);
				WRITE(wb[core][n] + (size_t)nwb[core][n], key);
				/** check if flush is needed */
				if(++nwb[core][n] == BFR_BUF_SIZE) {
					for(p = occ[core][n], j = 0; j < BFR_BUF_SIZE; p++, j++) {
						WRITE(dest + p, READ(wb[core][n] + j));
					}
					occ[core][n] = p; nwb[core][n] = 0;	/** clear write buffer */
				}
			}
			/** flush the remaining content */
			for(i = 0; i < BFR_OCC_SIZE; i++) {
				for(p = occ[core][i], j = 0; j < nwb[core][i]; p++, j++) {
					WRITE(dest + p, READ(wb[core][i] + j));
				}
			}
			#pragma omp barrier		/** join */
		}
		/** copy back to the source buffer if needed */
		if((to - from) & 0x01) {
			/** if to - from is odd, copy back the dest buffer to source array */
			for(i = begin; i < end; i++) {
				WRITE(arr + i, READ(dest + i));
			}
		}
		#pragma omp barrier			/** join */
	}

	/** clean up buffers */
	free(ptr);
	return 0;
}

/**
 * @fn wcradix
 *
 * @brief an interface to integer sort function.
 */
int wcradix(elem_t *arr, ulong_t len)
{
	return(wcradiximpl(arr, len, 0, sizeof(ulong_t)));
}

#ifdef TEST

#include <stdio.h>
#include <sys/time.h>

void test_0(void)
{
	int i;
	elem_t arr[] =    {{1, 0}, {0, 0}, {2, 0}, {1, 0}, {0, 0}, {2, 0}, {0, 0}, {0, 0}, {1, 0}, {1, 0}};
	elem_t sorted[] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {1, 0}, {1, 0}, {1, 0}, {1, 0}, {2, 0}, {2, 0}};
	wcradix(arr, sizeof(arr)/sizeof(elem_t));
	for(i = 0; i < sizeof(arr)/sizeof(elem_t); i++) {
		assert(arr[i].key == sorted[i].key);
	}
	return;
}

void test_1(void)
{
	int i;
	elem_t arr[] =    {{1000000, 0}, {0, 0}, {2000000, 0}, {1000000, 0}, {0, 0}, {2000000, 0}, {0, 0}, {0, 0}, {1000000, 0}, {1000000, 0}};
	elem_t sorted[] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {1000000, 0}, {1000000, 0}, {1000000, 0}, {1000000, 0}, {2000000, 0}, {2000000, 0}};
	wcradix(arr, sizeof(arr)/sizeof(elem_t));
	for(i = 0; i < sizeof(arr)/sizeof(elem_t); i++) {
		assert(arr[i].key == sorted[i].key);
	}
	return;
}

void test_2(void)
{
	int i;
	int const len = 100000;
	elem_t *arr, e;
	arr = aligned_malloc(sizeof(elem_t) * len, 128);
	for(i = 0; i < len; i++) { e.key = len - i; WRITE(arr + i, e); }
	wcradix(arr, len);
	for(i = 1; i < len; i++) {
		assert(arr[i-1].key < arr[i].key);
	}
	free(arr);
	return;
}

void bench(void)
{
	int i;
	int const len = 10000;
	elem_t *arr, e;
	struct timeval ts, te;
	arr = aligned_malloc(sizeof(elem_t) * len, 128);
	for(i = 0; i < len; i++) { e.key = len - i; WRITE(arr + i, e); }
	gettimeofday(&ts, NULL);
	wcradix(arr, len);
	gettimeofday(&te, NULL);
	fprintf(stderr, "%lu us\n", (te.tv_sec - ts.tv_sec) * 1000000 + (te.tv_usec - ts.tv_usec));
	free(arr);
	return;
}

int main(void)
{
	int i;
	test_0();
	test_1();
	test_2();
	for(i = 0; i < 3; i++) {
		bench();
	}
	return 0;
}

#endif /* #ifdef TEST */
/**
 * end of wcradix.c
 */
