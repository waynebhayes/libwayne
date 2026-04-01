// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#ifdef __cplusplus
extern "C" {
#endif
#ifndef _HTREE_H
#define _HTREE_H

// A hierarchical binary tree is a binary tree in which the *data* member of each node is itself another binary tree.
// It is best used when each key K at one level has associated with it many sub-keys of a different sort; these sub-keys
// are associated only with K. And so it goes down the hierarchy.
// The hierarchy has a fixed depth, and when you insert or lookup an element, you specify a depth you wish to go,
// and an array of keys with length equal to the depth you're searching.
// Both keys and data are foints; the user is responsible for knowing what's actually stored.

#include "tree.h" // switches between AVL and BINTREE

/*-------------------  Types  ------------------*/

typedef struct _hTree
{
    TREETYPE *tree;
    unsigned char depth; // we're going to assume it's less than 255 layers deep, OK?
    pCmpFcn cmpKey;
    pFointCopyFcn copyKey, copyInfo;
    pFointFreeFcn freeKey, freeInfo;
    int n; // total number of elements across all sub-trees.
} HTREE;

/*-----------   Function Prototypes  -----------*/

HTREE *HTreeAlloc(unsigned char depth, pCmpFcn cmpKey, pFointCopyFcn copyKey, pFointFreeFcn freeKey,
    pFointCopyFcn copyInfo, pFointFreeFcn freeInfo);

// key is an array with exactly "depth" elements, info is what you want to put at the lowest level.
// returns a pointer to the info just inserted
foint* const HTreeInsert(HTREE *, foint keys[], foint info);

<<<<<<< HEAD
// returns a foint* so that you can modify the element without having to re-insert it; 
// returns NULL upon failure; deletes the element if delete is true
foint* HTreeLookDel(HTREE *, foint keys[], unsigned char targetDepth, Boolean delete);
// "safe" version, only gives a value, returns false if not found
const Boolean SHTreeLookup(HTREE*, foint keys[], unsigned char targetDepth, foint* pInfo);
#define HTreeLookup(h,k) HTreeLookDel((h),(k),0,false)
#define HTreeDelete(h,k) HTreeLookDel((h),(k),0,true)
#define HTreeDeleteInner(h,k,d) HTreeLookDel((h),(k),(d),true)
=======
// implements both lookup and delete: if not found, return false. Otherwise, if (int)pInfo==1, delete the element.
// Otherwise, if pInfo!=NULL, populate it with new info; otherwise just return true (found).
Boolean HTreeLookDel(HTREE *, foint keys[], foint *pInfo);
#define HTreeLookup(h,k,p) HTreeLookDel((h),(k),(p))
#define HTreeDelete(h,k)   HTreeLookDel((h),(k),(foint*)1)
>>>>>>> upstream/master

// number of elements in trees down the hierarchy along key path; returns number of sizes[] we managed to fill,
// which should be equal to depth.
int HTreeSizes(HTREE *, foint keys[], int sizes[]);

void HTreeFree(HTREE *);

#endif  /* _HTREE_H */
#ifdef __cplusplus
} // end extern "C"
#endif
