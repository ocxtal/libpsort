
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>
#include <pthread.h>

long add_num(long **, long);
void qsortl(long *, size_t);
void qsortl_serial(long *, size_t);
void tsqsortl(long *, size_t);
void tsqsortl_serial(long *, size_t);
long get_us(void);

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
	printf("written in C with normal quicksort implementation\n");

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
	qsortl(arr, len);
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

void tsqsortl(long *base, size_t len)
{
	tsqsortl_serial(base, len);
	return;
}

void inline swapl(void *a, void *b)
{
	long t;
	t = *(long*)a; *(long*)a = *(long*)b; *(long*)b = t;
	return;
}

long inline compl(long const *a, long const *b)
{
	return(*(long*)a - *(long*)b);
}

long *med3(long *a, long *b, long *c)
{
	return(compl(a, b) > 0
		? compl(b, c) > 0 ? b : compl(c, a) > 0 ? a : c
		: compl(a, c) > 0 ? a : compl(b, c) > 0 ? c : b);
}

long *selectpivot(long *arr, long len)
{
	long q, d;
	q = len/4;
	d = len/16;
	if(len < 128) {
		return(med3(arr, arr+q+q, arr+len-1));
	} else if(len < 1024) {
		return(med3(med3(arr, arr+d+d, arr+q), arr+q+q, arr+len-1));
	} else {
		return(med3(med3(arr, arr+d, arr+d+d),
			med3(arr+q-d, arr+q, arr+q+d),
			med3(arr+len-d-d, arr+len-d, arr+len-1)));
	}
}

void tsqsortl_serial(long *base, size_t len)
{
	long i, j;
	long pivot, *l, *ll, *r, *rr;
	long tmp, res;

	if(len <= 64) {
		/* insertion sort */
		for(i = 1; i < len; i++) {
		  	tmp = base[i];
			if(base[i-1] > tmp) {
				j = i;
				do {
					base[j] = base[j-1];
					--j;
				} while (j > 0 && base[j-1] > tmp);
				base[j] = tmp;
			}
		}
	} else {
		/* quicksort */
//		pivot = *med3(base, base+(len/2), base+len-1);
		pivot = *selectpivot(base, len);
//		printf("pivot = %ld\n", pivot);
//		printf("before "); for(i = 0; i < len; i++) { printf("%ld, ", base[i]); } printf("\n");
		l = ll = base;
		r = rr = base+len-1; 
		while(1) {
			while(l <= r && (res = *l - pivot) <= 0) {
				if(res == 0) { l++; }
				l++; ll++;
			}
			while(l <= r && (res = *r - pivot) >= 0) {
				if(res == 0) { r--; }
				r--; rr--;
			}
			/* 終了条件の判定 */
			if(l > r) {
				break;
			}
			tmp = *r--;
			*rr-- = *l++;
			*ll++ = tmp;
		}
		for(; l < r; l++) {
			*l = pivot;
		}
//		printf("after "); for(i = 0; i < len; i++) { printf("%ld, ", base[i]); } printf("\n");
//		printf("divide into %ld and %ld\n", ll-base, base+len-rr);
		tsqsortl_serial(base, ll-base);
		tsqsortl_serial(rr, base+len-rr);
	}
	return;
}

void qsortl(long *base, size_t len)
{
	qsortl_serial(base, len);
	return;
}

void qsortl_serial(long *base, size_t len)
{
	long i, j;
	long *pivot, *l, *la, *r, *ra;
	long tmp;

	if(len <= 64) {
		/* insertion sort */
		for(i = 1; i < len; i++) {
		  	tmp = base[i];
			if(base[i-1] > tmp) {
				j = i;
				do {
					base[j] = base[j-1];
					--j;
				} while (j > 0 && base[j-1] > tmp);
				base[j] = tmp;
			}
		}
	} else {
		/* quicksort */
//		pivot = med3(base, base+(len/2), base+len-1);
//		printf("selectpivot\n");
		pivot = selectpivot(base, len);
//		printf("pivot = %ld\n", *pivot);
//		printf("before "); for(i = 0; i < len; i++) { printf("%ld, ", base[i]); } printf("\n");
		swapl(base, pivot);
		l = la = base; r = ra = base+len;
		l++; r--;
		while(1) {
			while(l <= r && compl(l, base) <= 0) { l++; }
			while(l <= r && compl(r, base) > 0) { r--; }
			if(l > r) { break; }
			swapl(l++, r--);
		}
		swapl(base, l-1);
//		printf("after "); for(i = 0; i < len; i++) { printf("%ld, ", base[i]); } printf("\n");
//		printf("divide into %ld and %ld\n", l-la, ra-l);
		qsortl_serial(base, l-la);
		qsortl_serial(base+(l-la), ra-l);
	}
	return;
}

long get_us(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return((long)tv.tv_sec * 1000000 + (long)tv.tv_usec);
}
