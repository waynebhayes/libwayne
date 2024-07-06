#ifdef __cplusplus
extern "C" {
#endif
#ifndef _AVLTREE_H
#define _AVLTREE_H

#include <malloc.h>
#include <stdio.h>
#include "misc.h"   /* for foint */


/*-------------------  Types  ------------------*/

typedef struct _avlTreeNode
{
    foint key, info;
    struct _avlTreeNode *left, *right;
    char balance:2; // 2 bits to store an int between -2 and +1
} AVLTREENODE;

typedef struct _avlTree
{
    unsigned n; // total number of entries in the tree
    AVLTREENODE *root;
    pCmpFcn cmpKey;
    pFointCopyFcn copyKey, copyInfo;
    pFointFreeFcn freeKey, freeInfo;
} AVLTREE;

/*-----------   Function Prototypes  -----------*/

AVLTREE *AvlTreeAlloc(pCmpFcn cmpKey, pFointCopyFcn copyKey, pFointFreeFcn freeKey,
	    pFointCopyFcn copyInfo, pFointFreeFcn freeInfo);

void AvlTreeInsert(AVLTREE *, foint key, foint info); // replaces info if the key already exists

/* O(log n); returns false if failure, and true if found and then assigns to *pInfo if *pInfo is non-NULL */
Boolean AvlTreeLookup(AVLTREE *, foint key, foint *pInfo);

// O(log n), returns true if deleted successfully, false if item not found.
Boolean AvlTreeDelete(AVLTREE *, foint key);

/*
** AvlTreeTraverse: Traverse an AVL tree, calling your function pointer (pFointTraversalFcn) on each
** element, in order.
*/
Boolean AvlTreeTraverse ( AVLTREE *bt, pFointTraverseFcn);
Boolean AvlTreeSanityCheck ( AVLTREE *bt ); // returns true if success, otherwise generates an assertion failure

void AvlTreeFree(AVLTREE *);

#endif  /* _AVLTREE_H */
#ifdef __cplusplus
} // end extern "C"
#endif
