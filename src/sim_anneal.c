// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#include "sim_anneal.h"
#include "rand48.h"
#include "stats.h"

SIM_ANNEAL *SimAnnealAlloc(double direction, foint initSol, pMoveFunc Move, pScoreFunc Score, pAcceptFunc Accept,
    unsigned long maxIters, double pBadStart, double pBadEnd, pReportFunc Report) {
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
    sa->Report = Report;
    sa->currentSolution = initSol;
    sa->currentScore = Score(true, initSol);
    sa->pBadStart = pBadStart ? pBadStart : 0.99;
    sa->pBadEnd = pBadEnd ? pBadEnd : 1e-8;
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
	accept = (Drand48() < pBad); // inline drand48(): same stream, no call
    }
    //Note("Iter: score %g (%g) T %.3e pB %g\n", sa->currentScore, delta, sa->temperature, pBad);
    accept = sa->Accept(accept, sa->currentSolution);
    if(accept) sa->currentScore += delta;
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
    } while (i < 3000 || fabs(prevSumPbad/(i-1) / (sumPbad/i) - 1) > 1e-4); // stabilize to at least this relative precision
    Note("temperature %g gives pBad %g after %d iterations", temperature, mean, i);
    return mean;
}

Boolean SimAnnealSetSchedule(SIM_ANNEAL *sa, double tInitial, double tDecay) {
    sa->tInitial = tInitial;
    sa->tDecay = tDecay;
    return true;
}

void SimAnnealAutoSchedule(SIM_ANNEAL *sa) {
    double tEnd = 1;
    while (findPbad(sa, tEnd) > sa->pBadEnd) tEnd /= 2;
    while (findPbad(sa, tEnd) < sa->pBadEnd) tEnd *= 1.2;
    double tInitial = tEnd;
    while (findPbad(sa, tInitial) < sa->pBadStart) tInitial *= 2;
    while (findPbad(sa, tInitial) > sa->pBadStart) tInitial /= 2;
    while (findPbad(sa, tInitial) < sa->pBadStart) tInitial *= 1.2;
    sa->tInitial = tInitial;
    sa->tDecay = -log(tEnd / tInitial);
    Note("tInitial %g tDecay %g", tInitial, sa->tDecay);
}


void SimAnnealSetTolerance(SIM_ANNEAL *sa, double tolerance, unsigned long batchSize) {
    if(tolerance < 0) Fatal("SimAnnealSetTolerance: tolerance must be >= 0, but is %g", tolerance);
    sa->tolerance = tolerance;
    sa->batchSize = batchSize;
}

// Re-score the solution from scratch, warn if the incrementally-maintained score has drifted from it, and resync.
static void CheckIncrementalScore(SIM_ANNEAL *sa) {
    double realScore = sa->Score(true, sa->currentSolution);
    double error = sa->currentScore-realScore;
    if(error>0) {
	if(realScore==0) Note("[incEvalError: %g vs. real %g (error %g)]", sa->currentScore, realScore, error);
	else {
	    double relError = fabs(error/realScore);
	    if(relError>1e-12) Note("[incEvalError: %g vs. real %g (error %g)]", sa->currentScore, realScore, error);
	}
    }
    sa->currentScore=realScore;
}

// The tolerance schedule below is a port of SANA's runUsingConfidenceIntervals (github.com/waynebhayes/SANA,
// src/methods/SANA.cpp), including its heuristic constants.
#define SA_MAX_TAU_STEP 0.01	// largest step through the schedule: at most 100 temperatures
#define SA_MIN_TAU_STEP 0.001	// smallest step, used where the score refuses to settle
#define SA_MIN_BATCHES 30	// batches at a temperature before we even look at the confidence intervals
#define SA_HAPPY_BATCHES 10000	// after this many batches at one temperature, start shrinking the step instead
#define SA_MIN_CONFIDENCE 0.99999
#define SA_TOL_SAFETY_MARGIN 1.07 // from SANA: empirically cuts the failure rate to below 5%

