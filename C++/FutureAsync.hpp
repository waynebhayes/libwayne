#include "misc.h"
#include "FutureAsync.h"
#include <iostream>
#include <utility>
#include <future>
#include <limits>

class FutureAsyncClass {
    public:
        foint *RunAll(void);
        FutureAsyncClass(pAsyncFunc f, int n, foint inputs[]);
	~FutureAsyncClass(void);

    private:
	pAsyncFunc f;
	int numThreads;
	foint *inputs, *outputs;
};

FutureAsyncClass::FutureAsyncClass(pAsyncFunc f, int n, foint inputs[n]) {
    this->f = f;
    numThreads = n;
    this->inputs = inputs;
    this->outputs = (foint*)malloc(numThreads * sizeof(*outputs));
}

FutureAsyncClass::~FutureAsyncClass(void) { free(this->outputs); }

foint *FutureAsyncClass::RunAll(void) {
    int i;
    std::future<foint> futureList[numThreads];

    for(i=0; i<numThreads; i++)
	futureList[i] = std::async(std::launch::async, this->f, this->inputs[i]);
    for(i=0; i<numThreads; i++)
	outputs[i] = futureList[i].get();

    return outputs;
}
