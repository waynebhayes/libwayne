// This is the C++ code that will be called from C functions that want to run a bunch of pAsyncFunctions in parallel threads.

#include "FutureAsync.hpp"
#include "thread-sets.h"
#include <future>

extern "C" {
    THREAD_SET *ThreadSetAlloc(pAsyncFunc f, int numThreads, foint input[]) {
	FutureAsync *fa = new FutureAsync(f, numThreads, input);
	return (THREAD_SET*)fa;
    }

    foint *ThreadSetRunAll(const THREAD_SET *ts) {
	FutureAsync *fa = (FutureAsync *)ts;
	return fa->RunAll();
    }

    void ThreadSetFree(const THREAD_SET *ts) {
	FutureAsync *fa = (FutureAsync *)ts;
	delete fa;
    }
}
