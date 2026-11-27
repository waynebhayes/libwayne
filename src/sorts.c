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

/* Bookend Sort
** Named for what it does: each bookend can grow at either end.  Originally called
** "Garage Sort", and then "Pile Sort"; it was renamed because "pile sorting"
** is a common term in psychology for grouping cards by concept, which has
** nothing to do with ordering.
** The original description below calls the sorted stacks of items "piles";
** the code calls them "bookends".
**
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
typedef struct {int top, bottom, *a;} BOOKENDInts;

// Merge a bunch of bookends into destination; you must ensure it has space
// Returns the number of things merged.
static int BookendMergeInts(int *dest, int numBookends, BOOKENDInts *bookend)
{
    Boolean done = false;
    int n=0;
    while(!done)
    {
	done = true;
	int smallest = -1, p;
	for(p=0; p < numBookends; p++)
	{
	    int top = bookend[p].top, bottom = bookend[p].bottom;
	    if(top <= bottom)
	    {
		done = false;
		if(smallest == -1) smallest = p;
		else
		{
		    _compareCount++;
		    if(bookend[p].a[top] < bookend[smallest].a[bookend[smallest].top]) smallest = p;
		}
	    }
	}
	assert(done || smallest != -1);
	if(!done) dest[n++] = bookend[smallest].a[bookend[smallest].top++];
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
static int BookendMergeTempsInts(int *dest, int nMerge, int mergeSeparator[nMerge+1], int *tempMerge)
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
** records; use BookendSort (further below), which is stable.
** The ints must be non-negative: the interpolation that picks a new bookend's
** starting position assumes it.
*/
int BookendSortInts(int *a, size_t n)	// integer-only version, which uses a "distance" metric
{
    if(n < 2) return 0;
    int p, i, numCopies = 0;
    int numBookends = (log(n))+1, bookendSize=(n/numBookends), nextFreeBookend=0;
    //int numBookends = log(n)/log(2)+1, bookendSize=numBookends, nextFreeBookend=0;
    int tempMerge[n], mergeSeparator[n+1], nMerge=0; // probably way too many merge separators
    BOOKENDInts bookend[numBookends];
    mergeSeparator[0]=0;

    // initialize the bookends
    for(p=0; p<numBookends; p++)
    {
	bookend[p].a = alloca(bookendSize * sizeof(bookend[0].a[0])); // it's OK not to initialize bottom and top.
    }
#define UINT_INFINITY ((unsigned int)(-1))
#define INT_INFINITY (UINT_INFINITY/2)
    // STEP 1: build bookends
    // top and bottom point to the *actual* element and will be equal if there's only one element in the bookend.
    // Thus the number of elements in a bookend is bottom-top+1.
    for(i=0; i < n; i++)
    {
	++numCopies;
	unsigned int minDistance = UINT_INFINITY;
	int bestBookend = INT_INFINITY;
	Boolean useTop = maybe;
	for(p=0;p<nextFreeBookend;p++)
	{
	    int top = bookend[p].top, bottom = bookend[p].bottom;
	    assert(0 <= top && top <= bottom && bottom < bookendSize);
	    if(a[i] <= bookend[p].a[top] && bookend[p].a[top]-a[i] < minDistance)
	    {
		minDistance = bookend[p].a[top]-a[i]; bestBookend=p; useTop=true;
	    }
	    if(a[i] >= bookend[p].a[bottom] && a[i]-bookend[p].a[bottom] < minDistance)
	    {
		minDistance = a[i]-bookend[p].a[bottom]; bestBookend=p; useTop=false;
	    }
	    _compareCount += 2;
	}
	//assert(useTop != maybe);

	// need to put it in a new bookend?
	if(bestBookend == INT_INFINITY || (useTop && bookend[bestBookend].top == 0) || (!useTop && bookend[bestBookend].bottom == bookendSize-1))
	{
	    assert(p == nextFreeBookend);
	    assert(0 <= nextFreeBookend && nextFreeBookend < numBookends);
	    int interpolant = bookendSize*(a[i]/(double)(INT_INFINITY));
	    if(interpolant >= bookendSize) interpolant = bookendSize-1; // happens if a[i] == INT_INFINITY
	    bookend[nextFreeBookend].top = bookend[nextFreeBookend].bottom = interpolant;
	    bookend[nextFreeBookend].a[bookend[nextFreeBookend].top] = a[i];
	    nextFreeBookend++;
	    if(nextFreeBookend >= numBookends)
	    {
		int nItems = BookendMergeInts(tempMerge + mergeSeparator[nMerge], numBookends, bookend);
		numCopies += nItems;
		mergeSeparator[nMerge+1] = mergeSeparator[nMerge] + nItems;
		nMerge++;
		nextFreeBookend = 0;
	    }
	}
	else
	{
	    assert(0 <= bestBookend && bestBookend < nextFreeBookend);
	    if(useTop)
	    {
		assert(bookend[bestBookend].top > 0);
		bookend[bestBookend].a[--bookend[bestBookend].top] = a[i];
	    }
	    else
	    {
		assert(bookend[bestBookend].bottom < bookendSize-1);
		bookend[bestBookend].a[++bookend[bestBookend].bottom] = a[i];
	    }
#if 0
	    // Check to see if the bookend is getting too full
	    double topSpace = (bookend[bestBookend].a[bookend[bestBookend].top]/(double)(INT_INFINITY));
	    double bottomSpace = 1-(bookend[bestBookend].a[bookend[bestBookend].bottom]/(double)(INT_INFINITY));
	    if(topSpace < 1/(double)bookendSize && bottomSpace < 1/(double)bookendSize) // end elements are too close to 0 or infinity, move to mergeQueue
	    {
		int nItems = BookendMergeInts(tempMerge + mergeSeparator[nMerge], numBookends, bookend);
		mergeSeparator[nMerge+1] = mergeSeparator[nMerge] + nItems;
		nMerge++;
		nextFreeBookend = 0;
	    }
#endif
	}
    }
    // Merge what's left. Only bookends below nextFreeBookend are in use; the rest may never have been initialized.
    int nItems = BookendMergeInts(tempMerge + mergeSeparator[nMerge], nextFreeBookend, bookend);
    numCopies += nItems;
    if(nItems > 0) // BookendMergeTempsInts can't handle an empty run
    {
	mergeSeparator[nMerge+1] = mergeSeparator[nMerge] + nItems;
	nMerge++;
    }
    //printf("Done with bookends!  nMerge %d\n", nMerge); fflush(stdout);
    //for(i=0; i<=nMerge; i++) printf(" %d", mergeSeparator[i]);
    //printf("\n");
    //assert(false);
    // At this point, all the bookends should be sorted, now it's time to merge them all together.
    nItems = BookendMergeTempsInts(a, nMerge, mergeSeparator, tempMerge);
    assert(nItems == n);
    numCopies += nItems;
    return numCopies;
}

