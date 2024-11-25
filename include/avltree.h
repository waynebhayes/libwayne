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

/* O(log n): both lookup and/or delete: returns false if element not found.
** If found and (int)pInfo==1, delete it; otherwise if pInfo != NULL, populate with new info; otherwise just return true.
*/
Boolean AvlTreeLookDel(AVLTREE *, foint key, foint *pInfo);
#define AvlTreeDelete(T,k) AvlTreeLookDel((T),(k),(foint*)1)
#define AvlTreeLookup(T,k,f) AvlTreeLookDel((T),(k), (f))

/*
** AvlTreeTraverse: Traverse an AVL tree, calling your function pointer (pFointTraversalFcn) on each
** element, in order.
*/
Boolean AvlTreeTraverse (foint globals, AVLTREE *bt, pFointTraverseFcn);
Boolean AvlTreeSanityCheck ( AVLTREE *bt ); // returns true if success, otherwise generates an assertion failure

void AvlTreeFree(AVLTREE *);

#endif  /* _AVLTREE_H */
#ifdef __cplusplus
} // end extern "C"
#endif
