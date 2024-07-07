#ifdef __cplusplus
extern "C" {
#endif
#ifndef _BINTREE_H
#define _BINTREE_H

#include <malloc.h>
#include <stdio.h>
#include "misc.h"   /* for foint */


/*-------------------  Types  ------------------*/

typedef struct _binTreeNode
{
    foint key, info;
    struct _binTreeNode *left, *right;
    Boolean deleted;
} BINTREENODE;

typedef struct _binTree
{
    unsigned n, physical_n, depthSum, depthSamples; // number of entries & tree depth stats which can be averaged anytime.
    BINTREENODE *root;
    pCmpFcn cmpKey;
    pFointCopyFcn copyKey, copyInfo;
    pFointFreeFcn freeKey, freeInfo;
} BINTREE;

/*-----------   Function Prototypes  -----------*/

BINTREE *BinTreeAlloc(pCmpFcn cmpKey, pFointCopyFcn copyKey, pFointFreeFcn freeKey,
	    pFointCopyFcn copyInfo, pFointFreeFcn freeInfo);

void BinTreeInsert(BINTREE *, foint key, foint info); // replaces info if the key already exists

/* O(log n); returns false if failure, and true if found and then assigns to *pInfo if *pInfo is non-NULL */
Boolean BinTreeLookDel(BINTREE *, foint key, foint *pInfo);
#define BinTreeLookup(t,k,p) BinTreeLookDel((t),(k),(p))
#define BinTreeDelete(t,k)   BinTreeLookDel((t),(k),(foint*)1)

/*
** BinTreeTraverse: Traverse a binary tree, calling your function pointer (pFointTraversalFcn) on each
** element, in order.
*/
Boolean BinTreeTraverse ( BINTREE *bt, pFointTraverseFcn);
Boolean BinTreeSanityCheck ( BINTREE *bt ); // returns true if success, otherwise generates an assertion failure
void BinTreeRebalance(BINTREE *tree);


void BinTreeFree(BINTREE *);

#endif  /* _BINTREE_H */
#ifdef __cplusplus
} // end extern "C"
#endif