/* Generic Bookend Sort: same interface as the other SortFcn's.
**
** This generic, stable version of Bookend Sort, and these comments, were written
** by Claude (Anthropic's Claude Sonnet 5.5, via Claude Code) in November 2026,
** working with Wayne Hayes and building on his Bookend Sort design described
** above.  Wayne asked for it to be stable, then for it to be stable without
** recording each element's original position, and then to try nesting the
** bookends.
**
** OVERVIEW.  The bookends are kept NESTED: each one's range of values lies
** inside the range of the one before it.  That is,
**	if p < q, top(p) <= top(q) and bottom(q) <= bottom(p).
** A new element x is placed as follows.  Let k be the first (lowest-numbered)
** bookend whose range does NOT contain x, ie., x < top(k) or x > bottom(k);
** because of nesting, no later bookend contains x either, so k is found by
** binary search.  Then
** - if x equals the bottom of bookend k-1, and there's room, append it there;
** - otherwise put x on bookend k: on top if x < top(k), else at the bottom.
**   If there's no bookend k, start a new one.
** Both choices preserve nesting.  If bookend k is full, or a new one is needed
** and none is free, all the bookends are merged into one sorted "run" and we
** start over (putting x anywhere else would break the nesting).  The runs are
** merged together at the end using a heap.
**
** We can't interpolate a bookend's starting position (there's no "distance"
** metric with only a comparison function), so each bookend is a circular
** buffer, which can grow in either direction until it's full.  Note that
** nesting dictates which bookend gets x, so unlike BookendSortInts, we don't
** look for the "closest" top or bottom.
**
** MERGING BOOKENDS.  Nesting means the tops are already in increasing order
** when a merge starts, so we get a sorted list of bookends for free.  We take
** the top of the first one, then re-insert that bookend into the list by binary
** search, first checking whether it can simply stay first.  That costs about
** log(q) comparisons per element for q bookends, instead of q-1 for a scan of
** all the tops.
**
** CREDITS for the binary searches.  Using binary search to PLACE an element
** among nested lists is Skiena's idea (1988; see RELATED WORK below), not new
** here; Wayne asked to try nesting the bookends.  Using binary search in the
** MERGE, to exploit the order that nesting gives us for free, was Claude's
** idea (Nov 2026), as was the handling of equal elements that keeps the nested
** placement stable.  Binary insertion is a standard technique, and we haven't
** checked whether this use of it has been published.
**
** There are BOOKEND_FACTOR*(ln(n)+1) bookends, holding about n elements in all.
** O(n log n); uses O(n*w) extra space (from the heap, not the stack).
**
** STABILITY.  Elements that compare equal come out in their original order.
** This is purely a property of the algorithm: we don't record any element's
** original position.  It takes three rules.
**
** 1) Within a bookend: an element only goes on top if it's STRICTLY smaller
**    than the top, while an element equal to the bottom may be appended below
**    it.  So equal elements are always in arrival order from top to bottom.
**
** 2) Across bookends: we maintain the invariant
**	  if p < q, every element in bookend p arrived before any equal element
**	  in bookend q
**    so that when merging bookends, ties simply go to the lowest-numbered one.
**    To maintain it, a new element x may not go into bookend p if some
**    higher-numbered bookend contains x in its range (and so might hold an
**    element equal to x).  Nesting gives us this for free: x goes into
**    bookend k or k-1, and by the choice of k, no bookend numbered k or higher
**    contains x.
**
**    Why the invariant follows: when x goes into bookend p, no higher bookend
**    contains x's value.  So any equal element that reaches a higher bookend
**    does so later.  From then on that higher bookend's range contains the
**    value (a bookend's range only grows until it's merged away), which blocks
**    any further equal elements from going into p.  A newly created bookend is
**    the highest-numbered one, and holds the latest element, so it satisfies
**    the invariant too.
**
**    Note the choice of k-1 when x equals its bottom: lower bookends may have
**    the same bottom, but putting x in one of them would break the rule.
**
** 3) Across runs: each run holds a contiguous stretch of the input, earlier
**    runs holding earlier elements, so when merging runs, ties go to the
**    earlier run.
**
** COST.  Every element is copied exactly 3 times: into a bookend, from the
** bookend into a run, and from the run back into the original array.  That's
** far fewer copies than MergeSort, CombSort or HeapSort, which matters for
** large elements.  On random data it takes more comparisons than MergeSort, but
** on some inputs it takes fewer (see below).
**
** OBSERVATIONS (Claude, Nov 2026).  One sort of 4,194,304 8-byte records;
** millions of comparisons, with seconds in parentheses (single runs, gcc -O2,
** Cygwin).  The columns are:
**   unnested	the previous version: closest fit, linear-scan merge of
**		bookends, BOOKEND_FACTOR 1
**   nest only	nested placement, but still the linear-scan merge, factor 1
**   nested x1	nesting and the ordered merge, factor 1
**   nested x4	the same with factor 4, which is THIS code
**   merge	MergeSort
**
**			unnested     nest only    nested x1    nested x4    merge
** random		212.1 (2.42) 191.2 (2.38) 155.9 (2.14) 151.5 (1.30)  87.0 (1.36)
** 256 values		228.3 (1.77) 208.4 (1.72) 174.0 (1.45) 177.9 (1.19)  86.9 (1.11)
** 4 values		 77.6 (0.64)  45.1 (0.38)  36.4 (0.28)  63.5 (0.41)  77.7 (0.97)
** sorted		102.8 (0.61)  32.2 (0.22)  32.2 (0.25)  54.1 (0.34)  46.1 (0.91)
** reversed		 67.1 (0.44)  28.0 (0.23)  28.0 (0.23)  49.9 (0.34)  46.1 (0.83)
** reversed, dups	110.2 (0.67) 123.8 (0.69)  96.3 (0.48)  86.2 (0.44)  50.4 (0.84)
** nearly sorted		205.3 (1.13)  97.8 (0.83)  58.5 (0.50)  69.5 (0.55)  55.2 (1.00)
**
** - Nesting alone helped on every input except reversed data with duplicates,
**   where the ordered merge was needed to come out ahead.
** - With the same number of bookends (nested x1 vs. unnested), nesting plus the
**   ordered merge needed fewer comparisons on every input.
** - With factor 1, fewer comparisons than MergeSort on sorted, reversed and
**   4-valued data; with factor 4, only on 4-valued data.  MergeSort still wins
**   clearly on comparisons for random data.
** - Even so, on several inputs this sort ran faster than MergeSort despite
**   making more comparisons (though comparisons were cheap here).
** - Copies: exactly 3 per element (12.6M here), vs. 96.5M for MergeSort.
** - In the TEST_SORTS program below (two sorts, many duplicate keys) it took
**   360M comparisons (2.25s); the previous version 455M (3.3s); MergeSort
**   174M (2.19s).
**
** HISTORY, for the curious.  The first stable version recorded each element's
** original position (one size_t per element) to break ties.  The second kept
** it stable without that, using rule 2 alone: it scanned all the bookends from
** the highest down, stopping at the first containing x, and picked the closest
** fit; perhaps surprisingly, it usually needed fewer comparisons than recording
** positions, even though it restricted the choice of bookend.  Nesting then
** reduced comparisons on every input measured.
**
** BOOKEND_FACTOR trades one kind of input against another: with a factor of 1,
** random data took about 3% more comparisons and noticeably more time (it
** makes many more runs), but sorted, reversed and few-valued data took
** substantially fewer comparisons.
**
** RELATED WORK (Claude's commentary, Nov 2026).  Skiena's paper was read in
** full, but this is not a thorough literature search; check the literature
** before claiming anything here is new.  Sources are listed at the end of this
** comment.
**
** - Skiena's "encroaching lists" and the sort built on them, Melsort (S.S.
**   Skiena, "Encroaching lists as a measure of presortedness", BIT
**   28:775-784, 1988) are the closest relative found, and the nesting used
**   here is essentially that paper's idea:
**	* Each element is put on either end of the OLDEST sorted list on which
**	  it fits, else it starts a new list.  This keeps the lists nested, so
**	  the right list is found by binary search, in O(log m) for m lists.
**	* The lists are linked lists, merged two at a time; O(n log m) overall,
**	  hence linear on well-ordered data.  m is proved to average at most
**	  2 sqrt(n), and conjectured to be about sqrt(2n).
**	* It also suggests BOUNDING the number of lists: when the space for them
**	  fills up, merge them into one list, set it aside, and carry on.
**	* Skiena states that Melsort, as specified there, is NOT stable.
**	* Skiena notes that linked lists are slower than arrays, which hampers
**	  Melsort's practicality.
**   So the nesting, the binary search, and bounding the number of lists are NOT
**   new.  Our placement differs in how it treats equal elements (strictly less
**   to go on top; the highest containing bookend when equal to its bottom),
**   which is what makes it stable.
** - A later paper (arXiv:1802.05662) adapts encroaching lists to a heap.
** - Patience sorting is a one-ended cousin: elements are only added to one end
**   of each pile, and the piles can be merged using a priority queue.
**   Chandramouli & Goldstein ("Patience is a Virtue", SIGMOD 2014) make it
**   fast on modern processors, merging runs two at a time.
**
** COPIES.  Our low copy count is a real strength, but probably not unique.
** The following is inferred from how these algorithms work, not measured:
** - Melsort's linked lists need not move elements at all, but pay for pointers,
**   which merging two at a time rewrites a number of times growing like
**   log(m); and the data must be, or end up, in a linked list, or else be
**   copied.
** - Patience sorting with a priority-queue merge needs about 2 copies per
**   element (onto a pile, then to the output).
** - Any version that merges its lists two at a time, in arrays, makes a number
**   of copies that grows like the log of the number of lists.
** - Any comparison sort can move few elements by sorting pointers to them and
**   rearranging the elements afterwards.
** Our low count comes mainly from merging many sorted lists at once, helped by
** circular buffers, which never shift elements when adding at either end.  So
** the more defensible claim to novelty is the COMBINATION: nested two-ended
** lists in contiguous circular buffers, merged many at a time (exploiting the
** nesting) for exactly 3 copies per element whatever the input order, and
** stability without recording positions.  We did not find out whether that
** combination has been published.
**
** SOURCES
** - Skiena 1988:			https://doi.org/10.1007/BF01954897
** - List Heaps, the "later paper":	https://arxiv.org/abs/1802.05662
** - Encroaching List Set (MathWorld):
**	https://mathworld.wolfram.com/EncroachingListSet.html
** - Patience sorting (Wikipedia, seen only as a search summary):
**	https://en.wikipedia.org/wiki/Patience_sorting
** - Chandramouli & Goldstein (publisher's page, seen only via search):
**	https://www.microsoft.com/en-us/research/?p=166281
*/
#define BOOKEND_FACTOR 4	// number of bookends is this times (ln(n)+1)

