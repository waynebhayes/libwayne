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
** Wayne Hayes, wayne@cs.toronto.edu
*/

/* reasonably opaque implementation of bit fields, or sets of integers, by
** Wayne Hayes, wayne@cs.toronto.edu.
*/

#include <stdio.h>
#include <stdarg.h>
#include "sets.h"

/*
** BitvecStartup needs to set some stuff, and since we use it bitvecs...
*/
static Boolean _doneInit;
Boolean SetStartup(void)
{
    if(_doneInit) return false;
    _doneInit = true;
    return BitvecStartup();
}


int SetComputeCrossover(unsigned n)
{
    static unsigned prevN, prevResult;
    if(n==prevN) return prevResult;
    prevN=n;

    // binary search
    int low=0, high=n-1, mid=0; // inclusive
    while(low<high) {
	mid = low + (high-low)/2;
	assert(low <= mid && mid <= high);
	int B=BitvecBytes(mid), L=mid*sizeof(SET_ELEMENT_TYPE);
	if(L>B) high=mid-1; // if the list takes more space, the crossover must be BELOW mid
	else low=mid+1;
    }
    return prevResult = mid;
}

/*
** SetAlloc: create a new empty set of max size n elements. Return its handle.
*/
SET *SetAlloc(SET_ELEMENT_TYPE n)
{
    if(!_doneInit) SetStartup();
    SET *set = (SET*) Calloc(1,sizeof(SET));
    assert(set->cardinality == 0);
    set->maxElem = n;
    set->smallestElement = n; // ie., invalid
    set->bitvec = NULL;
    set->listSize = SET_MIN_LIST;
    set->list = (SET_ELEMENT_TYPE*) Calloc(sizeof(SET_ELEMENT_TYPE), set->listSize);
    set->crossover = SetComputeCrossover(n);
    return set;
}

/* query if an element is in a set; return 0 or non-zero.
*/
Boolean SetInSafe(SET *set, unsigned element)
{
    assert(element < set->maxElem);
    if(set->bitvec) {
#if PARANOID_ASSERTS
	assert(!_smallestGood || set->smallestElement == set->bitvec->smallestElement);
#endif
	return BitvecInSafe(set->bitvec, element);
    }
    assert(set->list && set->cardinality <= set->crossover);
    int i;
    for(i=0; i<set->cardinality; i++) if(element == set->list[i]) return true;
    return false;
}

// "Upgrade" a set from using an unsorted list to BITVEC
static SET *SetMakeBitvec(SET *s)
{
    if(s->bitvec) {assert(!s->list && !s->listSize); return s;}
    assert(s->list && s->cardinality <= s->crossover);
    s->bitvec = BitvecAlloc(s->maxElem);
    int i;
    for(i=0; i<s->cardinality; i++) BitvecAdd(s->bitvec, s->list[i]);
    Free(s->list);
    s->list = NULL;
    s->listSize = 0;
    return s;
}


/*
** SetResize: re-size a set.
*/
SET *SetResize(SET *set, unsigned new_n)
{
    // int i, old_n = set->maxElem;
    if(set->bitvec) set->bitvec = BitvecResize(set->bitvec, new_n); // don't bother going back to list if new_n is small
    else assert(set->list && set->cardinality <= set->crossover); // nothing to do if it's still a list
    set->maxElem = new_n;
    return set;
}


/*
** erase all members from a set, but don't free it's memory.
*/
SET *SetEmpty(SET *set)
{
    if(set->bitvec) BitvecEmpty(set->bitvec);
    set->smallestElement = set->maxElem;
    set->cardinality = 0; // in the spirit of C, don't bother zapping the list elements to zero
    return set;
}


/* free all space occupied by a set
*/
void SetFree(SET *set)
{
    if(set)
    {
	if(set->bitvec) BitvecFree(set->bitvec);
	if(set->list) Free(set->list);
	Free(set);
    }
}


