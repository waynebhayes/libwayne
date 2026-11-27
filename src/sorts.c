// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#ifdef __cplusplus
extern "C" {
#endif
/* Version 0.0
** From "Wayne's Little DSA Library" (DSA == Data Structures and
** Algorithms) Feel free to change, modify, or burn these sources, but if
** you modify them please don't distribute your changes without a clear
** indication that you've done so.  If you think your change is spiffy,
** send it to me and maybe I'll include it in the next release.
**
** Wayne Hayes, wayne@csri.utoronto.ca (preffered), or wayne@csri.toronto.edu
*/

#include "misc.h"
#include "sorts.h"
#include "heap.h"	/* for heapsort */
#include "sets.h"
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define TEST_SORTS 0
int _compareCount;

int QuickSort(void *a, size_t n, size_t w, pfnCmpFcn compare)
{
    qsort(a, n, w, compare);
    return 0;
}

/* Pile Sort
** Inspired by physically sorting a few hundred shipping labels in my garage
** because I had to look for duplicates.  It's inspired by MergeSort, which
** I've used before: the goal is to build up a small set of lists, each of
** which is already sorted, so you can perform a final mergesort on them
** once they contain everything in your list. The unique, novel part of the
** sort was that when I was physically doing it, I noticed that there were
** some O(1) operations I could do with each new elements: given a bunch
** of piles of existing labels, each pile already sorted smallest-at-top,
** I could take a new label in hand and see if there was a place I could
** put it on top of an existing pile: if this new element was smaller than
** the existing top element in some pile P, then we can place it on top
** of P in O(1) time. Or, if the item was larger than the bottom element
** in some pile Q, then it can be placed in Q. The problem is that finding
** which pile satisfies one of these properties can take awhile if there's
** too many piles already.  So we want to minimize the number of piles.
** Finally, if there is no pile onto which we can place it either or top,
** or append to the bottom, then it has to be put is the lone item in a
** new pile all by itself.
**
** In an effort to keep the number of piles small, we also want to find
** the top or bottom element that is closest to the new element in hand;
** that results in the fewest "holes" in the ordering in the piles.  If the
** number of piles is q, then this takes exactly 2*q computations, as we
** look for the optimal place to put the new element.
**
** To implement the piles, we'd basically keep a pointer top and bottom for
** each pile.  Each pile is implemented as a fixed-size array (not sure
** what size to use here...), and we put the first element in the middle
** of the array---or perhaps at a position proportional to its value,
** like interpolation search, to maximize the probability of being able to
** "fill up" the array before we have to give up using it.  Once decide
** to give up using a pile, we throw it into the MergeQueue, which is the
** collection of piles that will be later merged in one final sweep---or
** alternately we can merge them more frequently to minimize the number of
** piles that are in the MergeQueue.
*/
typedef struct {int top, bottom, *a;} PILEInts;

// Merge a bunch of piles into destination; you must ensure it has space
// Returns the number of things merged.
static int PileMergeInts(int *dest, int numPiles, PILEInts *pile)
{
    Boolean done = false;
    int n=0;
    while(!done)
    {
	done = true;
	int smallest = -1, p;
	for(p=0; p < numPiles; p++)
	{
	    int top = pile[p].top, bottom = pile[p].bottom;
	    if(top <= bottom)
	    {
		done = false;
		if(smallest == -1) smallest = p;
		else
		{
		    _compareCount++;
		    if(pile[p].a[top] < pile[smallest].a[pile[smallest].top]) smallest = p;
		}
	    }
	}
	assert(done || smallest != -1);
	if(!done) dest[n++] = pile[smallest].a[pile[smallest].top++];
    }
    return n;
}


static int *_tempMerge, *_mergePosition;
static int TempMergeCmpInts(const foint X, const foint Y)
{
    const int tm1 = X.i, tm2 = Y.i;
    _compareCount++;
    return _tempMerge[_mergePosition[tm1]] - _tempMerge[_mergePosition[tm2]];
}

// A tempMerge list is one big long array containing subarrays that are each sorted.
// The subarrays start at mergeSeparator[i] for i going from 0 to numMerges
// Returns the number of things merged.
static int PileMergeTempsInts(int *dest, int nMerge, int mergeSeparator[nMerge+1], int *tempMerge)
{
    int mergePosition[nMerge];
    int i, n=0;

    _tempMerge = tempMerge;
    _mergePosition = mergePosition;
    HEAP *heap = HeapAlloc(nMerge, (pCmpFcn)TempMergeCmpInts, NULL);
    for(i=0; i<nMerge; i++)
    {
	mergePosition[i]=mergeSeparator[i];
	HeapInsert(heap, (foint)i);
    }

    while(HeapSize(heap) > 0)
    {
	int which = HeapNext(heap).i;
	assert(0 <= which && which < nMerge && mergePosition[which] < mergeSeparator[which+1]);
	dest[n++] = tempMerge[mergePosition[which]++];
	if(mergePosition[which] < mergeSeparator[which+1]) HeapInsert(heap, (foint)which);
    }
    HeapFree(heap);
    return n;
}

/* Note that the algorithm below is NOT stable: it can put an element on top of
** an equal one that arrived earlier, and its merges don't break ties by arrival
** order.  With bare integers you can't tell, but don't copy this logic to sort
** records; use PileSort (further below), which is stable.
** The ints must be non-negative: the interpolation that picks a new pile's
** starting position assumes it.
*/
int PileSortInts(int *a, size_t n)	// integer-only version, which uses a "distance" metric
{
    if(n < 2) return 0;
    int p, i, numCopies = 0;
    int numPiles = (log(n))+1, pileSize=(n/numPiles), nextFreePile=0;
    //int numPiles = log(n)/log(2)+1, pileSize=numPiles, nextFreePile=0;
    int tempMerge[n], mergeSeparator[n+1], nMerge=0; // probably way too many merge separators
    PILEInts pile[numPiles];
    mergeSeparator[0]=0;

    // initialize the piles
    for(p=0; p<numPiles; p++)
    {
	pile[p].a = alloca(pileSize * sizeof(pile[0].a[0])); // it's OK not to initialize bottom and top.
    }
#define UINT_INFINITY ((unsigned int)(-1))
#define INT_INFINITY (UINT_INFINITY/2)
    // STEP 1: build piles
    // top and bottom point to the *actual* element and will be equal if there's only one element in the pile.
    // Thus the number of elements in a pile is bottom-top+1.
    for(i=0; i < n; i++)
    {
	++numCopies;
	unsigned int minDistance = UINT_INFINITY;
	int bestPile = INT_INFINITY;
	Boolean useTop = maybe;
	for(p=0;p<nextFreePile;p++)
	{
	    int top = pile[p].top, bottom = pile[p].bottom;
	    assert(0 <= top && top <= bottom && bottom < pileSize);
	    if(a[i] <= pile[p].a[top] && pile[p].a[top]-a[i] < minDistance)
	    {
		minDistance = pile[p].a[top]-a[i]; bestPile=p; useTop=true;
	    }
	    if(a[i] >= pile[p].a[bottom] && a[i]-pile[p].a[bottom] < minDistance)
	    {
		minDistance = a[i]-pile[p].a[bottom]; bestPile=p; useTop=false;
	    }
	    _compareCount += 2;
	}
	//assert(useTop != maybe);

	// need to put it in a new pile?
	if(bestPile == INT_INFINITY || (useTop && pile[bestPile].top == 0) || (!useTop && pile[bestPile].bottom == pileSize-1))
	{
	    assert(p == nextFreePile);
	    assert(0 <= nextFreePile && nextFreePile < numPiles);
	    int interpolant = pileSize*(a[i]/(double)(INT_INFINITY));
	    if(interpolant >= pileSize) interpolant = pileSize-1; // happens if a[i] == INT_INFINITY
	    pile[nextFreePile].top = pile[nextFreePile].bottom = interpolant;
	    pile[nextFreePile].a[pile[nextFreePile].top] = a[i];
	    nextFreePile++;
	    if(nextFreePile >= numPiles)
	    {
		int nItems = PileMergeInts(tempMerge + mergeSeparator[nMerge], numPiles, pile);
		numCopies += nItems;
		mergeSeparator[nMerge+1] = mergeSeparator[nMerge] + nItems;
		nMerge++;
		nextFreePile = 0;
	    }
	}
	else
	{
	    assert(0 <= bestPile && bestPile < nextFreePile);
	    if(useTop)
	    {
		assert(pile[bestPile].top > 0);
		pile[bestPile].a[--pile[bestPile].top] = a[i];
	    }
	    else
	    {
		assert(pile[bestPile].bottom < pileSize-1);
		pile[bestPile].a[++pile[bestPile].bottom] = a[i];
	    }
#if 0
	    // Check to see if the pile is getting too full
	    double topSpace = (pile[bestPile].a[pile[bestPile].top]/(double)(INT_INFINITY));
	    double bottomSpace = 1-(pile[bestPile].a[pile[bestPile].bottom]/(double)(INT_INFINITY));
	    if(topSpace < 1/(double)pileSize && bottomSpace < 1/(double)pileSize) // end elements are too close to 0 or infinity, move to mergeQueue
	    {
		int nItems = PileMergeInts(tempMerge + mergeSeparator[nMerge], numPiles, pile);
		mergeSeparator[nMerge+1] = mergeSeparator[nMerge] + nItems;
		nMerge++;
		nextFreePile = 0;
	    }
#endif
	}
    }
    // Merge what's left. Only piles below nextFreePile are in use; the rest may never have been initialized.
    int nItems = PileMergeInts(tempMerge + mergeSeparator[nMerge], nextFreePile, pile);
    numCopies += nItems;
    if(nItems > 0) // PileMergeTempsInts can't handle an empty run
    {
	mergeSeparator[nMerge+1] = mergeSeparator[nMerge] + nItems;
	nMerge++;
    }
    printf("Done with piles!  nMerge %d\n", nMerge); fflush(stdout);
    //for(i=0; i<=nMerge; i++) printf(" %d", mergeSeparator[i]);
    //printf("\n");
    //assert(false);
    // At this point, all the piles should be sorted, now it's time to merge them all together.
    nItems = PileMergeTempsInts(a, nMerge, mergeSeparator, tempMerge);
    assert(nItems == n);
    numCopies += nItems;
    return numCopies;
}

/* Generic Pile Sort: same interface as the other SortFcn's.
**
** This generic, stable version of Pile Sort, and these comments, were written
** by Claude (Anthropic's Claude Sonnet 5.5, via Claude Code) in November 2026,
** working with Wayne Hayes and building on his Pile Sort design described
** above.  Wayne asked for it to be stable, and then for it to be stable
** without recording each element's original position.
**
** OVERVIEW.  With only a comparison function there's no "distance" metric, so:
** - "closest top" means the smallest top that is > the new element, and
**   "closest bottom" means the largest bottom that is <= the new element;
** - if both exist, we use whichever of those two piles has more room;
** - we can't interpolate a pile's starting position, so each pile is a
**   circular buffer, which can grow in either direction until it's full.
** When we need a new pile and none are free, all the piles are merged into
** one sorted "run"; the runs are merged together at the end using a heap.
** There are about ln(n) piles, each holding about n/ln(n) elements.
** O(n log n); uses O(n*w) extra space (from the heap, not the stack).
**
** STABILITY.  Elements that compare equal come out in their original order.
** This is purely a property of the algorithm: we don't record any element's
** original position.  It takes three rules.
**
** 1) Within a pile: an element only goes on top if it's STRICTLY smaller than
**    the top, while an element equal to the bottom may be appended below it.
**    So equal elements are always in arrival order from top to bottom.
**
** 2) Across piles: we maintain the invariant
**	  if p < q, every element in pile p arrived before any equal element
**	  in pile q
**    so that when merging piles, ties simply go to the lowest-numbered pile.
**    To maintain it, a new element x may not go into pile p if some
**    higher-numbered pile might already hold an element equal to x.  "Might
**    hold" means top <= x <= bottom, so we only need to look at the two ends
**    of each pile, which we're looking at anyway.  So we scan the piles from
**    the highest number down, and stop at the first one whose range contains x:
**    every pile below it is off limits.  That pile itself can still take x at
**    its bottom, if x equals the bottom and there's room.  Note that a FULL
**    pile can't accept x, but it can still block the piles below it.
**
**    Why this works: when x goes into pile p, no higher pile contains x's
**    value.  So any equal element that reaches a higher pile does so later.
**    From then on that higher pile's range contains the value (a pile's range
**    only grows until it's merged away), which blocks any further equal
**    elements from going into p.  A newly created pile is the highest-numbered
**    one, and holds the latest element, so it satisfies the invariant too.
**    The range test is conservative (the pile may not really contain a value
**    equal to x), which costs us some placement choices but never stability.
**
** 3) Across runs: each run holds a contiguous stretch of the input, earlier
**    runs holding earlier elements, so when merging runs, ties go to the
**    earlier run.
**
** COST.  Every element is copied exactly 3 times: into a pile, from the pile
** into a run, and from the run back into the original array.  That's far fewer
** copies than MergeSort, CombSort or HeapSort, which matters for large
** elements, but it takes more comparisons than MergeSort.
**
** An alternative way to get stability is to record each element's original
** position (one size_t per element) and use it to break ties between piles,
** which allows any pile to be used.  Perhaps surprisingly, the rule above
** usually needs FEWER comparisons than that, even though it restricts the
** choice of pile: we stop scanning early, and we don't look at a pile's bottom
** if x is less than its top.  The exception is already-sorted or reversed
** input, probably because full piles could be skipped without any comparisons
** in the alternative, but here must be checked to see if they block.
** Measured (Nov 2026) on one sort of 4,194,304 8-byte records, millions of
** comparisons (seconds), recorded positions vs. this algorithm:
**	random			266.6 (2.50)	212.2 (2.52)
**	256 distinct values	276.9 (1.92)	221.4 (1.91)
**	4 distinct values	114.6 (0.58)	 91.1 (0.62)
**	nearly sorted		320.5 (1.39)	280.0 (1.44)
**	reversed, duplicates	173.3 (0.69)	110.2 (0.66)
**	sorted			 39.8 (0.31)	102.8 (0.61)
**	reversed		 39.8 (0.31)	 67.1 (0.44)
** In the TEST_SORTS program below (two sorts, many duplicate keys) it took
** 455M comparisons, vs. 569M with recorded positions, and 494M for an earlier
** version that wasn't stable at all; MergeSort took 174M.
*/
typedef struct {size_t head, count; char *a;} PILE; // head is the slot of the top element

// address of the k'th element (counting from the top) of a pile
static char *PileElem(const PILE *pile, size_t k, size_t pileSize, size_t w)
{
    return pile->a + w * ((pile->head + k) % pileSize);
}

// Merge (and empty) a bunch of piles into dest; returns the number of elements merged.
static size_t PileMerge(char *dest, int numPiles, PILE *pile, size_t pileSize, size_t w, pfnCmpFcn compare)
{
    size_t n=0;
    while(true)
    {
	int smallest = -1, p;
	// Must use "<", not "<=", so that ties go to the lowest-numbered pile,
	// which holds the earliest of the equal elements (stability rule 2).
	for(p=0; p < numPiles; p++)
	    if(pile[p].count > 0 && (smallest == -1 ||
		compare(PileElem(pile+p, 0, pileSize, w), PileElem(pile+smallest, 0, pileSize, w)) < 0))
		smallest = p;
	if(smallest == -1) return n;
	memcpy(dest + w*n++, PileElem(pile+smallest, 0, pileSize, w), w);
	pile[smallest].head = (pile[smallest].head + 1) % pileSize;
	pile[smallest].count--;
    }
}

// Used by the heap to compare the next unmerged element of two runs
static const char *_runData;
static size_t *_runNext, _runWidth;
static pfnCmpFcn _runCompare;
static int RunCmp(const foint X, const foint Y)
{
    int c = _runCompare(_runData + _runWidth * _runNext[X.i], _runData + _runWidth * _runNext[Y.i]);
    return c ? c : X.i - Y.i; // for stability, ties go to the earlier run
}

int PileSort(void *a, size_t n, size_t w, pfnCmpFcn compare)
{
    if(n < 2) return 0;

    int numPiles = (int)log(n) + 1, usedPiles = 0, nRuns = 0, p, r;
    size_t pileSize = n/numPiles + 1, i, merged = 0;
    size_t maxRuns = n/numPiles + 1; // every run but the last has at least numPiles elements
    char *A = (char*)a;
    char *store = Malloc(numPiles * pileSize * w);
    char *runs = Malloc(n * w); // one long array of consecutive sorted runs
    size_t *runStart = Malloc((maxRuns+1) * sizeof(runStart[0]));
    PILE pile[numPiles];
    int numCopies = 0;

    for(p=0; p<numPiles; p++) pile[p].a = store + p * pileSize * w;
    runStart[0] = 0;

    // STEP 1: build piles
    for(i=0; i < n; i++)
    {
	const char *x = A + w*i;
	int topPile = -1, bottomPile = -1;
	// Find the best pile for x.  Go from the highest-numbered pile down,
	// stopping at the first one that might hold an element equal to x: lower
	// piles are off limits (stability rule 2, above).
	for(p=usedPiles-1; p >= 0; p--)
	{
	    Boolean full = (pile[p].count == pileSize); // can't take x, but may still block
	    char *top = PileElem(pile+p, 0, pileSize, w);
	    if(compare(x, top) < 0) // strictly less, for stability (rule 1)
	    {
		if(!full && (topPile == -1 || compare(top, PileElem(pile+topPile, 0, pileSize, w)) < 0))
		    topPile = p;
		continue; // x is outside this pile's range, so no need to look at its bottom
	    }
	    char *bottom = PileElem(pile+p, pile[p].count-1, pileSize, w);
	    int c = compare(x, bottom);
	    if(c >= 0 && !full && (bottomPile == -1 ||
		compare(bottom, PileElem(pile+bottomPile, pile[bottomPile].count-1, pileSize, w)) > 0))
		bottomPile = p;
	    if(c <= 0) break; // top <= x <= bottom: this pile blocks all lower ones
	}

	if(topPile != -1 && bottomPile != -1) // choose the one with more room
	{
	    if(pile[topPile].count <= pile[bottomPile].count) bottomPile = -1;
	    else topPile = -1;
	}

	if(topPile != -1) // put it on top
	{
	    p = topPile;
	    pile[p].head = (pile[p].head + pileSize - 1) % pileSize;
	    pile[p].count++;
	    memcpy(PileElem(pile+p, 0, pileSize, w), x, w);
	}
	else if(bottomPile != -1) // append to the bottom
	{
	    p = bottomPile;
	    memcpy(PileElem(pile+p, pile[p].count++, pileSize, w), x, w);
	}
	else // it needs a new pile
	{
	    if(usedPiles == numPiles) // none free: merge them all into a new run
	    {
		size_t nItems = PileMerge(runs + w*merged, usedPiles, pile, pileSize, w, compare);
		merged += nItems;
		numCopies += nItems;
		runStart[++nRuns] = merged;
		usedPiles = 0;
	    }
	    p = usedPiles++;
	    pile[p].head = 0;
	    pile[p].count = 1;
	    memcpy(pile[p].a, x, w);
	}
	++numCopies;
    }
    size_t nItems = PileMerge(runs + w*merged, usedPiles, pile, pileSize, w, compare);
    merged += nItems;
    numCopies += nItems;
    runStart[++nRuns] = merged;
    assert(merged == n && nRuns <= maxRuns);

    // STEP 2: merge the runs back into the original array
    size_t *runNext = Malloc(nRuns * sizeof(runNext[0]));
    _runData = runs;
    _runNext = runNext;
    _runWidth = w;
    _runCompare = compare;
    HEAP *heap = HeapAlloc(nRuns, (pCmpFcn)RunCmp, NULL);
    for(r=0; r<nRuns; r++)
    {
	foint f; f.i = r;
	runNext[r] = runStart[r];
	HeapInsert(heap, f);
    }
    for(i=0; HeapSize(heap) > 0; i++)
    {
	foint f = HeapNext(heap);
	memcpy(A + w*i, runs + w*runNext[f.i]++, w);
	++numCopies;
	if(runNext[f.i] < runStart[f.i+1]) HeapInsert(heap, f);
    }
    assert(i == n);

    HeapFree(heap);
    free(runNext);
    free(runStart);
    free(runs);
    free(store);
    return numCopies;
}

/* insertion sort. Good for lists that are already "nearly" sorted, ie no
** element is too far from it's sorted position.  Return the number of
** copies that had to be done (which on average would be O(n^2).)
*/
int InsertionSort(void *a, size_t n, size_t w, pfnCmpFcn compare)
{
    int i, numCopies = 0;
    char *A = (char*) a;
    void *x = alloca(w);     /* temporary storage */

    for(i=1; i < n; i++)
    {
	int j = i;  /* j scans left to find i's position */
	memcpy(x, (void*)(A + w*i), w);
	numCopies++;
	while(j >= 1 && compare((void*)(A + w*(j-1)), x) > 0)
	{
	    memcpy((void*)(A + w*j), (void*)(A + w*(j-1)), w);
	    j--;
	    numCopies++;
	}
	memcpy((void*)(A + w*j), x, w);
	numCopies++;
    }
    return numCopies;
}
//Insertion sort for integers only. Less overheads
void InsertionSortInt(int arr[], size_t n)
{
   //printf("%d\n",n);
   int i, key, j;
   for (i = 1; i < n; i++)
   {
       key = arr[i];
       j = i-1;

       /* Move elements of arr[0..i-1], that are
          greater than key, to one position ahead
          of their current position */
       while (j >= 0 && arr[j] > key)
       {
           arr[j+1] = arr[j];
           j = j-1;
       }
       arr[j+1] = key;
   }
}

/* Merge two sorted lists.
*/
static int merge(void *L1, size_t n1,    void *L2, size_t n2,
    size_t width, void *Dest, pfnCmpFcn CmpFcn)
{
    int numCopies = 0;
    int id = 0, i1 = 0, i2 = 0; /* indexes into the lists */
    char *l1 = L1, *l2 = L2, *dest = Dest;
    while(i1 < n1 || i2 < n2)
    {
	char *pd = dest + id * width,   *p1 = l1 + i1 * width,
					*p2 = l2 + i2 * width;
	if(i2 >= n2)    /* list 2 is finished, just add from list 1 */
	{
	    memcpy(pd, p1, width);
	    numCopies++;
	    i1++;
	}
	else if(i1 >= n1)   /* list 1 is finished, just add from list 2 */
	{
	    memcpy(pd, p2, width);
	    numCopies++;
	    i2++;
	}
	else if(CmpFcn(p1, p2) <= 0)
	{
	    memcpy(pd, p1, width);
	    numCopies++;
	    i1++;
	}
	else
	{
	    memcpy(pd, p2, width);
	    numCopies++;
	    i2++;
	}
	id++;
    }
    assert(i1 == n1 && i2 == n2);
    return numCopies;
}

int MergeSort(void *a, size_t n, size_t w, pfnCmpFcn compare)
{
    int numCopies = 0;
    char *A = (char*)a;
    void *tmp = alloca(n*w);
    assert(n >= 0);
    if(n < 2)
	return 0;
    numCopies += MergeSort(a, n/2, w, compare);
    numCopies += MergeSort((void*)(A + (n/2)*w), n-n/2, w, compare);
    numCopies += merge(a, n/2, (void*)(A+(n/2)*w), n-n/2, w, tmp, compare);
    memcpy(a, tmp, n*w);
    ++numCopies;
    return numCopies;
}


/* simple, in-place approximately nlogn sort; see BYTE Apr 1991.
 * I don't think it's been *proven* to be nlogn, although it
 * certainly tends to be, because until gap=1, the outer loop
 * has at most log_{COMB_SHRINK}(n) iterations.  However, often
 * more than one iteration is required at gap=1, and I don't know
 * that anybody has shown that the number of gap=1 iterations is
 * bounded by a constant.
 */
#define COMB_SHRINK 1.3
int CombSort(void *a, size_t n, size_t w, pfnCmpFcn compare)
{
     /* Possible improvement: An initial gap=n/1.3 results in the first gap
      * being about 0.77n.  The for loop, then, only
      * considers elements from 0 through 0.23n for swapping with elements
      * in 0.77n through n.  This means that elements from 0.23n to 0.77n
      * aren't even considered during the first outer loop.  It makes
      * sense (to me) that it would be best if everybody was considered
      * for swapping from the start.  This would mean we want a gap of
      * n/2 in the first iteration, rather than gap/1.3.
      */
    int gap = MAX(n/COMB_SHRINK,1), gap1 = 0;
    char *A = (char*)a;
    void *tmp = alloca(w);
    Boolean done;
    int numCopies = 0;
    do
    {
	int i;
	done = true;
	/* Experiment has shown that the sequence of gaps starting at 11
	 * gives better results than the sequence starting at 9 or 10.
	 */
	if(gap == 9 || gap == 10)
	    gap = 11;
	if(gap == 1)
	    ++gap1;
	for(i=0; i < n-gap; i++)
	    if(compare((void*)(A + w*i), (void*)(A + w*(i+gap))) > 0)
	    {
		memcpy(tmp, (void*)(A + w*i), w);
		memcpy((void*)(A + w*i), (void*)(A + w*(i+gap)), w);
		memcpy((void*)(A + w*(i+gap)), tmp, w);
		numCopies += 3;
		done = false;
	    }
	gap = MAX(gap/COMB_SHRINK,1);
    } until(gap1 > 0 && done);
    /*printf("[gap1=%d]",gap1);*/
    return numCopies;
}

/*
** To make heads or tails of this function, take a look at heap.c.
*/
int HeapSort(void *a, size_t n, size_t w, pfnCmpFcn compare)
{
    int i, numCopies = 0, heapSize;
    char *A = a, *tmp = alloca(w), *top = alloca(w), *bubble = alloca(w);
    A -= w;		/* make A[1] the first element */

    /* The first element is a heap by itself, so start at 2. */
    heapSize = 1;
    for(i=2; i <= n; i++)
    {
	/* HeapInsert(A[i]); */
	int place = ++heapSize;	/* the new element starts at the bottom */
	/*
	** bubble it up.  Note the comparison is backwards; this is
	** necessary since we place things backwards at the next stage,
	** so we want to secretly create a *largest* at top heap, then
	** when we're done, the top of the heap goes to the back, etc.
	*/
	while(place > 1 && compare(A + (place/2)*w, A + place*w) <= 0)
	{
	    memcpy(tmp, A + place*w, w);
	    memcpy(A + place*w, A + (place/2)*w, w);
	    place /= 2;
	    memcpy(A + place*w, tmp, w);
	    numCopies += 3;
	}
    }

    /* Now we have a heap.  Build the sorted array. */
    for(i=0; i < n; i++)
    {
	int place, child1, child2;
	memcpy(top, A+w, w);	/* store the top element for later */
	memcpy(bubble, A + heapSize*w, w);
	memcpy(A+w, bubble, w);
	numCopies += 3;
	--heapSize;
	place = 1;
	/* bubble it down */
	while(child1 = 2*place, child2 = child1 + 1, child1 <= heapSize)
	{
	    int smallestChild = child1;
	    if(child2 <= heapSize && compare(A + child2*w, A + child1*w) >= 0)
		smallestChild = child2;
	    if(compare(bubble, A + smallestChild*w) > 0)	/* done */
		break;

	    /* exchange with smallest child */
	    memcpy(A + place*w, A + smallestChild*w, w);
	    place = smallestChild;
	    memcpy(A + place*w, bubble, w);
	    numCopies += 3;
	}
	/* Finally, put the top in the proper place */
	memcpy(A + (heapSize+1)*w, top, w);
	++numCopies;
    }

    return numCopies;
}


#ifndef PointerSort
static pfnCmpFcn _PointerSortCompare;
static int _PointerSortCmp(const void *x, const void *y)
{
    const char *const*X = x, *const*Y = y;
    return _PointerSortCompare(*X, *Y);
}

int PointerSort(SortFcn Sort, void *a, size_t n, size_t w, pfnCmpFcn compare)
{
    void *tmp = alloca(w);
    char *A = a, *p[n];
    int i, k, start, numCopies = 0;

    if(n < 2)
	return 0;

    for(i=0; i<n; i++)
	p[i] = A+i*w;
    _PointerSortCompare = compare;
    numCopies += Sort(p, n, sizeof(p[0]), _PointerSortCmp);

    /*
    ** This is the permutation array.  I[k] contains the j for which a[j]
    ** should be moved to a[k].  So, for example, if I[0] = 5, then the
    ** smallest element is currently in a[5], and should be moved to a[0],
    ** and of course *(p[0]) == a[5].
    */
#define I(k) ((p[k]-A)/w)
    /*printf("I = "); for(i=0; i<n; i++) printf("%d ", I(i)); printf("\n");*/

    /*
    ** Now do the actual permutation.
    ** start = the first element in a cycle.
    ** p[k] is set to NULL once it's a[k] is properly assigned.
    ** This is O(n).  Each element is touched no more than twice; once
    ** when start touches it, and once when it is touched by a cycle.
    ** We only need to loop to n-1 because the last one is either in
    ** it's proper place, or it was part of a previous cycle.
    */
    for(start = 0; start < n-1; start++)
    {
	if(p[start] == NULL || I(start) == start)
	    continue;

	/* temporarily store the beginning of the cycle, move it to the
	** end later. */
	memcpy(tmp, A + start*w, w); ++numCopies;
	k = start; /* now do the cycle */
	do
	{
	    int oldK = k;
	    memcpy(A + k*w, A + I(k)*w, w); ++numCopies;
	    k = I(k);
	    p[oldK] = NULL;
	} until(I(k) == start);
	memcpy(A + k*w, tmp, w); ++numCopies;
	p[k] = NULL;
    }
#undef I
    return numCopies;
}
#endif

#define FREDERICKS 1
#if FREDERICKS
/*
** This algorithm is called Frederickson's algorithm.  It was outlined
** in the following talk.  I didn't understand the author's extension
** very well, but Frederickson's algorithm is quite trivial.  In essence,
** it is this:
**
**     Split the input into n/logn blocks of size logn.  Keep a priority
**     queue P of the smallest elements in each block (P has size n/logn).
**     After deleting an element in P, delete the same element from its
**     block, and use a brute-force search of the block for the next
**     smallest element.
**
** Here is the abstract of the talk.
**
**            Department of Computer Science, University of Toronto
**          (SF = Sandford Fleming Building, 10 King's College Road)
**
**        -------------------------------------------------------------
**
**                                   THEORY
**                 SF1101, at 11:00 a.m., Friday 12 June 1998
**
**                                 Theis Rauhe
**                         BRICS, University of Aarhus
**
**                 "Optimal Time-Space Trade-offs for Sorting"
**
** This talk addresses the fundamental problem of sorting n elements from an
** ordered universe in a sequential model of computation and in particular
** consider the time-space trade-off (product of time and space) for this
** problem.  Beame has shown a lower bound of $Omega(n^2)$ for this product
** leaving a gap of a logarithmic factor up to the previously best known upper
** bound of $O(n^2log n)$ due to Frederickson.
**
** In this talk a comparison based sorting algorithm which closes this gap is
** presented.  This algorithm obtain the optimal time-space product $O(n^2)$
** for the full range of space bounds between $log n$ and $n/log n$.
*/

static SET *fredDone;
static void *fredArray;
static int fredSize, fredWidth;
static pfnCmpFcn fredCmp;

/*
** Compare Fred things using the array index rather than the objects
** themselves.  Use fredArray and fredCmp to call the "real" comparison
** function.
*/
static int FredCmpIndex(const foint fi, const foint fj)
{
    int i = fi.i, j = fj.i;
    int cmp = fredCmp((char*)fredArray + i*fredWidth,
	(char*)fredArray + j*fredWidth);
    assert(0 <= i && i < fredSize);
    assert(0 <= j && j < fredSize);
    if(cmp == 0)
	return i - j;
    else
	return cmp;
}

/*
** Use brute force to find the found element in a block, starting at
** "start" and going for n elements.  Return index of biggest, relative
** to the entire array, ie, start <= biggest < start + n.
*/
static int BruteSearch(int start, int end)
{
    int i, found = -1;

    for(i=start; i<end; i++)
    {
	if(SetIn(fredDone, i))
	    continue;

	if(found == -1)
	{
	    found = i;
	    continue;
	}

	if(FredCmpIndex((foint)i, (foint)found) < 0)
	    found = i;
    }
    return found;
}

int FredSort(void *a, size_t n, size_t w, pfnCmpFcn compare)
{
    int i, block, numCopies=0;
    int blockSize = Log2(n);	/* floor(log_2(n)) */
    int numBlocks = (n+blockSize-1)/blockSize;
    HEAP *P = HeapAlloc(numBlocks, FredCmpIndex, NULL);
    int outputNext = 0;
    char output[n*w];

	int next, nextsBlock, found;

    fredArray = a;
    fredSize = n;
    fredWidth = w;
    fredCmp = compare;
    fredDone = SetAlloc(n);

    /* Initialize the Heap */
    for(block=0; block < numBlocks; block++)
    {
	found = BruteSearch(block*blockSize, MIN((block+1)*blockSize, n));
	assert(block*blockSize <= found && found < MIN((block+1)*blockSize, n));
	HeapInsert(P, (foint)found);
    }
    assert(HeapSize(P) == numBlocks);

    for(i=0; i<n; i++)
    {
	next = HeapNext(P).i;
	assert(0 <= next && next < n);
	nextsBlock = next / blockSize;
	assert(0 <= nextsBlock && nextsBlock < numBlocks);
	memcpy(output + w*outputNext++, (char*)a + w*next, w);
	++numCopies;

	assert(!SetIn(fredDone, next));
	SetAdd(fredDone, next);
	found=BruteSearch(nextsBlock*blockSize,
	    MIN((nextsBlock+1)*blockSize, n));
	if(found != -1)
	    HeapInsert(P, (foint)found);
    }

    /* sanity checks */
    for(i=0; i<n; i++)
	assert(SetIn(fredDone,i));
    assert(HeapSize(P) == 0);

    /*
    ** Copy the output back into the correct places.
    */
    memcpy(a, output, n*w);
    numCopies += n;
    return numCopies;
}
#endif	/* FREDERICKS */

/***************************   END OF LIBRARY SOURCE *******************/

#if TEST_SORTS
#include <stdio.h>

#define TOTBYTES (4*1024*1024)
#define ElementSize (1)
#define NUM (TOTBYTES/ElementSize)

#define PreSortCmp ElementCmpj
#define SortCmp ElementCmpi

typedef struct _foo {unsigned char i,j; unsigned char f[ElementSize]; } ELEMENT;

static __attribute__((unused)) int ElementCmp(const void *X, const void *Y)
{
    const ELEMENT *x = X, *y = Y;
    int i;
    _compareCount++;
    if(x->i != y->i) return x->i - y->i;
    if(x->j != y->j) return x->j - y->j;

    for(i=0; i<ElementSize; i++)
	if(x->f[i] != y->f[i]) return x->f[i] - y->f[i];

    return 0;
}

static int ElementCmpi(const void *X, const void *Y)
{
    const ELEMENT *x = X, *y = Y;
    _compareCount++;
    return x->i - y->i;
}

static int ElementCmpj(const void *X, const void *Y)
{
    const ELEMENT *x = X, *y = Y;
    _compareCount++;
    return x->j - y->j;
}

static __attribute__((unused)) void ElementDmp(const ELEMENT *x)
{
    int i;
    printf("%d.%d[", x->i, x->j);
    for(i=0; i<ElementSize; i++)
	printf("%d,", x->f[i]);
    printf("]");
}

int main(void)
{
    /*assert(NUM*ElementSize <= 16384*1024);*/
    srand48(time(0) + getpid());
    printf("NUM %d, ElementSize %lud\n", NUM, sizeof(ELEMENT));
do
{
    ELEMENT a[NUM], b[NUM], c[NUM], d[NUM], e[NUM], f[NUM], g[NUM];
    int ints[NUM];
    int i, cpCnt, cm;
    double t, secs;
    Boolean stable;

    printf("generating random list..."); fflush(stdout);

    for(i=0; i<NUM; i++)
    {
	int j;
	ints[i] = lrand48() % INT_INFINITY;
	a[i].i = ints[i];
	a[i].j = lrand48();
	for(j=0; j<ElementSize; j += 4)
	{
	    unsigned l = lrand48();
	    a[i].f[j] = l;
	    a[i].f[j+1] = (l <<= 8);
	    a[i].f[j+2] = (l <<= 8);
	    a[i].f[j+3] = (l <<= 8);
	}
	/*ElementDmp(a+i); printf("\n");*/
    }

    memcpy(b, a, sizeof(ELEMENT)*NUM);
    memcpy(c, a, sizeof(ELEMENT)*NUM);
    memcpy(d, a, sizeof(ELEMENT)*NUM);
    memcpy(e, a, sizeof(ELEMENT)*NUM);
    memcpy(f, a, sizeof(ELEMENT)*NUM);
    memcpy(g, a, sizeof(ELEMENT)*NUM);

    printf("Starting sorts\n");

    printf("Pile Sort: "); fflush(stdout);
    _compareCount = 0;
    t = uTime();
    cpCnt = PileSortInts(ints, NUM);
    cm = _compareCount; secs = uTime()-t;
    stable = true; // NOT checked: equal ints are indistinguishable (see PileSortInts)
    for(i=0; i<NUM-1; i++)
    {
	assert(ints[i]<= ints[i+1]);
    }
    printf("cm: %9d cp: %9d (%gs) %sstable\n", cm, cpCnt, secs,
	stable?"":"un");

    printf("qsort: "); fflush(stdout);
    _compareCount = 0;
    t = uTime();
    cpCnt = PointerSort(QuickSort,a, NUM, sizeof(ELEMENT), PreSortCmp);
    cpCnt += PointerSort(QuickSort,a, NUM, sizeof(ELEMENT), SortCmp);
    cm = _compareCount; secs = uTime()-t;
    stable = true;
    for(i=0; i<NUM-1; i++)
    {
	assert(SortCmp(a+i, a+i+1) <= 0);
	if(SortCmp(a+i, a+i+1) == 0 && PreSortCmp(a+i, a+i+1) > 0)
	{
	    stable = false;
	    break;
	}
    }
    printf("cm: %9d cp: %9d (%gs) %sstable\n", cm, cpCnt, secs,
	stable?"":"un");

    printf("psort: "); fflush(stdout);
    _compareCount = 0;
    t = uTime();
    cpCnt = PointerSort(PileSort,e, NUM, sizeof(ELEMENT), PreSortCmp);
    cpCnt += PointerSort(PileSort,e, NUM, sizeof(ELEMENT), SortCmp);
    cm = _compareCount; secs = uTime()-t;
    stable = true;
    for(i=0; i<NUM-1; i++)
    {
	assert(SortCmp(e+i, e+i+1) <= 0);
	assert(SortCmp(a+i,e+i) == 0);
	if(SortCmp(e+i, e+i+1) == 0 && PreSortCmp(e+i, e+i+1) > 0)
	    stable = false; // but keep checking that it's sorted
    }
    printf("cm: %9d cp: %9d (%gs) %sstable\n", cm, cpCnt, secs,
	stable?"":"un");

    printf("csort: "); fflush(stdout);
    _compareCount = 0;
    t = uTime();
    cpCnt = PointerSort(CombSort,b, NUM, sizeof(ELEMENT), PreSortCmp);
    cpCnt += PointerSort(CombSort,b, NUM, sizeof(ELEMENT), SortCmp);
    cm = _compareCount; secs = uTime()-t;
    stable = true;
    for(i=0; i<NUM-1; i++)
    {
	assert(SortCmp(b+i, b+i+1) <= 0);
        /*ElementDmp(b+i); printf("\n");*/
	assert(SortCmp(a+i,b+i) == 0);
	if(SortCmp(b+i, b+i+1) == 0 && PreSortCmp(b+i, b+i+1) > 0)
	{
	    stable = false;
	    break;
	}
    }
    printf("cm: %9d cp: %9d (%gs) %sstable\n", cm, cpCnt, secs,
	stable?"":"un");

    printf("msort: "); fflush(stdout);
    _compareCount = 0;
    t = uTime();
    cpCnt = PointerSort(MergeSort,c, NUM, sizeof(ELEMENT), PreSortCmp);
    cpCnt += PointerSort(MergeSort,c, NUM, sizeof(ELEMENT), SortCmp);
    cm = _compareCount; secs = uTime()-t;
    stable = true;
    for(i=0; i<NUM-1; i++)
    {
	assert(SortCmp(c+i, c+i+1) <= 0);
	/*ElementDmp(c+i); printf("\n");*/
	assert(SortCmp(a+i,c+i) == 0);
	if(SortCmp(c+i, c+i+1) == 0 && PreSortCmp(c+i, c+i+1) > 0)
	{
	    stable = false;
	    break;
	}
    }
    printf("cm: %9d cp: %9d (%gs) %sstable\n", cm, cpCnt, secs,
	stable?"":"un");

    printf("hsort: "); fflush(stdout);
    _compareCount = 0;
    t = uTime();
    cpCnt = PointerSort(HeapSort,d, NUM, sizeof(ELEMENT), PreSortCmp);
    cpCnt += PointerSort(HeapSort,d, NUM, sizeof(ELEMENT), SortCmp);
    cm = _compareCount; secs = uTime()-t;
    stable = true;
    for(i=0; i<NUM-1; i++)
    {
	assert(SortCmp(d+i, d+i+1) <= 0);
	/*ElementDmp(d+i); printf("\n");*/
	assert(SortCmp(a+i,d+i) == 0);
	if(SortCmp(d+i, d+i+1) == 0 && PreSortCmp(d+i, d+i+1) > 0)
	{
	    stable = false;
	    break;
	}
    }
    printf("cm: %9d cp: %9d (%gs) %sstable\n", cm, cpCnt, secs,
	stable?"":"un");

#if 0
    printf("isort: "); fflush(stdout);
    _compareCount = 0;
    t = uTime();
    cpCnt = PointerSort(InsertionSort, e, NUM, sizeof(ELEMENT), PreSortCmp);
    cpCnt += PointerSort(InsertionSort, e, NUM, sizeof(ELEMENT), SortCmp);
    cm = _compareCount; secs = uTime()-t;
    stable = true;
    for(i=0; i<NUM-1; i++)
    {
	assert(SortCmp(e+i, e+i+1) <= 0);
	/*ElementDmp(e+i); printf("\n");*/
	assert(SortCmp(a+i,e+i) == 0);
	if(SortCmp(e+i, e+i+1) == 0 && PreSortCmp(e+i, e+i+1) > 0)
	{
	    stable = false;
	    break;
	}
    }
    printf("cm: %9d cp: %9d (%gs) %sstable\n", cm, cpCnt, secs,
	stable?"":"un");
#endif

#if 0	/* FIXME: FredSort has an infinite loop */
    printf("fsort: "); fflush(stdout);
    _compareCount = 0;
    t = uTime();
    cpCnt = FredSort(f, NUM, sizeof(ELEMENT), PreSortCmp);
    cpCnt += FredSort(f, NUM, sizeof(ELEMENT), SortCmp);
    cm = _compareCount; secs = uTime()-t;
    stable = true;
    for(i=0; i<NUM-1; i++)
    {
	assert(SortCmp(f+i, f+i+1) <= 0);
        /*ElementDmp(b+i); printf("\n");*/
	assert(SortCmp(a+i,f+i) == 0);
	if(SortCmp(f+i, f+i+1) == 0 && PreSortCmp(f+i, f+i+1) > 0)
	{
	    stable = false;
	    break;
	}
    }
    printf("cm: %9d cp: %9d (%gs) %sstable\n", cm, cpCnt, secs,
	stable?"":"un");
#endif

#if 0	/* David Neto's FredSort */
    printf("dfsort: "); fflush(stdout);
    _compareCount = 0;
    t = uTime();
    cpCnt = FredSort(g, NUM, sizeof(ELEMENT), PreSortCmp);
    cpCnt += FredSort(g, NUM, sizeof(ELEMENT), SortCmp);
    cm = _compareCount; secs = uTime()-t;
    stable = true;
    for(i=0; i<NUM-1; i++)
    {
	assert(SortCmp(g+i, g+i+1) <= 0);
        /*ElementDmp(b+i); printf("\n");*/
	assert(SortCmp(a+i,g+i) == 0);
	if(SortCmp(g+i, g+i+1) == 0 && PreSortCmp(g+i, g+i+1) > 0)
	{
	    stable = false;
	    break;
	}
    }
    printf("cm: %9d cp: %9d (%gs) %sstable\n", cm, cpCnt, secs,
	stable?"":"un");
#endif
} while(1);
    return 0;
}
#endif	/* TEST_SORTS */
#ifdef __cplusplus
} // end extern "C"
#endif
