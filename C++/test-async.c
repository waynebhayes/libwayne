#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include "misc.h"
#include "FutureAsync.h"

#define NUM_THREADS 8
#define COUNT 4000000000U
#define COUNT_MOD 1000000000U

foint FAtest(const foint f) {
    assert(f.i < NUM_THREADS);
    printf(" Starting FAtest %d", f.i);
    unsigned i;
    for(i=0; i<COUNT;i++) if(i % COUNT_MOD == 0) {printf(" %d:%uM", f.i,i/1000000); fflush(stdout);}
    printf("\n");
    return f;
}

int main(){
    int i;
    foint FAinput[NUM_THREADS];
    for(i=0;i<NUM_THREADS;i++) FAinput[i].i=i;
    FutureAsync *fa = FutureAsyncAlloc(FAtest, NUM_THREADS, FAinput);
    FutureAsyncRunAll(fa);
    FutureAsyncFree(fa);
    return 0;
}