/* Add an element to a set.  Returns the same set handle.
*/
SET *SetAdd(SET *s, unsigned element)
{
#if PARANOID_ASSERTS
    if(s->bitvec && _smallestGood) assert(s->smallestElement == s->bitvec->smallestElement);
#endif
    assert(element < s->maxElem);
    if(SetIn(s, element)) return s;
    if(s->bitvec) BitvecAdd(s->bitvec, element);
    else {
	assert(s->list);
	if(s->cardinality < s->crossover) { // divide by sqrt(2) to split the difference [listSize, 2*listSize]
	    assert(s->cardinality <= s->listSize);
	    if(s->cardinality == s->listSize) {
		s->listSize *= 2;
		s->list = (SET_ELEMENT_TYPE*) Realloc(s->list, sizeof(SET_ELEMENT_TYPE) * s->listSize);
	    }
	    s->list[s->cardinality] = element;
	} else {
	    SetMakeBitvec(s);
	    assert(!s->list);
	    BitvecAdd(s->bitvec, element);
	}
    }
    ++s->cardinality;
    if(element < s->smallestElement) s->smallestElement = element;
#if PARANOID_ASSERTS
    if(s->bitvec) {
	assert(!_smallestGood || s->smallestElement == s->bitvec->smallestElement);
	assert(s->cardinality == BitvecCardinality(s->bitvec));
    }
    else assert(s->list && s->cardinality <= s->crossover);
#endif
    return s;
}


/* Copy a set.  If the destination is NULL, it will be alloc'd
*/
SET *SetCopy(SET *dst, SET *src)
{
    if(!dst) dst = SetAlloc(src->maxElem);
    else SetEmpty(dst);
    if(src->bitvec) {
	SetMakeBitvec(dst);
	BitvecCopy(dst->bitvec, src->bitvec);
    } else {
	assert(dst-> list && src->list && src->cardinality <= src->crossover);
	int i;
	for(i=0;i<src->cardinality;i++) dst->list[i] = src->list[i];
    }
    assert(dst->maxElem == src->maxElem);
    dst->smallestElement = src->smallestElement;
    dst->cardinality = src->cardinality;
    dst->crossover = src->crossover;
    return dst;
}

/* Add a bunch of elements to a set.  End the list with (-1).
*/
SET *SetAddList(SET *set, ...)
{
#ifdef sgi
    Apology("stdarg doesn't work on the sgi");
    return NULL;
#else
    int e;
    va_list argptr;
    va_start(argptr, set);

    while((e = va_arg(argptr, int)) != -1)
	SetAdd(set, (unsigned)e);

    va_end(argptr);
    return set;
#endif
}


static unsigned int SetAssignSmallestElement1(SET *set)
{
    _smallestGood = false;
    SET_ELEMENT_TYPE new=set->maxElem;
    if(set->bitvec) {
	new = BitvecAssignSmallestElement1(set->bitvec);
    } else { assert(set->list && set->cardinality <= set->crossover);
	int i;
	for(i=0; i<set->cardinality; i++) if(set->list[i] < new) new = set->list[i];
    }
    _smallestGood = true;
    return (set->smallestElement = new);
}

static unsigned int SetAssignSmallestElement3(SET *C, SET *A, SET *B)
{
    _smallestGood = false;
    if(A->smallestElement == B->smallestElement)
	C->smallestElement = A->smallestElement;
    else if(SetIn(A, B->smallestElement))
    {
	assert(!SetIn(B, A->smallestElement));
	assert(A->smallestElement < B->smallestElement);
	C->smallestElement = A->smallestElement;
    }
    else if(SetIn(B, A->smallestElement))
    {
	assert(!SetIn(A, B->smallestElement));
	assert(B->smallestElement < A->smallestElement);
	C->smallestElement = B->smallestElement;
    }
    else
    {
	SetAssignSmallestElement1(C);
	assert(C->smallestElement > A->smallestElement);
	assert(C->smallestElement > B->smallestElement);
    }
    _smallestGood = true;
    return C->smallestElement;
}

/* Delete an element from a set.  Returns the same set handle.
*/
SET *SetDelete(SET *set, unsigned element)
{
    assert(element < set->maxElem);
    if(set->bitvec && _smallestGood) assert(set->smallestElement == set->bitvec->smallestElement);
    if(!SetIn(set, element)) return set;
    if(set->bitvec) BitvecDelete(set->bitvec, element);
    else {
	int i;
	for(i=0; i<set->cardinality; i++) if(set->list[i] == element) break;
	assert(i<set->cardinality); // it SHOULD be there!
	set->list[i] = set->list[set->cardinality-1]; // nuke the element by moving the last one to its position
    }
    set->cardinality--;
    if(element == set->smallestElement)
    {
	SetAssignSmallestElement1(set);
	assert(set->smallestElement > element);
    }
#if PARANOID_ASSERTS
    if(set->bitvec) {
	if(_smallestGood) assert(set->smallestElement == set->bitvec->smallestElement);
	assert(BitvecCardinality(set->bitvec) == set->cardinality);
    }
#endif
    return set;
}



