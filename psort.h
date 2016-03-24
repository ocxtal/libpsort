
/**
 * @file psort.h
 *
 * @brief parallel integer sort (radixsort) library
 *
 * @author Hajime Suzuki
 * @date 2016/3/20
 * @license MIT
 */
#ifndef _PSORT_H_INCLUDED
#define _PSORT_H_INCLUDED

/**
 * @type psort_t
 * @brief context container
 */
typedef struct psort_s psort_t;

/**
 * @fn psort_init
 */
psort_t *psort_init(
	int64_t num_threads,
	int64_t elem_size);

/**
 * @fn psort_clean
 */
void psort_clean(
	psort_t *ctx);

/**
 * @fn psort_sort
 * @brief integer sort
 */
void psort_sort(
	psort_t *ctx,
	void *ptr,
	int64_t len);

/**
 * @fn psort_keysort
 * @brief key sort (sort the lower half of the element)
 */
void psort_keysort(
	psort_t *ctx,
	void *ptr,
	int64_t len);

/**
 * @fn psort_partialsort
 */
void psort_partialsort(
	psort_t *ctx,
	void *ptr,
	int64_t len,
	int64_t lower_digit,
	int64_t higher_digit);

#endif /* _PSORT_H_INCLUDED */
/**
 * end of psort.h
 */
