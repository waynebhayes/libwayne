typedef void FutureAsync;

#ifdef __cplusplus
extern "C" {
#endif

#include "misc.h"
typedef foint (*pAsyncFunc)(const foint);
FutureAsync *FutureAsyncAlloc(pAsyncFunc f, int numThreads, foint input[numThreads]);
foint       *FutureAsyncRunAll(const FutureAsync *fa);
void         FutureAsyncFree(const FutureAsync *fa);

#ifdef __cplusplus
}
#endif

