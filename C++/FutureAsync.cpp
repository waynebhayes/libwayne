#include "FutureAsync.hpp"
#include "FutureAsync.h"
#include <future>

extern "C" {
    FutureAsync *FutureAsyncAlloc(pAsyncFunc f, int numThreads, foint input[numThreads]) {
	FutureAsyncClass *fa = new FutureAsyncClass(f, numThreads, input);
	return (FutureAsync*)fa;
    }

    foint *FutureAsyncRunAll(const FutureAsync *fa) {
	FutureAsyncClass *fac = (FutureAsyncClass *)fa;
	return fac->RunAll();
    }

    void FutureAsyncFree(const FutureAsync *fa) {
	FutureAsyncClass *fac = (FutureAsyncClass *)fa;
	delete fac;
    }
}
