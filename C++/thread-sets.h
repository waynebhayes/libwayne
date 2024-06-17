/*
** This little library allows you to do "lightweight threadding" by splitting calls to the same function among multiple
** You specify a function pointer, the number of threads, and an array of foint inputs[numThreads].
** The function should take just one foint as argument, and return one foint.
*/

typedef void THREAD_SET;

#ifdef __cplusplus
extern "C" {
#endif

#include "misc.h"

typedef foint (*pAsyncFunc)(const foint); // your function MUST satisfy this function prototype.

THREAD_SET *ThreadSetAlloc(pAsyncFunc f, int numThreads, foint input[]); // allocate (but do not run) a set of threads
foint      *ThreadSetRunAll(const THREAD_SET *ts); // start the threads; won't return until all threads are done
void        ThreadSetFree(const THREAD_SET *ts); // free the thread set

#ifdef __cplusplus
}
#endif

