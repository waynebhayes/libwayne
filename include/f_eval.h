#ifdef __cplusplus
extern "C" {
#endif
#ifndef _F_EVAL_H
#define _F_EVAL_H

/* Double precision */
typedef void (*F_EVAL)(int N, double T, double *Y, double *Ydot);

/* Long Double (== Fortran's QUAD or REAL*16 on Suns, or 12-byte on i686) */
typedef void (*LD_EVAL)(int N, long double T, long double *Y, long double *Ydot);

#endif  /* _F_EVAL_H */
#ifdef __cplusplus
} // end extern "C"
#endif