// Walk the same exponential schedule as the fixed-iteration run, T = tInitial*exp(-tDecay*tau) for tau in [0,1], but
// stay at each temperature until the score has settled there: run batches of batchSize iterations, and move on once
// the confidence intervals of both the batch-mean score and the batch-mean pBad are narrower than the tolerance
// (relative to the score). Where the score is changing fast, that takes many batches; where it's flat, MIN_BATCHES.
static int SimAnnealRunWithTolerance(SIM_ANNEAL *sa) {
    unsigned long batchSize = sa->batchSize;
    if(!batchSize) batchSize = MAX(100, (unsigned long)sqrt((double)sa->maxIters));
    double tau, tauStep = SA_MAX_TAU_STEP;
    double tolPerStep = sa->tolerance * tauStep / SA_TOL_SAFETY_MARGIN;
    double confidence = 1-pow(tolPerStep, 1.5); // from SANA: empirically works well
    if(confidence < SA_MIN_CONFIDENCE) confidence = SA_MIN_CONFIDENCE; // costs little CPU to be more confident
    Note("SimAnnealRun: tolerance %g, batchSize %lu, confidence %g, tolerance per step %g",
	sa->tolerance, batchSize, confidence, tolPerStep);

    STAT *scoreBatchMeans = StatAlloc(0, 0.0, 0.0, false, false), *pBadBatchMeans = StatAlloc(0, 0.0, 0.0, false, false);
    double scoreBatchSum = 0, previousScore = 0;
    unsigned long batchIters = 0, lastBatchCount = 0;
    sa->iter = 0;
    for(tau = 0; tau <= 1; tau += tauStep) {
	sa->temperature = sa->tInitial * exp(-sa->tDecay * tau);
	unsigned long batchesHere = 0, itersHere = sa->iter;
	Boolean satisfied = false;
	while(!satisfied) {
	    Iteration(sa);
	    ++sa->iter;
	    scoreBatchSum += sa->currentScore;
	    if(++batchIters < batchSize) continue;
	    // end of a batch
	    ++batchesHere;
	    StatAddSample(scoreBatchMeans, scoreBatchSum / batchIters);
	    StatAddSample(pBadBatchMeans, PbadMean(sa));
	    scoreBatchSum = batchIters = 0;
	    if(StatNumSamples(scoreBatchMeans) < SA_MIN_BATCHES) continue;

	    // The tolerance is relative to the final score, which we don't know yet. Early on, while the score is near zero
	    // and pBad is high, this is effectively an absolute tolerance scaled by pBad; once the score is bigger than
	    // pBad it becomes a genuine relative tolerance. Also loosen it slowly the longer we're stuck at one
	    // temperature, so a score that keeps fluctuating can't hold us here forever.
	    double relativeMultiplier = MAX(fabs(StatMean(scoreBatchMeans)), StatMean(pBadBatchMeans)) * (1+log(batchesHere));
	    double interval = tolPerStep * relativeMultiplier;
	    // "<=" rather than SANA's "<": a frozen score of exactly 0 with pBad 0 has interval 0, and must still finish
	    if(StatConfInterval(scoreBatchMeans, confidence) <= interval && StatConfInterval(pBadBatchMeans, confidence) <= interval)
		satisfied = true;
	    else if(StatNumSamples(scoreBatchMeans) >= SA_HAPPY_BATCHES) {
		if(sa->direction*StatMean(scoreBatchMeans) > sa->direction*previousScore) {
		    // still improving steadily: the early batches bias the mean, so start the statistics over
		    previousScore = StatMean(scoreBatchMeans);
		    lastBatchCount = 0; StatReset(scoreBatchMeans); StatReset(pBadBatchMeans);
		} else if(tauStep > SA_MIN_TAU_STEP && StatNumSamples(scoreBatchMeans) >= SA_HAPPY_BATCHES+lastBatchCount) {
		    tauStep = MAX(tauStep*2.0/3.0, SA_MIN_TAU_STEP); // not improving and not settling: take smaller steps
		    lastBatchCount = StatNumSamples(scoreBatchMeans);
		}
	    }
	}
	printf("%.1f%% T %.2e pBad %.3g iters %lu ", 100*tau, sa->temperature, PbadMean(sa), sa->iter - itersHere);
	if(sa->Report) sa->Report(sa->iter, sa->currentSolution);
	puts("");
	CheckIncrementalScore(sa);
	if(tauStep < SA_MAX_TAU_STEP) {
	    if(StatNumSamples(scoreBatchMeans) < SA_HAPPY_BATCHES) tauStep = MIN(tauStep*3, SA_MAX_TAU_STEP); // easy again
	    else if(sa->direction*StatMean(scoreBatchMeans) < sa->direction*previousScore)
		tauStep = MIN(tauStep*10, SA_MAX_TAU_STEP); // score is going nowhere here: skip ahead
	}
	previousScore = StatMean(scoreBatchMeans);
	StatReset(scoreBatchMeans); StatReset(pBadBatchMeans);
    }
    Note("SimAnnealRun: %lu iterations in total", sa->iter);
    StatFree(scoreBatchMeans); StatFree(pBadBatchMeans);
    return 1;
}

// returns >0 if success, 0 if not done and can continue, <0 if error
int SimAnnealRun(SIM_ANNEAL *sa) {
    if(sa->tInitial < 0 || sa->tDecay < 0) SimAnnealAutoSchedule(sa);
    sa->temperature = sa->tInitial;
    sa->prevPctDone = 0; // was a function-level static, so only the first run in a process ever reported progress
    sa->currentScore = sa->Score(true, sa->currentSolution);
    if(sa->tolerance > 0) return SimAnnealRunWithTolerance(sa);
    for(sa->iter = 0; sa->iter < sa->maxIters; sa->iter++) {
	SetIterTemperature(sa);
	Iteration(sa);
	int pctDone = 100.0*sa->iter/sa->maxIters;
	if(pctDone > sa->prevPctDone) {
	    printf("%d%% T %.2e pBad %.3g ", pctDone, sa->temperature, PbadMean(sa));
	    if(sa->Report) sa->Report(sa->iter, sa->currentSolution);
	    puts("");
	    CheckIncrementalScore(sa);
	    sa->prevPctDone=pctDone;

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
