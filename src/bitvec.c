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

/* reasonably opaque implementation of bit vectors, or sets of integers, by
** Wayne Hayes, wayne@cs.toronto.edu.
*/

#include "bitvec.h"
#include <stdio.h>
#include <stdarg.h>
#include <math.h> // for sqrt(n) in SPARSE_BITVEC

unsigned bitvecBits = sizeof(BITVEC_ELEMENT)*8, bitvecBits_1;
static int SIZE(int n) { return (n+bitvecBits-1)/bitvecBits; }   /* number of segments needed to store n bits */

unsigned int lookupBitCount[LOOKUP_SIZE];
/* count the number of 1 bits in a long
*/
static unsigned DumbCountBits(unsigned long i)
{
    unsigned n = 0;
    while(i)
    {
	if(i&1) ++n;
	i >>= 1;
    }
    return n;
}


/* Currently this just initializes lookupBitCount[].
** BitvecStartup doesn't perform startup more than once, so it's safe
** (and costs little) to call it again if you're not sure.  It returns
** 1 if it did the initialization, else 0.
*/
Boolean BitvecStartup(void)
{
    if(bitvecBits_1)
	return 0;
    else
    {
	assert(sizeof(BITVEC_ELEMENT) == 4);	// we assume 32-bit ints
	bitvecBits_1 = bitvecBits-1;
	unsigned long i;
	for(i=0; i<LOOKUP_SIZE; i++)
	    lookupBitCount[i] = DumbCountBits(i);
	return 1;
    }
}


/*
** BitvecAlloc: create a new empty bitvec of max size n elements,
** and return its handle.
*/
BITVEC *BitvecAlloc(unsigned n)
{
    if(!bitvecBits_1) BitvecStartup();
    BITVEC *vec = (BITVEC*) Calloc(1,sizeof(BITVEC));
    vec->n = n;
    vec->smallestElement = n; // ie., invalid
    vec->segment = (BITVEC_ELEMENT*) Calloc(sizeof(BITVEC_ELEMENT), SIZE(n));
    return vec;
}


SPARSE_BITVEC *SparseBitvecAlloc(unsigned long n)
{
    SPARSE_BITVEC *vec = (SPARSE_BITVEC*) Calloc(1,sizeof(SPARSE_BITVEC));
    vec->n = n;
    vec->sqrt_n = ceil(sqrt(n));
    vec->vecs = (BITVEC**) Calloc(vec->sqrt_n,sizeof(BITVEC*));
    return vec;
}


/*
** BitvecResize: re-size a vec.
*/
BITVEC *BitvecResize(BITVEC *vec, unsigned new_n)
{
    int i, old_n = vec->n;
    vec->segment = (BITVEC_ELEMENT*) Realloc(vec->segment, sizeof(BITVEC_ELEMENT) * SIZE(new_n));
    vec->n = new_n;
    for(i=old_n; i < new_n; i++)
	BitvecDelete(vec, i);
    return vec;
}


/*
** erase all members from a vec, but don't free it's memory.
*/
BITVEC *BitvecEmpty(BITVEC *vec)
{
    int segment=SIZE(vec->n);
    vec->smallestElement = vec->n;
    memset(vec->segment, 0, segment * sizeof(vec->segment[0]));
    return vec;
}


/*
** erase all members from a vec, but don't free it's memory.
*/
SPARSE_BITVEC *SparseBitvecEmpty(SPARSE_BITVEC *vec)
{
    int i;
    for(i=0; i < vec->sqrt_n; i++)
	if(vec->vecs[i])
	{
	    BitvecFree(vec->vecs[i]);
	    vec->vecs[i] = NULL;
	}
    return vec;
}


/* free all space occupied by a vec
*/
void BitvecFree(BITVEC *vec)
{
    if(vec)
    {
	if(vec->segment)
	    free(vec->segment);
	free(vec);
    }
}


/* free all space occupied by a vec
*/
void SparseBitvecFree(SPARSE_BITVEC *vec)
{
    int i;
    for(i=0; i < vec->sqrt_n; i++)
	if(vec->vecs[i])
	{
	    BitvecFree(vec->vecs[i]);
	    vec->vecs[i] = NULL;
	}
    free(vec);
}


/* Copy a vec.  If the destination is NULL, it will be alloc'd
*/
BITVEC *BitvecCopy(BITVEC *dst, BITVEC *src)
{
    int i, numSrc = SIZE(src->n);

    if(!dst)
	dst = BitvecAlloc(src->n);
    assert(dst->n == src->n);
    dst->smallestElement = src->smallestElement;

    for(i=0; i < numSrc; i++)
	dst->segment[i] = src->segment[i];
    return dst;
}

SPARSE_BITVEC *SparseBitvecCopy(SPARSE_BITVEC *dst, SPARSE_BITVEC *src)
{
    int i;

    if(!dst)
	dst = SparseBitvecAlloc(src->n);
    assert(dst->n == src->n);

    for(i=0; i < dst->sqrt_n; i++)
	BitvecCopy(dst->vecs[i], src->vecs[i]);
    return dst;
}


