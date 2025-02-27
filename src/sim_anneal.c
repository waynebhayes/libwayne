#include "sim_anneal.h"

SIM_ANNEAL *SimAnnealAlloc(double direction, foint initSol, pMoveFunc Move, pScoreFunc Score, pAcceptFunc Accept, unsigned long maxIters) {
    SIM_ANNEAL *sa = Calloc(sizeof(SIM_ANNEAL),1);
    sa->maxIters = maxIters;
    sa->iter = 0;

    // these are invalid values
    sa->temperature = sa->tInitial = sa->tDecay = -1; // invalid values

    if(direction < 0) sa->direction = -1;
    else if(direction > 0) sa->direction = 1;
    else Fatal("SimAnnealAlloc: direction cannot be 0");
    sa->Move = Move;
    sa->Score = Score;
    sa->Accept = Accept;
    sa->currentSolution = initSol;
    sa->currentScore = Score(true, initSol);
    return sa;
}

static double SetIterTemperature(SIM_ANNEAL *sa) {
    assert(sa->tInitial >= 0 && sa->tDecay >= 0);
    assert(sa->iter >=0 && sa->iter < sa->maxIters);
    double fraction = 1.0*sa->iter / sa->maxIters;
    return (sa->temperature = sa->tInitial * exp(-sa->tDecay * fraction));
}

void PbadReset(SIM_ANNEAL *sa) {
    sa->pBadSum = sa->pBadBufLen = sa->pBadBufPos = 0; // reset the buffer;
}

static void PbadRecord(SIM_ANNEAL *sa, double pBad) {
    assert(pBad >= 0.0 && pBad <= 1.0);
    int next = sa->pBadBufPos == PBAD_CIRC_BUF - 1 ? 0 : sa->pBadBufPos + 1;
    if (sa->pBadBufLen == PBAD_CIRC_BUF) {
	sa->pBadSum -= sa->pBadBuf[next];
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
    double delta = sa->Move(sa->currentSolution), pBad=0;
    Boolean accept;
    // always accept good (technically non-bad) moves... note we accept even if the score is the same... THIS IS IMPORTANT
    // because otherwise it's recorded as a "bad" move and adds a pBad=1 to the circular buffer, which we don't want.
    if(sa->direction*(delta) >= 0)
	accept = true;
    else {
	pBad = exp(-fabs(delta) / sa->temperature);
	PbadRecord(sa, pBad);
	accept = (drand48() < pBad);
    }
    //Note("Iter: score %g (%g) T %g pB %g\n", sa->currentScore, delta, sa->temperature, pBad);
    if(accept) sa->currentScore += delta;
    sa->currentSolution = sa->Accept(accept, sa->currentSolution);
}

static double findPbad(SIM_ANNEAL *sa, double temperature) {
    int i = 0;
    sa->temperature = temperature;
    double prevSumPbad=0, sumPbad=0, mean;
    PbadReset(sa);
    do {
	Iteration(sa);
	mean = PbadMean(sa);
	prevSumPbad = sumPbad;
	sumPbad += mean;
	//Note("findPbad(t=%g) iter %d mean pBad = %g", temperature, i, mean);
	++i;
	// below, we compute the previous and current average and demand they agree to some precision
    } while (i < 30 || fabs(prevSumPbad/(i-1) / (sumPbad/i) - 1) > 1e-4); // stabilize to at least this relative precision
    Note("temperature %g gives pBad %g after %d iterations", temperature, mean, i);
    return mean;
}

#define TARGET_PBAD_START 0.99
#define TARGET_PBAD_END 1e-9

Boolean SimAnnealSetSchedule(SIM_ANNEAL *sa, double tInitial, double tDecay) {
    sa->tInitial = tInitial;
    sa->tDecay = tDecay;
    return true;
}

void SimAnnealAutoSchedule(SIM_ANNEAL *sa) {
    double tInitial = 1;
    while (findPbad(sa, tInitial) < TARGET_PBAD_START) tInitial *= 2;
    while (findPbad(sa, tInitial) > TARGET_PBAD_START) tInitial /= 2;
    while (findPbad(sa, tInitial) < TARGET_PBAD_START) tInitial *= 1.2;
    double tEnd = tInitial;
    while (findPbad(sa, tEnd) > TARGET_PBAD_END) tEnd /= 2;
    while (findPbad(sa, tEnd) < TARGET_PBAD_END) tEnd *= 1.2;
    sa->tInitial = tInitial;
    sa->tDecay = -log(tEnd / tInitial);
    Note("tInitial %g tDecay %g", tInitial, sa->tDecay);
}


// returns >0 if success, 0 if not done and can continue, <0 if error
int SimAnnealRun(SIM_ANNEAL *sa) {
    if(sa->tInitial < 0 || sa->tDecay < 0) SimAnnealAutoSchedule(sa);
    sa->temperature = sa->tInitial;
    sa->currentScore = sa->Score(true, sa->currentSolution);
    for(sa->iter = 0; sa->iter < sa->maxIters; sa->iter++) {
	SetIterTemperature(sa);
	Iteration(sa);
	static int prevPctDone;
	int pctDone = 100.0*sa->iter/sa->maxIters;
	if(pctDone > prevPctDone) {
	    double realScore = sa->Score(true, sa->currentSolution);
	    Note("%d%%, incScore %g realScore %g (error %g) pBad %g", pctDone, sa->currentScore, realScore, 
		sa->currentScore-realScore, PbadMean(sa));
	    sa->currentScore=realScore;
	    prevPctDone=pctDone;
	}
    }
    //sa->currentSolution = sa->Accept(true, _bestSolution); // return the best solution seen, not the last one seen
    return 1;
}

foint SimAnnealSol(SIM_ANNEAL *sa) { return sa->currentSolution; }

void SimAnnealFree(SIM_ANNEAL *sa) {
    Free(sa);
}

#ifdef __cplusplus
} // end extern "C"
#endif