/* See if A and B are the same set.
*/
Boolean SetEq(SET *A, SET *B)
{
    if(A->cardinality != B->cardinality) return false;
    if(A->bitvec && B->bitvec) return BitvecEq(A->bitvec, B->bitvec);

    // At least one of them is a list, so just loop
    SET_ELEMENT_TYPE *list = A->list;
    SET *set = B;
    if(B->list) {
	list = B->list; set = A; // note this works even if both are lists
    }
    int i;
    for(i=0; i<set->cardinality; i++) if(!SetIn(set, list[i])) return false;
    return true;
}


/* See if A is a subset of B (including ==)
*/
Boolean SetSubsetEq(SET *A, SET *B)
{
    if(A->cardinality > B->cardinality) return false;
    if(A->bitvec && B->bitvec) return BitvecSubsetEq(A->bitvec, B->bitvec);
    int i;
    if(A->list) { for(i=0; i<A->cardinality; i++) if(!SetIn(B, A->list[i])) return false; }
    else for(i=0; i<A->maxElem; i++) if(SetIn(A,i) && !SetIn(B, i)) return false;
    return true;
}

Boolean SetSubsetProper(SET *A, SET *B)
{
    return !SetEq(A,B) && SetSubsetEq(A,B);
}


/* Union A and B into C.  Any or all may be the same pointer.
*/
SET *SetUnion(SET *C, SET *A, SET *B)
{
    int i;
    assert(C && A->maxElem == B->maxElem && B->maxElem == C->maxElem);
    SET *tmp = SetAlloc(A->maxElem);
    if(A->bitvec || B->bitvec) { // at least one uses bitvec
	SetMakeBitvec(tmp);
	if(A->bitvec && B->bitvec) { // both use bitvecs
	    BitvecUnion(tmp->bitvec, A->bitvec, B->bitvec);
	}
	else {
	    assert(!A->bitvec || !B->bitvec); // at MOST one uses bitvec
	    SET *vec = A, *list = B; // default
	    if(A->bitvec) assert(!A->list && !B->bitvec && B->list);
	    else { // swap the above
		assert(B->bitvec && !B->list && A->list);
		vec=B; list=A;
	    }
	    assert(vec->bitvec && list->list && !vec->list && !list->bitvec);
	    SetCopy(tmp, vec);
	    for(i=0;i<list->cardinality;i++) SetAdd(tmp, list->list[i]);
	}
	tmp->cardinality = BitvecCardinality(tmp->bitvec);
    } else {
	assert(A->list && B->list && !A->bitvec && !B->bitvec);
	SetCopy(tmp, A);
	for(i=0;i<B->cardinality;i++) SetAdd(tmp, B->list[i]);
    }
    tmp->smallestElement = MIN(A->smallestElement, B->smallestElement);
    SetCopy(C,tmp);
    SetFree(tmp);
    return C;
}


/* Intersection A and B into C.  Any or all may be the same pointer.
*/
SET *SetIntersect(SET *C, SET *A, SET *B)
{
    int i;
    assert(A->maxElem == B->maxElem && B->maxElem == C->maxElem);
    _smallestGood = false;
    if(A->bitvec && B->bitvec) BitvecIntersect(C->bitvec, A->bitvec, B->bitvec);
    else if(A->bitvec || B->bitvec) { // at least one is a bitvec, with the other a list
	SetEmpty(C);
	SetMakeBitvec(C);
	SET *vec = A, *list = B; // default
	if(B->bitvec) { // swap the above
	    vec=B; list=A;
	}
	assert(vec->bitvec && list->list);
	for(i=0;i<list->cardinality;i++) if(SetIn(vec, list->list[i])) SetAdd(C, list->list[i]);
	C->cardinality = BitvecCardinality(C->bitvec);
    } else {
	assert(A->list && B->list && !A->bitvec && !B->bitvec);
	for(i=0;i<B->cardinality;i++) if(SetIn(A, B->list[i])) SetAdd(C, B->list[i]);
    }
    C->smallestElement = C->maxElem;
    if(SetIn(C, A->smallestElement)) C->smallestElement = A->smallestElement;
    if(SetIn(C, B->smallestElement) && B->smallestElement < C->smallestElement) C->smallestElement = B->smallestElement;
    if(C->smallestElement == C->maxElem) C->smallestElement = SetAssignSmallestElement1(C);
    _smallestGood = true;
    return C;
}

