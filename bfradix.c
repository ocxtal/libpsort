
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define OCC_SIZE	(256)
#define BUF_SIZE	(64)
#define _USE_BUFFERING (1)
#define _USE_MAIN
#define _USE_DEBUG

#ifdef _USE_DEBUG
#include <stdio.h>
#endif /* _USE_DEBUG */

#ifdef _USE_MAIN
#include <stdio.h>
#include <sys/time.h>
#endif /* _USE_MAIN */

typedef union _byte8 {
	unsigned char _c[8];
	unsigned short _s[4];
	unsigned int _i[2];
	unsigned long _l;
} byte8;

#ifdef _USE_DEBUG

void print_arr(char const *str, long *arr, long len)
{
	long i;
	printf("arr(%s):\n", str);
	for(i = 0; i < len; i++) {
		printf("%lx\n", arr[i]);
	}
	return;
}

void print_occ(char const *str, long (*occ)[OCC_SIZE], long size)
{
	long i, j;
	printf("occ(%s)\n", str);
	for(j = 0; j < size; j++) {
		for(i = 0; i < OCC_SIZE; i++) {
			printf("%ld, ", occ[j][i]);
		}
		printf("\n");
	}
	printf("\n");
	return;
}

#endif /* _USE_DEBUG */

void
count_occ(
	long *arr,
	long len,
	long (*occ)[OCC_SIZE],
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

void
calc_start_point(
	long (*occ)[OCC_SIZE],
	int nth
	)
{
	long i, j, sum, temp_l;
	sum = 0;
	for(i = 0; i < OCC_SIZE; i++) {
		for(j = 0; j < nth; j++) {
			temp_l = occ[j][i];
			occ[j][i] = sum;
			sum += temp_l;
		}
	}
//	print_occ("after calc sum", occ, nth);
	return;
}

void
parallel_sort(
	long *dest,						/* 移動先 */
	long *src,						/* 移動元 */
	long len,						/* 要素数 */
	long (*occ)[OCC_SIZE],			/* hisotgram */
	long (*buf)[OCC_SIZE][BUF_SIZE],	/* 出力バッファ */
	short (*buf_cnt)[OCC_SIZE],		/* 出力バッファの要素数 */	
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
/*		for(i = 0; i < OCC_SIZE; i++) {
			buf_cnt[n][i][0] = buf_cnt[n][i][1] = occ[n][i] & 0x3f;
		}*/
		for(i = start; i < end; i++) {
			#ifdef _USE_BUFFERING		
			bin = ((byte8 *)src)[i]._c[depth];
			tmp = buf[n][bin][(size_t)buf_cnt[n][bin]++] = src[i];
			if(buf_cnt[n][bin] == BUF_SIZE) {	/* flush */
				tmp = occ[n][bin];
				for(j = 0; j < BUF_SIZE; j++) {
					dest[tmp++] = buf[n][bin][j];
				}
				occ[n][bin] = tmp;
				buf_cnt[n][bin] = 0;
			}
			#else
			dest[occ[n][(size_t)((byte8 *)src)[i]._c[depth]]++] = src[i];
			#endif
		}
		#ifdef _USE_BUFFERING
		for(i = 0; i < OCC_SIZE; i++) {
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

void inline swapp(void **a, void **b)
{
	void *temp_ptr;
	temp_ptr = *a;
	*a = *b;
	*b = temp_ptr;
	return;
}

void *aligned_malloc(size_t size, size_t align)
{
	void *ptr;
	posix_memalign(&ptr, align, size);
	return(ptr);
}

/* count_occとsortでOMP並列化をつかうバージョン */
int parallel_radixsort(long *arr, long len)
{
	int i, threads;
	long *sarr[2], *work;
	long (*occ)[OCC_SIZE];//, (*occ_next)[OCC_SIZE];
	long (*buf)[OCC_SIZE][BUF_SIZE];
	short (*buf_cnt)[OCC_SIZE];
	#ifdef _OPENMP
	threads = omp_get_num_procs();
	#else
	threads = 1;
	#endif

	work = (long *)malloc(sizeof(long) * len);
	sarr[0] = arr;
	sarr[1] = work;
	occ = (long (*)[OCC_SIZE])aligned_malloc(sizeof(long) * threads * OCC_SIZE, sizeof(long) * threads * OCC_SIZE);
	buf = (long (*)[OCC_SIZE][BUF_SIZE])aligned_malloc(sizeof(long) * threads * OCC_SIZE * BUF_SIZE, sizeof(long) * threads * OCC_SIZE);
	buf_cnt = (short (*)[OCC_SIZE])aligned_malloc(sizeof(short) * threads * OCC_SIZE, sizeof(long) * threads * OCC_SIZE);

	if(sarr[1] == NULL || occ == NULL) { return -1; }
	for(i = 0; i < (int)sizeof(long); i++) {
		memset(occ, 0, sizeof(long) * threads * OCC_SIZE);
		memset(buf_cnt, 0, sizeof(short) * threads * OCC_SIZE);		
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

#ifdef _USE_MAIN

long get_us(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return((long)tv.tv_sec * 1000000 + (long)tv.tv_usec);
}

long add_num(long **parr, long num)
{
	static long len = 0, buf_len = 0;

	if(buf_len == 0) {
		/* 最初の呼び出し */
		buf_len = 256;
		*parr = (long *)malloc(buf_len * sizeof(long));
	}
	if(++len >= buf_len) {
		buf_len *= 2;
		*parr = (long *)realloc(*parr, buf_len * sizeof(long));
	}
	if(*parr == NULL) {
		len = 0;
		buf_len = 0;
		return -1;
	} else {
		(*parr)[len] = num;
		return len;
	}
}

void init(long *arr, long len)
{
	long i;
	for(i = 0; i < len; i++) {
		arr[i] = ((long)rand()<<31) + (long)rand();
	}
	return;
}

int main(int argc, char *argv[])
{
	long *arr = NULL;
	long len = 0, i;
	long start, end;
	size_t buf_len = 100;
	char *buf;
	FILE *fin;
	int fail = 0;

	if(argc != 2) {
		printf("wrong number of arguments\n");
		exit(1);
	}
	#ifdef _USE_BUFFERING
	printf("written in C with parallel radixsort implementation (with bufferd scatter)\n");
	#else
	printf("written in C with parallel radixsort implementation\n");
	#endif

	if((fin = fopen(argv[1], "r")) == NULL) {
		len = atol(argv[1]);
		printf("length: %ld\n", len);
		start = get_us();
		arr = (long *)malloc(len * sizeof(long));
		init(arr, len);
		end = get_us();
		printf("elapsed time - fill array: %ld us\n", end - start);
	} else {
		start = get_us();
		buf = (char *)malloc(buf_len * sizeof(char));
		while(fgets(buf, buf_len, fin)) {
			len = add_num(&arr, (long)atol(buf));
			if(len < 0){
				printf("malloc failed\n");
				exit(1);
			}
		}
		free(buf);
		end = get_us();
		fclose(fin);
		printf("elapsed time - file read: %ld us\n", end - start);
	}
	start = get_us();
	parallel_radixsort(arr, len);
	end = get_us();
	printf("elapsed time - sort: %ld us\n", end - start);

	for(i = 1; i < len; i++) {
		if(arr[i-1] > arr[i]) {
			fail = 1;
		}
	}
	printf("%s\n", fail ? "sort failed" : "sort succeeded");
	free(arr);
	return 0;
}

#endif /* _USE_MAIN */

