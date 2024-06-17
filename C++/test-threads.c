// C code to test libwayne's threads, implemented via C++ future/async library.
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include "misc.h"
#include "thread-sets.h"

#define NUM_THREADS 4
#define COUNT 4000000000U
#define COUNT_MOD 1000000000U

foint ThreadTest(const foint f) {
    assert(f.i < NUM_THREADS);
    printf(" Starting thread %d", f.i);
    unsigned i;
    for(i=0; i<COUNT;i++) if(i % COUNT_MOD == 0) {printf(" %d:%uM", f.i,i/1000000); fflush(stdout);}
    printf("\n");
    return f;
}

int main(){
    int i;
    foint threadInput[NUM_THREADS];
    for(i=0;i<NUM_THREADS;i++) threadInput[i].i=i;
    THREAD_SET *ts = ThreadSetAlloc(ThreadTest, NUM_THREADS, threadInput);
    ThreadSetRunAll(ts);
    ThreadSetFree(ts);
    return 0;
}
