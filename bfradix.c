
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define BFR_OCC_SIZE		(256)
#define BFR_BUF_SIZE		(64)
#define BFR_USE_BUFFERING	(1)


#ifdef _USE_DEBUG
#include <stdio.h>
#endif /* _USE_DEBUG */


typedef union _byte8 {
	unsigned char _c[8];
	unsigned short _s[4];
	unsigned int _i[2];
	unsigned long _l;
} byte8;

#ifdef _USE_DEBUG

void static
print_arr(char const *str, long *arr, long len)
{
	long i;
	printf("arr(%s):\n", str);
	for(i = 0; i < len; i++) {
		printf("%lx\n", arr[i]);
	}
	return;
}

void static
print_occ(char const *str, long (*occ)[BFR_OCC_SIZE], long size)
{
	long i, j;
	printf("occ(%s)\n", str);
	for(j = 0; j < size; j++) {
		for(i = 0; i < BFR_OCC_SIZE; i++) {
			printf("%ld, ", occ[j][i]);
		}
		printf("\n");
	}
	printf("\n");
	return;
}

#endif /* _USE_DEBUG */

void static
count_occ(
	long *arr,
	long len,
	long (*occ)[BFR_OCC_SIZE],
	int nth,
	int depth
	)
{
	long i, start, end;
	int n;
#ifdef _OPENMP
#pragma omp parallel private(i, start, end, n) num_threads(nth)
#endif
	{
		#ifdef _OPENMP
		n = omp_get_thread_num();
		#else
		n = 0;
		#endif
		start = len/nth * n;
		if(n == (nth-1)) {
			end = len;
		} else {
			end = len/nth * (n+1);
		}
//		printf("from %ld to %ld\n", start, end);
		for(i = start; i < end; i++) {
			occ[n][(size_t)((byte8 *)arr)[i]._c[depth]]++;
		}
	}
//	print_occ("after counting", occ, nth);
	return;
}

void static
calc_start_point(
	long (*occ)[BFR_OCC_SIZE],
	int nth
	)
{
	long i, j, sum, temp_l;
	sum = 0;
	for(i = 0; i < BFR_OCC_SIZE; i++) {
		for(j = 0; j < nth; j++) {
			temp_l = occ[j][i];
			occ[j][i] = sum;
			sum += temp_l;
		}
	}
//	print_occ("after calc sum", occ, nth);
	return;
}

void static
parallel_sort(
	long *dest,						/* 移動先 */
	long *src,						/* 移動元 */
	long len,						/* 要素数 */
	long (*occ)[BFR_OCC_SIZE],			/* hisotgram */
	long (*buf)[BFR_OCC_SIZE][BFR_BUF_SIZE],	/* 出力バッファ */
	short (*buf_cnt)[BFR_OCC_SIZE],		/* 出力バッファの要素数 */	
	int nth,						/* number of thread */
	int depth						/* iteration count */
	)
{
	long i, j, start, end;
	int n;
	long bin, tmp;
	
#ifdef _OPENMP
#pragma omp parallel private(i, j, start, end, n, bin, tmp) num_threads(nth)
#endif
	{
		#ifdef _OPENMP
		n = omp_get_thread_num();
		#else
		n = 0;
		#endif
		start = len/nth * n;
		if(n == (nth-1)) {
			end = len;
		} else {
			end = len/nth * (n+1);
		}
//		printf("from %ld to %ld\n", start, end);
//		printf("%ld\n", buf_cnt[n][0]);
/*		for(i = 0; i < BFR_OCC_SIZE; i++) {
			buf_cnt[n][i][0] = buf_cnt[n][i][1] = occ[n][i] & 0x3f;
		}*/
		for(i = start; i < end; i++) {
			#ifdef BFR_USE_BUFFERING		
			bin = ((byte8 *)src)[i]._c[depth];
			tmp = buf[n][bin][(size_t)buf_cnt[n][bin]++] = src[i];
			if(buf_cnt[n][bin] == BFR_BUF_SIZE) {	/* flush */
				tmp = occ[n][bin];
				for(j = 0; j < BFR_BUF_SIZE; j++) {
					dest[tmp++] = buf[n][bin][j];
				}
				occ[n][bin] = tmp;
				buf_cnt[n][bin] = 0;
			}
			#else
			dest[occ[n][(size_t)((byte8 *)src)[i]._c[depth]]++] = src[i];
			#endif
		}
		#ifdef BFR_USE_BUFFERING
		for(i = 0; i < BFR_OCC_SIZE; i++) {
			if(buf_cnt[n][i] != 0) {
//				memcpy(&dest[occ[n][i]], &buf[n][i][j], buf_cnt[n][i]*sizeof(long));
				tmp = occ[n][i];
				for(j = 0; j < buf_cnt[n][i]; j++) {
					dest[tmp++] = buf[n][i][j];
				}
			}
		}
		#endif
	}
	return;
}

void static inline
swapp(void **a, void **b)
{
	void *temp_ptr;
	temp_ptr = *a;
	*a = *b;
	*b = temp_ptr;
	return;
}

void static
*aligned_malloc(size_t size, size_t align)
{
	void *ptr;
	posix_memalign(&ptr, align, size);
	return(ptr);
}

/* count_occとsortでOMP並列化をつかうバージョン */
int
bfradix(long *arr, long len)
{
	int i, threads;
	long *sarr[2], *work;
	long (*occ)[BFR_OCC_SIZE];//, (*occ_next)[BFR_OCC_SIZE];
	long (*buf)[BFR_OCC_SIZE][BFR_BUF_SIZE];
	short (*buf_cnt)[BFR_OCC_SIZE];
	#ifdef _OPENMP
	threads = omp_get_num_procs();
	#else
	threads = 1;
	#endif

	work = (long *)malloc(sizeof(long) * len);
	sarr[0] = arr;
	sarr[1] = work;
	occ = (long (*)[BFR_OCC_SIZE])aligned_malloc(sizeof(long) * threads * BFR_OCC_SIZE, sizeof(long) * threads * BFR_OCC_SIZE);
	buf = (long (*)[BFR_OCC_SIZE][BFR_BUF_SIZE])aligned_malloc(sizeof(long) * threads * BFR_OCC_SIZE * BFR_BUF_SIZE, sizeof(long) * threads * BFR_OCC_SIZE);
	buf_cnt = (short (*)[BFR_OCC_SIZE])aligned_malloc(sizeof(short) * threads * BFR_OCC_SIZE, sizeof(long) * threads * BFR_OCC_SIZE);

	if(sarr[1] == NULL || occ == NULL) { return -1; }
	for(i = 0; i < (int)sizeof(long); i++) {
		memset(occ, 0, sizeof(long) * threads * BFR_OCC_SIZE);
		memset(buf_cnt, 0, sizeof(short) * threads * BFR_OCC_SIZE);		
		count_occ(sarr[0], len, occ, threads, 0);
		calc_start_point(occ, threads);
		parallel_sort(sarr[1], sarr[0], len, occ, /*occ_next,*/ buf, buf_cnt, threads, i);
		swapp((void **)&sarr[0], (void **)&sarr[1]);
	}
	free(work);
	free(occ);
	free(buf);
	free(buf_cnt);
	return 0;
}
