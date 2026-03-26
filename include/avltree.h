// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#ifdef __cplusplus
extern "C" {
#endif
#ifndef _AVLTREE_H
#define _AVLTREE_H

#include <malloc.h>
#include <stdio.h>
#include <stdint.h>
#include "misc.h"   /* for foint */


/*-------------------  Types  ------------------*/

typedef struct _avlTreeNode
{
    foint key, info;
    struct _avlTreeNode *left, *right;
	// NOTE: balance is stored in the lower 2 bits of right;
	// we can do this because the alignment of AVLTREENODE
	// is guaranteed to be at least 4 on a 32-bit system,
	// since all members would be 4 bytes, making the lowest 2 bits unused
} AVLTREENODE;

AVLTREENODE* const getRight(AVLTREENODE* node);

void setRight(AVLTREENODE* node, AVLTREENODE* right);

const char getBalance(AVLTREENODE* node);

void setBalance(AVLTREENODE* node, char balance);

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

// replaces info if the key already exists, returns a pointer to the info inserted
foint* const AvlTreeInsert(AVLTREE *, foint key, foint info);

// returns a foint* so that you can modify the element without having to re-insert it; 
// returns NULL upon failure; deletes the element when delete is true
foint* const AvlTreeLookup(AVLTREE *, foint key);
const Boolean AvlTreeDelete(AVLTREE *, foint key);
// "safe" version, just gives a value, returns false if not found
const Boolean SAvlTreeLookup(AVLTREE *, foint key, foint* pInfo);

/*
** AvlTreeTraverse: Traverse an AVL tree, calling your function pointer (pFointTraversalFcn) on each element,
** in order. It will return -k if k elements were deleted, otherwise 0 or 1 as returned by your function.
*/
int AvlTreeTraverse (foint globals, AVLTREE *bt, pFointTraverseFcn);
Boolean AvlTreeSanityCheck ( AVLTREE *bt ); // returns true if success, otherwise generates an assertion failure

void AvlTreeFree(AVLTREE *);

#endif  /* _AVLTREE_H */
#ifdef __cplusplus
} // end extern "C"
#endif