/* XOR A and B into C.  Any or all may be the same pointer.
*/
SET *SetXOR(SET *C, SET *A, SET *B)
{
    if(A->bitvec && B->bitvec) C->bitvec = BitvecXOR(C->bitvec, A->bitvec, B->bitvec);
    else Apology("Sorry, haven't yet implemented SetXOR for lists");
    SetAssignSmallestElement3(C,A,B);
    return C;
}


/* Complement of A.  Both may be the same pointer.
*/
SET *SetComplement(SET *B, SET *A)
{
    if(A->bitvec && B->bitvec) B->bitvec = BitvecComplement(A->bitvec, B->bitvec);
    else Apology("Sorry, haven't yet implemented SetComplement for lists");
    SetAssignSmallestElement1(B);
    return B;
}


unsigned SetCardinality(SET *A)
{
    if(A->bitvec) A->cardinality = BitvecCardinality(A->bitvec);
    return A->cardinality;
}

/* populate the given array with the list of members currently present
** in the set.  The array is assumed to have enough space.
*/
unsigned SetToArray(unsigned int *array, SET *set)
{
    int pos = 0;
    int i;
    for(i=0; i < set->maxElem; i++)
	if(SetIn(set,i))
	    array[pos++] = i;

    assert(pos == SetCardinality(set));
    return pos;
}

unsigned SSetToArray(unsigned int *array, SSET set)
{
    int pos = 0;
    int i;
    for(i=0; i < MAX_SSET; i++)
	if(SSetIn(set,i))
	    array[pos++] = i;

    assert(pos == SSetCountBits(set));
    return pos;
}


/* Add the elements listed in the array to the set.
*/
SET *SetFromArray(SET *set, int n, unsigned int *array)
{
    while(n > 0)
	SetAdd(set, array[--n]);
    return set;
}

/* Add the elements listed in the array to the small set.
*/
SSET SSetFromArray(int n, unsigned int *array)
{
    SSET set;
    SSetEmpty(set);
    while(n > 0)
	SSetAdd(set, array[--n]);
    return set;
}

char *SSetToString(int len, char s[], SSET set)
{
    int i;
    for(i=0; i<len-1; i++)
	s[i] = '0' + !!SSetIn(set, i);
    s[len-1] = '\0';
    return s;
}

char *SetToString(int len, char s[], SET *set)
{
    int i;
    assert(len > set->maxElem); /* need space for trailing '\0' */
    for(i=0; i<MIN(len, set->maxElem); i++)
	s[i] = '0' + !!SetIn(set, i);
    s[i] = '\0';
    return s;
}

/* Use a seive to get all primes between 0 and n */
SET *SetPrimes(long n)
{
    SET *primes = SetAlloc(n+1);
    int i, p;
    SetMakeBitvec(primes);

    for(i=0; i<NUMSEGS(n+1); i++)
	primes->bitvec->segment[i] = bitvecBits_1;     /* turn on all the bits */
    SetDelete(primes, 0);
    SetDelete(primes, 1);

    p=2;
    while(p <= n)
    {
	for(i = p+p; i <= n; i+=p) /* delete all multiples of p */
	    SetDelete(primes, i);
	/* find next prime */
	do  ++p;
	while(p <= n && !SetIn(primes, p));
    }
    return primes;
}

void SetPrint(SET *A)
{
    int i;
    for(i=0;i<A->maxElem;i++) if(SetIn(A,i)) printf("%d ", i);
    printf("\n");
}


/*
** SSETDICT: a set of sets.  Idea is to be able to store sets and quickly
** query if a set is in the dictionary yet.  Used mostly by the circulant
** graph generation routines which need to quickly see if a circulant has
** already been generated yet.  Each set is stored only once -- adding
** an already existing set does nothing.  At allocation time, you need to
** know about how many items will eventually be stored.
** Implementation is an array of hash tables all of the same size.
** Collisions are resolved by putting it in the next hash table "down",
** and if you run out of hash tables then you move to the first hash
** table's next position, etc.
*/

