/*
 * lapack.h -- replacement header for lapack functions if the system does
 *             not have an lapack header (Mac OS X has one in the
 *             Accelerate framework)
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswi
 */
#ifndef _lapack_h
#define _lapack_h

#ifdef __cplusplus
extern "C" {
#endif 

void	dgels_(char *trans, int *m, int *n, int *nrhs, double *a, int *lda,
		double *b, int *ldb, double *work, int *lwork, int *info);

#ifdef __cplusplus
}
#endif

#endif /* _lapack_h */
