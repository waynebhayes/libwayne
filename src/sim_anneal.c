#include "sim_anneal.h"

SIM_ANNEAL *SimAnnealAlloc(double direction, foint initSol, pSolutionFunc Move, pScoreFunc Score, pSolutionFunc Accept, int maxIters) {
    SIM_ANNEAL *sa = Calloc(sizeof(SIM_ANNEAL),1);
    sa->maxIters = maxIters;
    sa->iter = 0;

    // these are invalid values
    sa->temperature = sa->tInitial = -1;

    if(direction < 0) sa->direction = -1;
    else if(direction > 0) sa->direction = 1;
    else Fatal("SimAnnealAlloc: direction cannot be 0");
    sa->Move = Move;
    sa->Score = Score;
    sa->Accept = Accept;
    sa->currentSolution = initSol;
    return sa;
}

static double GetTemperature(SIM_ANNEAL *sa) {
    double fraction = 1.0*sa->iter / sa->maxIters;
    return (sa->temperature = sa->tInitial * exp(-sa->tDecay * fraction));
}

static void PbadRecord(SIM_ANNEAL *sa, double pBad) {
    assert(pBad >= 0.0 && pBad <= 1.0);
    int next = sa->pBadBufPos == PBAD_CIRC_BUF - 1 ? 0 : sa->pBadBufPos + 1;
    if (sa->pBadBufLen == PBAD_CIRC_BUF) {
	sa->pBadSum -= sa->pBadBuf[next]; // the next one is the first one
	if(sa->pBadSum < 0) sa->pBadSum = 0; // very rarely numerical error put it < 0
    } else {
	sa->pBadBufLen++;
    }
    sa->pBadBufPos = next;
    sa->pBadBuf[next] = pBad;
    sa->pBadSum += pBad;
}

static double PbadMean(SIM_ANNEAL *sa) {
    if(sa->pBadBufLen == 0) return 0.5;
    return sa->pBadSum / sa->pBadBufLen;
}

static void Iteration(SIM_ANNEAL *sa) {
    double oldScore = sa->Score(sa->currentSolution);
    foint newSol = sa->Move(true, sa->currentSolution);
    double newScore = sa->Score(newSol);
    Boolean accepted;
    if(sa->direction*(newScore - oldScore) > 0) // always accept good (technically non-bad) moves
	accepted = true;
    else {
	double diff = fabs(newScore - oldScore); // don't need direction any more, we know it's a bad move
	double pBad = exp(-diff / sa->temperature);
	PbadRecord(sa, pBad);
	accepted = (drand48() < pBad);
    }
    sa->Accept(accepted, newSol);
}

static double findPbad(SIM_ANNEAL *sa, double temperature) {
    int i = 0;
    sa->temperature = temperature;
    double prevSumPbad=0, sumPbad=0, mean;
    do {
	Iteration(sa);
	double newPbad = PbadMean(sa);
	prevSumPbad = sumPbad;
	sumPbad += newPbad;
	++i;
	// below, we compute the previous and current average and demand they agree to some precision
    } while (i < 30 || fabs(prevSumPbad/(i-1) / (sumPbad/i) - 1) > 1e-3); // stabilize to at least this relative precision
    mean = PbadMean(sa);
    printf("temperature %g gives pBad %g after %d iterations\n", temperature, mean, i);
    return mean;
}

#define TARGET_PBAD_START 0.85
#define TARGET_PBAD_END 1e-6

static void CreateSchedule(SIM_ANNEAL *sa) {
    sa->tInitial = 1;
    while (findPbad(sa, sa->tInitial) < TARGET_PBAD_START) sa->tInitial *= 2;
    while (findPbad(sa, sa->tInitial) > TARGET_PBAD_START) sa->tInitial /= 2;
    while (findPbad(sa, sa->tInitial) < TARGET_PBAD_START) sa->tInitial *= 1.2;
    double tEnd = sa->tInitial;
    while (findPbad(sa, tEnd) > TARGET_PBAD_END) tEnd /= 2;
    while (findPbad(sa, tEnd) < TARGET_PBAD_END) tEnd *= 1.2;

    sa->tDecay = -log(tEnd / sa->tInitial);
    printf("tInitial = %g, tEnd = %g, tDecay = %g\n", sa->tInitial, tEnd, sa->tDecay);
}

// returns >0 if success, 0 if not done and can continue, <0 if error
int SimAnnealRun(SIM_ANNEAL *sa) {
    CreateSchedule(sa); // FIXME: may not need to create the schedule (it's expensive) if already known
    for(sa->iter = 0; sa->iter < sa->maxIters; sa->iter++) Iteration(sa);
    return 1;
}

foint SimAnnealSol(SIM_ANNEAL *sa) { return sa->currentSolution; }

void SimAnnealFree(SIM_ANNEAL *sa) {
    Free(sa);
}

#ifdef __cplusplus
} // end extern "C"
#endif