/* Add an element to a vec.  Returns the same vec handle.
*/
BITVEC *BitvecAdd(BITVEC *vec, unsigned element)
{
    assert(element < vec->n);
    vec->segment[element/bitvecBits] |= BITVEC_BIT(element);
    if(element < vec->smallestElement) vec->smallestElement = element;
    return vec;
}

SPARSE_BITVEC *SparseBitvecAdd(SPARSE_BITVEC *vec, unsigned long element)
{
    assert(element < vec->n);
    int which = element / vec->sqrt_n;
    if(!vec->vecs[which])
	vec->vecs[which] = BitvecAlloc(vec->sqrt_n);
    BitvecAdd(vec->vecs[which], element - which*vec->sqrt_n);
    return vec;
}



/* Add a bunch of elements to a vec.  End the list with (-1).
*/
BITVEC *BitvecAddList(BITVEC *vec, ...)
{
#ifdef sgi
    Apology("stdarg doesn't work on the sgi");
    return NULL;
#else
    int e;
    va_list argptr;
    va_start(argptr, vec);

    while((e = va_arg(argptr, int)) != -1)
	BitvecAdd(vec, (unsigned)e);

    va_end(argptr);
    return vec;
#endif
}


unsigned int BitvecAssignSmallestElement1(BITVEC *vec)
{
    int i, old=vec->smallestElement;
    assert(old == vec->n || !BitvecIn(vec, old)); // it should not be in there!

    for(i=0; i<vec->n; i++)
        if(BitvecIn(vec, i)) // the next smallest element is here
	       break;
    vec->smallestElement = i; // note this works even if there was no new smallest element, so it's now vec->n
    if(i == vec->n)
	assert(BitvecCardinality(vec) == 0);
    return i;
}

/* Delete an element from a vec.  Returns the same vec handle.
*/
BITVEC *BitvecDelete(BITVEC *vec, unsigned element)
{
    assert(element < vec->n);
    vec->segment[element/bitvecBits] &= ~BITVEC_BIT(element);
    if(element == vec->smallestElement)
    {
	BitvecAssignSmallestElement1(vec);
	assert(vec->smallestElement > element);
    }
    return vec;
}



/* query if an element is in a vec; return 0 or non-zero.
*/
#define BITVEC_BIT_SAFE(e) (1UL<<((e)%bitvecBits))
Boolean BitvecInSafe(BITVEC *vec, unsigned element)
{
    assert(element < vec->n);
    unsigned segment = element/bitvecBits, e_bit = BITVEC_BIT_SAFE(element);
    return (vec->segment[segment] & e_bit);
}

Boolean SparseBitvecIn(SPARSE_BITVEC *vec, unsigned long element)
{
    assert(element < vec->n);
    int which = element / vec->sqrt_n;
    return vec->vecs[which] && BitvecIn(vec->vecs[which], element - which*vec->sqrt_n);
}

/* See if A and B are the same vec.
*/
Boolean BitvecEq(BITVEC *A, BITVEC *B)
{
    int i;
    int loop = SIZE(A->n);
    assert(A->n == B->n);
    for(i=0; i < loop; i++)
	if(A->segment[i] != B->segment[i])
	    return false;
    return true;
}
Boolean SparseBitvecEq(SPARSE_BITVEC *A, SPARSE_BITVEC *B)
{
    int i;
    assert(A->n == B->n);
    for(i=0; i < A->sqrt_n; i++)
	if(!BitvecEq(A->vecs[i], B->vecs[i]))
	    return false;
    return true;
}


/* See if A is a (non-proper) subset of B.
*/
Boolean BitvecSubsetEq(BITVEC *A, BITVEC *B)
{
    int i;
    int loop = SIZE(A->n);
    assert(A->n == B->n);
    for(i=0; i < loop; i++)
	if((A->segment[i] & B->segment[i]) != A->segment[i])
	    return false;
    return true;
}

Boolean BitvecSubsetProper(BITVEC *A, BITVEC *B)
{
    return BitvecSubsetEq(A,B) && !BitvecEq(A,B);
}


/* Union A and B into C.  Any or all may be the same pointer.
*/
BITVEC *BitvecUnion(BITVEC *C, BITVEC *A, BITVEC *B)
{
    int i;
    int loop = SIZE(C->n);
    assert(A->n == B->n && B->n == C->n);
    for(i=0; i < loop; i++)
	C->segment[i] = A->segment[i] | B->segment[i];
    C->smallestElement = MIN(A->smallestElement, B->smallestElement);
    return C;
}
SPARSE_BITVEC *SparseBitvecUnion(SPARSE_BITVEC *C, SPARSE_BITVEC *A, SPARSE_BITVEC *B)
{
    int i;
    assert(A->n == B->n && B->n == C->n);
    for(i=0; i < A->sqrt_n; i++)
	BitvecUnion(C->vecs[i], A->vecs[i], B->vecs[i]);
    return C;
}


