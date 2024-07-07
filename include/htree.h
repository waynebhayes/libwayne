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
// For now, to keep things simple, we assume the key is *always* a string, and the data is always a simple foint.
// That way all key operations use strcmp, strdup, and free.

#include "bintree.h"

/*-------------------  Types  ------------------*/

typedef struct _hTree
{
    BINTREE *tree;
    unsigned char depth; // we're going to assume it's less than 255 layers deep, OK?
    pCmpFcn cmpKey;
    pFointCopyFcn copyKey, copyInfo;
    pFointFreeFcn freeKey, freeInfo;
    //int n; // total number of elements across all sub-trees.
} HTREE;

/*-----------   Function Prototypes  -----------*/

HTREE *HTreeAlloc(int depth, pCmpFcn cmpKey, pFointCopyFcn copyKey, pFointFreeFcn freeKey,
    pFointCopyFcn copyInfo, pFointFreeFcn freeInfo);

// key is an array with exactly "depth" elements, info is what you want to put at the lowest level.
void HTreeInsert(HTREE *, foint keys[], foint info);

// implements both lookup and delete: if not found, return false. Otherwise, if (int)pInfo==1, delete the element.
// Otherwise, if pInfo!=NULL, populate it with new info; otherwise just return true (found).
Boolean HTreeLookDel(HTREE *, foint keys[], foint *pInfo);
#define HTreeLookup(h,k,p) HTreeLookDel((h),(k),(p))
#define HTreeDelete(h,k)   HTreeLookDel((h),(k),(pInfo*)1)

// number of elements in trees down the hierarchy along key path; returns number of sizes[] we managed to fill,
// which should be equal to depth.
int HTreeSizes(HTREE *, foint keys[], int sizes[]);

void HTreeFree(HTREE *);

#endif  /* _HTREE_H */
#ifdef __cplusplus
} // end extern "C"
#endif
