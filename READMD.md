# libpsort

Libpsort is a fast and lightweight pthread-based parallel integer / string sort library written in pure C99. It does not depend on OpenMP or any other non-posix frameworks. The library is developed as a submodule of the comb aligner.

## Build

Python (2.7 or 3.x) is required to run the build script written in [waf](https://github.com/waf-project/waf).

## Functions

### psort_full

Sort integers, returns 0 if succeeded, -1 otherwise. `elem_size` must be the size of the elements (`sizeof(elem_t)`). If `num_threads` is zero, the function runs on a single thread.

```
int psort_full(
	void *arr,
	int64_t len,
	int64_t elem_size,
	int64_t num_threads);
```

### psort_half

Sort the lower half of the integers, returns 0 if succeeded, -1 otherwise.

```
int psort_half(
	void *arr,
	int64_t len,
	int64_t elem_size,
	int64_t num_threads);
```

### psort_partial

Sort the integers within digits between `from` and `to`. `psort_full` is equivalent to `psort_partial` with `from == 0` and `to == elem_size`.

```
int psort_partial(
	void *arr,
	int64_t len,
	int64_t elem_size,
	int64_t num_threads,
	int64_t from,
	int64_t to);
```
