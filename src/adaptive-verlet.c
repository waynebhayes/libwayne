#ifdef __cplusplus
extern "C" {
#endif
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include "leapfrog.h"

#define MAX_N 1000

static int N;
static double T, DT, *R, *V, internal_r[MAX_N], internal_v[MAX_N];
static FF_EVAL F;

/* Returns destination, so the value of this function can be passed to other functions */
double *VecCopy(int n, double destination[n], double src[n])
{
    int i;
    for(i=0; i<n; i++)
	destination[i] = src[i];
    return destination;
}

double Norm(int n, double v[n], double r[n])
{
    double norm1 = 0.0, norm2 = 0.0, norm = 0.0;
    int i;
    for(i=0; i<n; i++)
	norm1 += v[i]*v[i];
		for(i=0; i<n; i++)
	norm2 += r[i]*r[i];
	norm = norm1+norm2;
    return sqrt(norm);
}

/*
** n is the number of dimensions.  r and v are each n-dimensional.
*/
void init_leapfrog(int n, double t0, double dt, double *r, double *v, FF_EVAL f)
{
    double A[n];
    int i;
    N = n;
    T = t0;
    DT = dt;
    R = r;
    V = v;
    assert(0 < n && n <= MAX_N);
    VecCopy(n, internal_r, R);
    VecCopy(n, internal_v, V);
    F = f;


}


/*
** return actual tout.  Note: T and tout refer to the time of the positions;
*/
double integrate_leapfrog(double tout)
{
    double A[N];
    double internal_rh[MAX_N], internal_vh[MAX_N], rho = 0.0;
    int i;

    if(tout == T)
	return T;

    assert(tout >= T);

    /*
    ** calculate acceleration and intial value for rho
    ** rho is what makes the timestep variable
    */
    F(N, T, internal_r, A);
    rho = Norm(N, internal_r, internal_v);

    /* Algorithm: as long as the positions are at a time
     * less than tout, keep going.  Since T represents the time of the positions,
     * and the velocities are half a step ahead after each loop iteration, it's
     * possible the velocities could be *past* tout when we're done.  That's OK.
     */
    while(T + DT <= tout)
    {
	for(i=0; i<N; i++)
			internal_vh[i] = internal_v[i] - DT / (2*rho) * A[i];
	for(i=0; i<N; i++)
	    internal_rh[i] = internal_r[i] + DT / (2*rho) * internal_vh[i];
	T += DT;
	rho = 2*Norm(N, internal_rh, internal_vh)-rho;
	for(i=0; i<N; i++)
			internal_r[i] = internal_rh[i] + DT / (2*rho) * internal_vh[i];
	F(N, T, internal_r, A);
	for(i=0; i < N; i++)
	    internal_v[i] = internal_vh[i] - DT / (2*rho) * A[i];
    }

    /* Now we're close.  Take a weird mutant baby Euler step to tout.
     * The velocities are half a timestep ahead of the positions, so
     * their mutant timestep is shorter by DT/2.
     */
   for(i=0; i<N; i++)
	R[i] = internal_r[i];
	for(i=0; i < N; i++)
	V[i] = internal_v[i];

  /*
    for(i=0; i<N; i++)
	R[i] = internal_r[i] + (tout - T) * internal_v[i];
    F(N, tout, R, A);
    for(i=0; i < N; i++)
	V[i] = internal_v[i] + (tout - (T+DT/2)) * A[i];
*/
    return tout;
}
#ifdef __cplusplus
} // end extern "C"
#endif
