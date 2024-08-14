// C code to test libwayne's threads, implemented via C++ future/async library.
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include "rand48.h"
#include "misc.h"
#include "thread-sets.h"
#include "mt19937.h"

#define ORDINALS 100
#define NUM_THREADS 4
#define COUNT     100000000U
#define COUNT_MOD 10000000U

static double _realSums[NUM_THREADS], _ordinalSums[ORDINALS];
static MT19937 *_randGen[NUM_THREADS];


foint ThreadTest(const foint f) {
    static char _race[COUNT+1]; // who got there first?
    assert(f.i < NUM_THREADS);
    printf("S%d", f.i+1);
    unsigned i;
    for(i=0; i<=COUNT;i++) {
	double r = Mt19937NextDouble(_randGen[f.i]);
	unsigned where = Mt19937NextDouble(_randGen[f.i])*ORDINALS;
	_ordinalSums[where] += r;
	_realSums[f.i] += r;
	if(i % COUNT_MOD == 0) {
	    if(!_race[i]) {_race[i]=f.i+1; printf(" T%d:%uM!", f.i+1,i/1000000);}
	    else printf(" T%d:%u=%d", f.i+1,i/1000000,_race[i]);
	    fflush(stdout);
	}
    }
    printf("\n");
    return f;
}

int main(){
    int i;
    foint threadInput[NUM_THREADS];
    for(i=0;i<NUM_THREADS;i++) {
 	threadInput[i].i=i;
	_randGen[i] = Mt19937Alloc(i);
	double Mt19937NextDouble(const MT19937 *t);
    }
    THREAD_SET *ts = ThreadSetAlloc(ThreadTest, NUM_THREADS, threadInput);
    ThreadSetRunAll(ts);
    ThreadSetFree(ts);
    double realSum = 0.0, ordinalSum=0.0;
    for(i=0; i<NUM_THREADS;i++) realSum += _realSums[i];
    for(i=0; i<ORDINALS;i++) ordinalSum += _ordinalSums[i];
    printf("\noverlapping sum %g, realSum %g, ratio %g\n", ordinalSum, realSum, ordinalSum/realSum);
    return 0;
}
