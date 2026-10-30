// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#ifdef __cplusplus
extern "C" {
#endif
/*
** libwayne's own drand48 family. It's the generator POSIX specifies, X <- (a*X + c) mod 2^48 with
** a = 0x5DEECE66D and c = 0xB unless lcong48 changes them, and returns exactly what libc's does, so a
** given seed gives the same sequence it always has. Because libwayne defines these functions, they
** replace libc's for the whole program, so everything (the caller and libwayne itself) still draws
** from one stream. The point of having our own is that the state is visible, which lets rand48.h
** offer Drand48(), an inline drand48() for hot loops that shares this same state.
*/
#include "rand48.h"

#define RAND48_DEFAULT_A 0x5DEECE66DULL
#define RAND48_DEFAULT_C 0xBULL

unsigned long long _rand48State = 0; // libc's unseeded state is also 0
unsigned long long _rand48A = RAND48_DEFAULT_A, _rand48C = RAND48_DEFAULT_C;

static unsigned long long Get48(const unsigned short x[3]) {
    return x[0] | ((unsigned long long)x[1] << 16) | ((unsigned long long)x[2] << 32);
}

static void Put48(unsigned short x[3], unsigned long long v) {
    x[0] = v & 0xffff; x[1] = (v >> 16) & 0xffff; x[2] = (v >> 32) & 0xffff;
}

// Advance a caller-owned 48-bit state (erand48 etc.) using the current a and c
static unsigned long long Next48(unsigned short x[3]) {
    unsigned long long v = (_rand48A * Get48(x) + _rand48C) & RAND48_MASK;
    Put48(x, v);
    return v;
}

double drand48(void) { return Drand48(); }
long lrand48(void) { return (long)(Rand48Next() >> 17); }
long mrand48(void) { return (long)(int)(Rand48Next() >> 16); }

double erand48(unsigned short X[3]) { return Next48(X) * RAND48_TO_DOUBLE; }
long nrand48(unsigned short X[3]) { return (long)(Next48(X) >> 17); }
long jrand48(unsigned short X[3]) { return (long)(int)(Next48(X) >> 16); }

void srand48(long seedval) {
    _rand48State = (((unsigned long long)seedval & 0xffffffffULL) << 16) | 0x330E;
    _rand48A = RAND48_DEFAULT_A; _rand48C = RAND48_DEFAULT_C;
}

unsigned short *seed48(unsigned short seed16v[3]) {
    static unsigned short old[3];
    Put48(old, _rand48State);
    _rand48State = Get48(seed16v);
    _rand48A = RAND48_DEFAULT_A; _rand48C = RAND48_DEFAULT_C;
    return old;
}

void lcong48(unsigned short param[7]) {
    _rand48State = Get48(param);
    _rand48A = Get48(param + 3);
    _rand48C = param[6];
}

#ifdef __cplusplus
} // end extern "C"
#endif
