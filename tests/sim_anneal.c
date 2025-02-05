#include "misc.h"
#include "sim_anneal.h"
#include "rand48.h"

// DUMB SORT: sort array of integers via simulated annealing

#define N 10UL
int _array[N];

// Goal: array is sorted smallest-to-largest (non-decreasing).
// So smaller objective is better.
static double ScoreLocal(const int i) {
    //printf("L"); fflush(stdout);
    int j;
    double sum=0;
    for(j=0;j<N;j++) {
	Boolean bad = false;
	if(j<i) { // we WANT a[j] < a[i]
	    if(_array[j] > _array[i]) bad=true;
	} else { // j>i so we WANT a[j]>a[i]
	    if(_array[j] < _array[i]) bad=true;
	}
	if(bad) sum += fabs((_array[i]-_array[j])*1.0*(j-i));
    }
    return sum;
}
double ScoreGlobal(const foint f) {
    //printf("G"); fflush(stdout);
    int i;
    double sum=0;
    for(i=0;i<N;i++) sum+= ScoreLocal(i);
    return sum;
}

static int _swap1=-1, _swap2=-1; // -1 means "not currently assigned"

double SwapElements(const foint f) {
    //printf("S"); fflush(stdout);
    assert(_swap1<0 && _swap2<0);
    _swap1 = N*drand48();
    do _swap2 = N*drand48(); while(_swap1==_swap2);
    double before=ScoreLocal(_swap1) + ScoreLocal(_swap2);
    int tmp = _array[_swap1]; _array[_swap1]=_array[_swap2]; _array[_swap2]=tmp;
    double after =ScoreLocal(_swap1) + ScoreLocal(_swap2);
    return (after-before); // we want to minimize, so if after<before, this is negative (ie., good)
}


foint AcceptReject(Boolean accept, const foint f) {
    //printf("%c", accept?'A':'R'); fflush(stdout);
    assert(_swap1>=0 && _swap2>=0);
    if(!accept) { // swap them back
	int tmp = _array[_swap1]; _array[_swap1]=_array[_swap2]; _array[_swap2]=tmp;
    }
    _swap1 = _swap2 = -1;
    return f;
}


int main(void) {
    int i;
    foint f;
    // srand48(GetFancySeed(false)); // comment out this line for reproducible results
    for(i=0;i<N;i++) _array[i] = drand48()*N;

    const unsigned long maxIters = SQR(N)*SQR(N*N);
    printf("%g\n", ScoreGlobal(f));
    SIM_ANNEAL *sa = SimAnnealAlloc(-1, (foint)(void*)_array, SwapElements, ScoreGlobal, AcceptReject, maxIters);
    SimAnnealAutoSchedule(sa);
    int result = SimAnnealRun(sa);
    printf("SimAnnealRun returned %d\n", result);
    printf("%g\n", ScoreGlobal(f));
}

#if 0
// direction < 0 to minimize, > 0 to maximize
SIM_ANNEAL *SimAnnealAlloc(double direction, foint initSol, pSolutionFunc Move, pScoreFunc, pSolutionFunc Accept, int maxIters);
Boolean SimAnnealSetSchedule(SIM_ANNEAL *sa, double tInitial, double tDecay);
void SimAnnealAutoSchedule(SIM_ANNEAL *sa); // to automatically create schedule
int SimAnnealRun(SIM_ANNEAL *sa); // returns >0 if success, 0 if not done and can continue, <0 if error
foint SimAnnealSol(SIM_ANNEAL *sa);
void SimAnnealFree(SIM_ANNEAL *sa);

-----------------------------------
int main(void) {
    SIM_ANNEAL *sa = SimAnnealAlloc(-1, (foint)(void*)(&_g), SA_GenMove, SA_Score, SA_Accept, SQR(_k));
    SimAnnealAutoSchedule(sa, _tInitials[_k], _tDecays[_k]);
    int result = SimAnnealRun(sa);
    if(result <= 0) Fatal("SimAnnealRun returned %d", result);
    foint solution = SimAnnealSol(sa);
    SimAnnealFree(sa);
}
#endif

