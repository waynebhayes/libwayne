#ifdef __cplusplus
extern "C" {
#endif
#ifndef _RK4_H
#define _RK4_H

#include "f_eval.h"


typedef struct _rk4
{
    int N;
    double DT, *Y, T;
    F_EVAL F;
} RK4;


/*
 * RK4 is the "classical" 4-stage, 4th order RK formula.

 */
RK4 *Rk4Alloc(int n, double t, double *y, F_EVAL f, int stiff_flag,
    double dt, double zero);

/* Do the actual integration.  Return the actual TOUT */
double Rk4Integrate(RK4 *l, double TOUT);

void Rk4Free(RK4 *l);

#endif  /* _RK4_H */
#ifdef __cplusplus
} // end extern "C"
#endif
