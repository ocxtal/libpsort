

#include <stdio.h>
#include <time.h>


extern int bfradix(long *arr, long len);
extern int tsquick(long *arr, long len);


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

int test(long len, int (*func)(long *, long))
{
	long *arr = NULL;
	long i;
	long start, end;
	int fail = 0;

	#ifdef BFR_USE_BUFFERING
	fprintf(stderr, "written in C with parallel radixsort implementation (with bufferd scatter)\n");
	#else
	fprintf(stderr, "written in C with parallel radixsort implementation\n");
	#endif

	len = atol(argv[1]);
	fprintf(stderr, "length: %ld\n", len);
	start = get_us();
	arr = (long *)malloc(len * sizeof(long));
	init(arr, len);
	end = get_us();
	fprintf(stderr, "elapsed time - fill array: %ld us\n", end - start);

	start = get_us();
	parallel_radixsort(arr, len);
	end = get_us();
	fprintf(stderr, "elapsed time - sort: %ld us\n", end - start);

	for(i = 1; i < len; i++) {
		if(arr[i-1] > arr[i]) {
			fail = 1;
		}
	}
	fprintf(stderr, "%s\n", fail ? "sort failed" : "sort succeeded");
	free(arr);

	return(fail == 1 ? 1 : 0);
}

int main(int argc, char *argv[])
{

	if(argc != 2) {
		printf("usage: ./main <len>\n");
		exit(1);
	}

	return 0;
}