/*
** Number of "stacked" hash tables.  Experiments show that as long as
** the initial allocation is a good estimate of the actual number of
** things eventually to be inserted, then there's no gain to have more
** than two.
*/
#define NUM_HASHES 2

struct _ssetDict {
    SSET *array[NUM_HASHES];
    int nCols;	/* number of columns - always prime */
    int nElem;	/* numbef of elements currently stored, not including NULLSET */
    Boolean containsNull; /* never explicitly store the empty Set */
};


SSETDICT *SSetDictAlloc(int n)
{
    int i;
    SSETDICT *ssd = Calloc(sizeof(SSETDICT), 1);
    assert(n>=1);
    ssd->nCols = n;
    ssd->nElem = 0;
    for(i=0; i< NUM_HASHES; i++)
	ssd->array[i] = Calloc(sizeof(SSET), ssd->nCols);
    return ssd;
}

/*
** Finds a place for ss; if ss is in the dictionary, it returns a pointer
** to ss's position; otherwise it returns a pointer to a blank position
** where ss can be put.
** Returns NULL if the dictionary is full.
*/
static SSET *SSetDict_find_place(SSETDICT *ssd, SSET ss)
{
    int position = ss % ssd->nCols, col;
    assert(ss != SSET_NULLSET);

    for(col=position; col != (position+ssd->nCols-1) % ssd->nCols; col = (col+1) % ssd->nCols)
    {
	int itable;
	for(itable=0; itable<NUM_HASHES; itable++)
	{
	    SSET *place = &(ssd->array[itable][col]);
	    if(*place == ss || *place == SSET_NULLSET)
		return place;
	}
    }

    return NULL;
}


/* Double the size of the dictionary */
static SSETDICT *SSetDictDouble(SSETDICT *ssd)
{
    int col, itable, nAdded = 0;
    SSETDICT *newDict = SSetDictAlloc(2*ssd->nCols);

    for(itable=0; itable<NUM_HASHES; itable++)
    {
	for(col=0; col < ssd->nCols; col++)
	    if(ssd->array[itable][col])
	    {
		SSetDictAdd(newDict, ssd->array[itable][col]);
		++nAdded;
	    }
	Free(ssd->array[itable]);
	ssd->array[itable] = newDict->array[itable];
    }
    assert(nAdded == ssd->nElem);

    ssd->nCols = newDict->nCols;
    Free(newDict);

    return ssd;
}


SSETDICT *SSetDictAdd(SSETDICT *ssd, SSET ss)
{
    SSET *place;

    if(ss == SSET_NULLSET)
    {
	ssd->containsNull = true;
	return ssd;
    }

    if(ssd->nElem == ssd->nCols)
	SSetDictDouble(ssd);

    place = SSetDict_find_place(ssd, ss);

    assert(place);

    if(*place != ss)
    {
	assert(*place == SSET_NULLSET);
	ssd->nElem++;
	*place = ss;	/* it's either already ss or NULLSET; assign it either way */
    }

    return ssd;
}

Boolean SSetDictIn(SSETDICT *ssd, SSET ss)
{
    SSET *place;
    if(ss == SSET_NULLSET)
	return ssd->containsNull;
    place = SSetDict_find_place(ssd, ss);
    if(place && *place == ss)
	return true;
    else
	return false;
}

void SSetDictFree(SSETDICT *ssd)
{
    int i;
    for(i=0; i<NUM_HASHES; i++)
	Free(ssd->array[i]);
    Free(ssd);
}

#if TINY_SET_SIZE < 64
unsigned TSetToArray(unsigned int *array, TSET set)
{
    int pos = 0;
    int i;
    for(i=0; i < MAX_TSET; i++)
	if(TSetIn(set,i))
	    array[pos++] = i;

    assert(pos == TSetCountBits(set));
    return pos;
}


TSET TSetFromArray(int n, unsigned int *array)
{
    TSET set;
    TSetEmpty(set);
    while(n > 0)
	TSetAdd(set, array[--n]);
    return set;
}

char *TSetToString(int len, char s[], TSET set)
{
    int i;
    for(i=0; i<len-1; i++)
	s[i] = '0' + !!TSetIn(set, i);
    s[len-1] = '\0';
    return s;
}
#endif
#ifdef __cplusplus
} // end extern "C"
#endif