unsigned int BitvecAssignSmallestElement3(BITVEC *C,BITVEC *A,BITVEC *B)
{
    if(A->smallestElement == B->smallestElement)
	C->smallestElement = A->smallestElement;
    else if(BitvecIn(A, B->smallestElement))
    {
	assert(!BitvecIn(B, A->smallestElement));
	assert(A->smallestElement < B->smallestElement);
	C->smallestElement = A->smallestElement;
    }
    else if(BitvecIn(B, A->smallestElement))
    {
	assert(!BitvecIn(A, B->smallestElement));
	assert(B->smallestElement < A->smallestElement);
	C->smallestElement = B->smallestElement;
    }
    else
    {
	BitvecAssignSmallestElement1(C);
	assert(C->smallestElement > A->smallestElement);
	assert(C->smallestElement > B->smallestElement);
    }
    return C->smallestElement;
}

/* Intersection A and B into C.  Any or all may be the same pointer.
*/
BITVEC *BitvecIntersect(BITVEC *C, BITVEC *A, BITVEC *B)
{
    int i;
    int loop = SIZE(C->n);
    assert(A->n == B->n && B->n == C->n);
    for(i=0; i < loop; i++)
	C->segment[i] = A->segment[i] & B->segment[i];
    BitvecAssignSmallestElement3(C,A,B);
    return C;
}

SPARSE_BITVEC *SparseBitvecIntersect(SPARSE_BITVEC *C, SPARSE_BITVEC *A, SPARSE_BITVEC *B)
{
    int i;
    assert(A->n == B->n && B->n == C->n);
    for(i=0; i < C->sqrt_n; i++)
	BitvecIntersect(C->vecs[i], A->vecs[i], B->vecs[i]);
    return C;
}


/* XOR A and B into C.  Any or all may be the same pointer.
*/
BITVEC *BitvecXOR(BITVEC *C, BITVEC *A, BITVEC *B)
{
    int i;
    int loop = SIZE(C->n);
    if(!A) return BitvecCopy(C, B);
    if(!B) return BitvecCopy(C, A);
    assert(A->n == B->n && B->n == C->n);
    for(i=0; i < loop; i++)
	C->segment[i] = A->segment[i] ^ B->segment[i];
    BitvecAssignSmallestElement3(C,A,B);
    return C;
}


/* Complement of A.  Both may be the same pointer.
*/
BITVEC *BitvecComplement(BITVEC *B, BITVEC *A)
{
    int i;
    int loop = SIZE(B->n);
    assert(A->n == B->n);
    for(i=0; i < loop; i++)
	B->segment[i] = ~A->segment[i];
    BitvecAssignSmallestElement1(B);
    return B;
}


unsigned BitvecCardinality(BITVEC *A)
{
    unsigned n = 0, i, loop = SIZE(A->n);
    for(i=0; i < loop; i++)
	if(A->segment[i]) n += BitvecCountBits(A->segment[i]);
    return n;
}

unsigned long SparseBitvecCardinality(SPARSE_BITVEC *vec)
{
    unsigned int i;
    unsigned long sum=0;
    for(i=0; i < vec->sqrt_n; i++)
	if(vec->vecs[i])
	    sum += BitvecCardinality(vec->vecs[i]);
    return sum;
}

/* populate the given array with the list of members currently present
** in the vec.  The array is assumed to have enough space.
*/
unsigned BitvecToArray(unsigned int *array, BITVEC *vec)
{
    int pos = 0;
    int i;
    for(i=0; i < vec->n; i++)
	if(BitvecIn(vec,i))
	    array[pos++] = i;

    assert(pos == BitvecCardinality(vec));
    return pos;
}

/* Add the elements listed in the array to the vec.
*/
BITVEC *BitvecFromArray(BITVEC *vec, int n, unsigned int *array)
{
    while(n > 0)
	BitvecAdd(vec, array[--n]);
    return vec;
}

char *BitvecToString(int len, char s[], BITVEC *vec)
{
    int i;
    assert(len > vec->n); /* need space for trailing '\0' */
    for(i=0; i<MIN(len, vec->n); i++)
	s[i] = '0' + !!BitvecIn(vec, i);
    s[i] = '\0';
    return s;
}

/* Use a seive to get all primes between 0 and n */
BITVEC *BitvecPrimes(long n)
{
    BITVEC *primes = BitvecAlloc(n+1);
    int i, loop=SIZE(n+1), p;

    for(i=0; i<loop; i++)
	--primes->segment[i];     /* turn on all the bits */
    BitvecDelete(primes, 0);
    BitvecDelete(primes, 1);

    p=2;
    while(p <= n)
    {
	for(i = p+p; i <= n; i+=p) /* delete all multiples of p */
	    BitvecDelete(primes, i);
	/* find next prime */
	do  ++p;
	while(p <= n && !BitvecIn(primes, p));
    }
    return primes;
}

void BitvecPrint(BITVEC *A)
{
    int i;
    for(i=0;i<A->n;i++) if(BitvecIn(A,i)) printf("%d ", i);
    printf("\n");
}


#ifdef __cplusplus
} // end extern "C"
#endif