typedef struct {size_t head, count; char *a;} BOOKEND; // head is the slot of the top element

// address of the k'th element (counting from the top) of a bookend
static char *BookendElem(const BOOKEND *bookend, size_t k, size_t bookendSize, size_t w)
{
    return bookend->a + w * ((bookend->head + k) % bookendSize);
}

// does the top of bookend p come out before the top of bookend q?
static Boolean BookendBefore(const BOOKEND *bookend, int p, int q, size_t bookendSize, size_t w, pfnCmpFcn compare)
{
    int c = compare(BookendElem(bookend+p, 0, bookendSize, w), BookendElem(bookend+q, 0, bookendSize, w));
    return c < 0 || (c == 0 && p < q); // ties go to the lower-numbered bookend (stability rule 2)
}

// Merge (and empty) a bunch of NESTED bookends into dest; returns the number of elements merged.
static size_t BookendMerge(char *dest, int numBookends, BOOKEND *bookend, size_t bookendSize, size_t w, pfnCmpFcn compare)
{
    size_t n=0;
    // order[first..numBookends-1] lists the non-empty bookends, the next one to take from first.
    // Nesting means they start out in order, so this costs no comparisons.
    int order[numBookends], first=0, i;
    for(i=0; i<numBookends; i++) order[i] = i;

    while(first < numBookends)
    {
	int p = order[first];
	memcpy(dest + w*n++, BookendElem(bookend+p, 0, bookendSize, w), w);
	bookend[p].head = (bookend[p].head + 1) % bookendSize;
	if(--bookend[p].count == 0) { ++first; continue; }
	// Put p back in its place; often it can stay first.
	if(first+1 == numBookends || BookendBefore(bookend, p, order[first+1], bookendSize, w, compare)) continue;
	int lo = first+2, hi = numBookends; // binary search for the first one that p comes before
	while(lo < hi)
	{
	    int mid = (lo+hi)/2;
	    if(BookendBefore(bookend, p, order[mid], bookendSize, w, compare)) hi = mid;
	    else lo = mid+1;
	}
	memmove(order+first, order+first+1, (lo-first-1) * sizeof(order[0]));
	order[lo-1] = p;
    }
    return n;
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

int BookendSort(void *a, size_t n, size_t w, pfnCmpFcn compare)
{
    if(n < 2) return 0;

    int numBookends = BOOKEND_FACTOR * ((int)log(n) + 1), used = 0, nRuns = 0, p, r;
    size_t bookendSize = n/numBookends + 1, i, merged = 0;
    // every run but the last either used all the bookends, or filled one of them
    size_t maxRuns = n/MIN((size_t)numBookends, bookendSize) + 1;
    char *A = (char*)a;
    char *store = Malloc(numBookends * bookendSize * w);
    char *runs = Malloc(n * w); // one long array of consecutive sorted runs
    size_t *runStart = Malloc((maxRuns+1) * sizeof(runStart[0]));
    BOOKEND bookend[numBookends];
    int numCopies = 0;

    for(p=0; p<numBookends; p++) bookend[p].a = store + p * bookendSize * w;
    runStart[0] = 0;

    // STEP 1: build bookends
    for(i=0; i < n; i++)
    {
	const char *x = A + w*i;
	// Binary search for k, the first bookend whose range does not contain x
	int lo = 0, hi = used;
	Boolean onTop = false;	  // if bookend k exists: does x go on its top?
	Boolean atBottom = false; // does x equal the bottom of bookend k-1?
	while(lo < hi)
	{
	    int mid = (lo+hi)/2;
	    if(compare(x, BookendElem(bookend+mid, 0, bookendSize, w)) < 0) // strictly less (rule 1)
	    {
		hi = mid; onTop = true;
		continue;
	    }
	    int c = compare(x, BookendElem(bookend+mid, bookend[mid].count-1, bookendSize, w));
	    if(c > 0) { hi = mid; onTop = false; }
	    else { lo = mid+1; atBottom = (c == 0); } // the last such mid is k-1
	}
	int k = lo;

	if(k > 0 && atBottom && bookend[k-1].count < bookendSize) // append to bookend k-1
	    memcpy(BookendElem(bookend+k-1, bookend[k-1].count++, bookendSize, w), x, w);
	else
	{
	    // if x can't go on bookend k, merge them all into a new run
	    if(k < used ? bookend[k].count == bookendSize : used == numBookends)
	    {
		size_t nItems = BookendMerge(runs + w*merged, used, bookend, bookendSize, w, compare);
		merged += nItems;
		numCopies += nItems;
		runStart[++nRuns] = merged;
		used = k = 0;
	    }
	    if(k == used) // it needs a new bookend
	    {
		++used;
		bookend[k].head = 0;
		bookend[k].count = 1;
		memcpy(bookend[k].a, x, w);
	    }
	    else if(onTop)
	    {
		bookend[k].head = (bookend[k].head + bookendSize - 1) % bookendSize;
		bookend[k].count++;
		memcpy(BookendElem(bookend+k, 0, bookendSize, w), x, w);
	    }
	    else // append to the bottom
		memcpy(BookendElem(bookend+k, bookend[k].count++, bookendSize, w), x, w);
	}
	++numCopies;
    }
    size_t nItems = BookendMerge(runs + w*merged, used, bookend, bookendSize, w, compare);
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

#define TOTBYTES (72*1024*1024)
#define ElementSize (253)
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

    printf("bSort: "); fflush(stdout);
    _compareCount = 0;
    t = uTime();
    cpCnt = BookendSortInts(ints, NUM);
    cm = _compareCount; secs = uTime()-t;
    stable = true; // NOT checked: equal ints are indistinguishable (see BookendSortInts)
    for(i=0; i<NUM-1; i++)
    {
	assert(ints[i]<= ints[i+1]);
    }
    printf("cm: %9d cp: %9d (%gs) %sstable\n", cm, cpCnt, secs,
	stable?"":"un");

    printf("psort: "); fflush(stdout);
    _compareCount = 0;
    t = uTime();
    cpCnt = PointerSort(BookendSort,e, NUM, sizeof(ELEMENT), PreSortCmp);
    cpCnt += PointerSort(BookendSort,e, NUM, sizeof(ELEMENT), SortCmp);
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
