#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "misc.h"
#include "stats.h"

char USAGE[] = "USAGE: %s batchSize confidence interval\n"
    "read numbers on the standard input (no files!), in batches of batchSize\n"
    "keep reading until we know the mean within interval (smaller is better),\n"
    "with confidence 'confidence' (closer to 1 is better)\n"
    "NOTE: with no arguments, the default is batchSize 100 conf 0.99 interval 0.01\n";

int main(int argc, char *argv[])
{
    STAT *batch, *batchMeans;
    Boolean geom=false, allData=false;
    int batchSize, numBins=0;
    double sample, confidence, interval, histMin=0, histMax=0;
    if(argc <= 2) {
	batchSize = 100; confidence = 0.99, interval = 0.01;
    } else if(argc != 4)
	Fatal(USAGE,argv[0]);
    else {
	batchSize = atoi(argv[1]); confidence=atof(argv[2]); interval=atof(argv[3]);
    }
    fprintf(stderr, "Using batchSize %d conf %g interval %g\n", batchSize, confidence, interval);

    batch = StatAlloc(numBins, histMin, histMax, geom, allData);
    batchMeans = StatAlloc(numBins, histMin, histMax, geom, allData);

    while(scanf("%lf", &sample) == 1 && (StatSampleSize(batchMeans)<3 || fabs(StatConfInterval(batchMeans, confidence)) > interval))
    {
	StatAddSample(batch, sample);
	if(StatSampleSize(batch) == batchSize)
	{
	    StatAddSample(batchMeans, StatMean(batch));
	    StatReset(batch);
	}
    }

    StatFree(batch);
    printf("# %d mean %.16g min %.16g max %.16g stdDev %.16g var %.16g skew %.16g\n",
	StatSampleSize(batchMeans), StatMean(batchMeans), StatMin(batchMeans), StatMax(batchMeans),
	    StatStdDev(batchMeans), StatVariance(batchMeans), StatSkew(batchMeans));
    StatFree(batchMeans);

    return 0;
}
