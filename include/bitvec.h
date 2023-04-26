#ifdef __cplusplus
extern "C" {
#endif
#ifndef _BITVEC_H
#define _BITVEC_H
/* Blame this on Wayne Hayes, wayne@csri.utoronto.ca.
**
** simple implementation of bit vector.  You allocate a BITVEC by specifying
** the max number N of things that could be in the vector; each possible element
** is represented by an unsigned from 0..N-1, and it's presence in the vector is
** represented by that bit in the vector being a 1, else 0.
**
** It is very space efficient, and reasonably time efficient, except for
** BitvecToArray in bitvec.c, which can be very slow for sparse vectors inside
** a large BITVEC structure.
**
** Any operations on more than one bitvec *must* have both operands of
** the exact same size and type of vector (this restriction may be laxed in
** later implementations).
*/

#include <stdlib.h>
#include <assert.h>
#include "misc.h"

typedef unsigned BITVEC_ELEMENT;
extern unsigned bitvecBits, bitvecBits_1;
//static unsigned BITVEC_BIT(unsigned e) {assert((e%bitvecBits) == (e&bitvecBits_1)); return (1U<<((e)%bitvecBits));}
//#define BITVEC_BIT(e) (1U<<((e)%bitvecBits))
#define BITVEC_BIT(e) (1U<<((e)&bitvecBits_1))

typedef struct _bitvecType {
    unsigned n; /* in bits */
    unsigned smallestElement;
    BITVEC_ELEMENT* segment;
} BITVEC;

#define LOOKUP_NBITS 16
#define LOOKUP_SIZE (1 << LOOKUP_NBITS)
#define LOOKUP_MASK (LOOKUP_SIZE - 1)
extern unsigned lookupBitCount[LOOKUP_SIZE];
#define BitvecCountBits(i) \
    (lookupBitCount[((BITVEC_ELEMENT)(i)) & LOOKUP_MASK] + \
	lookupBitCount[(((BITVEC_ELEMENT)(i)) >> LOOKUP_NBITS) & LOOKUP_MASK])

Boolean BitvecStartup(void); // always succeeds, but returns whether it did anything or not.

/* allocate & return empty Bitvec capable of storing integers 0..n-1 inclusive */
BITVEC *BitvecAlloc(unsigned n);
BITVEC *BitvecResize(BITVEC *s, unsigned new_n);
void BitvecFree(BITVEC *vec); /* free all memory used by a bitvec */
BITVEC *BitvecEmpty(BITVEC *vec);    /* make the vec empty (vec must be allocated )*/
#define BitvecReset BitvecEmpty
BITVEC *BitvecCopy(BITVEC *dst, BITVEC *src);  /* if dst is NULL, it will be alloc'd */
BITVEC *BitvecAdd(BITVEC *vec, unsigned element);    /* add single element to vec */
BITVEC *BitvecAddList(BITVEC *vec, ...); /* end list with (-1); uses varargs/stdarg */
BITVEC *BitvecDelete(BITVEC *vec, unsigned element); /* delete a single element */
BITVEC *BitvecUnion(BITVEC *C, BITVEC *A, BITVEC *B);  /* C = union of A and B */
BITVEC *BitvecIntersect(BITVEC *C, BITVEC *A, BITVEC *B);  /* C = intersection of A and B */
BITVEC *BitvecXOR(BITVEC *C, BITVEC *A, BITVEC *B);  /* C = XOR of A and B */
BITVEC *BitvecComplement(BITVEC *B, BITVEC *A);  /* B = complement of A */
unsigned BitvecCardinality(BITVEC *A);    /* returns non-negative integer */
Boolean BitvecInSafe(BITVEC *vec, unsigned element); /* boolean: 0 or 1 */
#define BitvecSmallestElement(S) (S->smallestElement)
#if 1 //NDEBUG || !PARANOID_ASSERTS
// Note we do not check here if e is < vec->n, which is dangerous
#define BitvecIn(vec,e) ((vec)->segment[(e)/bitvecBits] & BITVEC_BIT(e))
//#define BitvecIn(vec,e) ((e)>=0 && (e)<(vec)->n && ((vec)->segment[(e)/bitvecBits] & BITVEC_BIT(e)))
#else
#define BitvecIn BitvecInSafe
#endif
Boolean BitvecEq(BITVEC *vec1, BITVEC *vec2);
Boolean BitvecSubsetEq(BITVEC *sub, BITVEC *super); /* is sub <= super? */
#define BitvecSupersetEq(spr,sb) BitvecSubsetEq((sb),(spr))
Boolean BitvecSubsetProper(BITVEC *sub, BITVEC *super);	/* proper subset */
#define BitvecSupersetProper(spr,sub) BitvecSubsetProper((sub),(spr))

/*
** You allocate an array big enough to hold the number of elements,
** and this function will populate it with the actual integers that are
** members of the vec.  So if the vec contains, say {0,17,324}, array
** will have array[0]=0, array[1]=17, array[2]=324, array[3..*] unchanged,
** and the function returns the cardinality (ie, number of array elements
** populated)
*/
unsigned BitvecToArray(unsigned *array, BITVEC *vec);
BITVEC *BitvecFromArray(BITVEC *s, int n, unsigned *array);
char *BitvecToString(int len, char s[], BITVEC *vec);

BITVEC *BitvecPrimes(long n); /* return the vec of all primes between 0 and n */
void BitvecPrint(BITVEC *A); /* print elements of the vec */

/*
*********  SPARSE_BITVEC  ********
*/
typedef struct _sparseBitvecType {
    unsigned long n;
    unsigned sqrt_n;
    BITVEC **vecs;
} SPARSE_BITVEC;
SPARSE_BITVEC *SparseBitvecAlloc(unsigned long n);
void SparseBitvecFree(SPARSE_BITVEC *vec); /* free all memory used by a vec */
SPARSE_BITVEC *SparseBitvecEmpty(SPARSE_BITVEC *vec);    /* make the vec empty (vec must be allocated )*/
#define BitvecReset BitvecEmpty
SPARSE_BITVEC *SparseBitvecCopy(SPARSE_BITVEC *dst, SPARSE_BITVEC *src);  /* if dst is NULL, it will be alloc'd */
SPARSE_BITVEC *SparseBitvecAdd(SPARSE_BITVEC *vec, unsigned long element);    /* add single element to vec */
SPARSE_BITVEC *SparseBitvecAddList(SPARSE_BITVEC *vec, ...); /* end list with (-1); uses varargs/stdarg */
SPARSE_BITVEC *SparseBitvecDelete(SPARSE_BITVEC *vec, unsigned long element); /* delete a single element */
SPARSE_BITVEC *SparseBitvecUnion(SPARSE_BITVEC *C, SPARSE_BITVEC *A, SPARSE_BITVEC *B);  /* C = union of A and B */
SPARSE_BITVEC *SparseBitvecIntersect(SPARSE_BITVEC *C, SPARSE_BITVEC *A, SPARSE_BITVEC *B);  /* C = intersection of A and B */
SPARSE_BITVEC *SparseBitvecXOR(SPARSE_BITVEC *C, SPARSE_BITVEC *A, SPARSE_BITVEC *B);  /* C = XOR of A and B */
SPARSE_BITVEC *SparseBitvecComplement(SPARSE_BITVEC *B, SPARSE_BITVEC *A);  /* B = complement of A */
unsigned long SparseBitvecCardinality(SPARSE_BITVEC *A);    /* returns non-negative integer */
Boolean SparseBitvecIn(SPARSE_BITVEC *vec, unsigned long element); /* boolean: 0 or 1 */
Boolean SparseBitvecEq(SPARSE_BITVEC *vec1, SPARSE_BITVEC *vec2);
Boolean SparseBitvecSubsetEq(SPARSE_BITVEC *sub, SPARSE_BITVEC *super); /* is sub <= super? */
#define SparseBitvecSupersetEq(spr,sb) SparseBitvecSubsetEq((sb),(spr))
Boolean SparseBitvecSubsetProper(SPARSE_BITVEC *sub, SPARSE_BITVEC *super);	/* proper subset */
#define SparseBitvecSupersetProper(spr,sub) BitvecSubsetProper((sub),(spr))

#endif /* _BITVEC_H */
#ifdef __cplusplus
} // end extern "C"
#endif
