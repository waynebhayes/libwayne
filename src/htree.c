#ifdef __cplusplus
extern "C" {
#endif
#include "misc.h"
#include "htree.h" // bintree or avltree is included there (as appropriate)

/*-------------------  Types  ------------------*/

HTREE *HTreeAlloc(int depth, pCmpFcn cmpKey, pFointCopyFcn copyKey, pFointFreeFcn freeKey,
    pFointCopyFcn copyInfo, pFointFreeFcn freeInfo)
{
    assert(depth>0);
    HTREE *h = Calloc(1, sizeof(HTREE));
    h->depth = depth;
    // Be nice and create the top-level tree.
    h->cmpKey = cmpKey; h->copyKey = copyKey; h->freeKey = freeKey;
    h->copyInfo = copyInfo; h->freeInfo = freeInfo;
    h->tree = TreeAlloc(cmpKey, copyKey, freeKey, copyInfo, freeInfo);
    return h;
}

static void HTreeInsertHelper(HTREE *h, int currentDepth, TREETYPE *tree, foint keys[], foint data)
{
    assert(tree && 0 <= currentDepth && currentDepth < h->depth);
    if(currentDepth == h->depth-1) // we're hit the lowest level tree; its data elements are the final elements.
	TreeInsert(tree, keys[currentDepth], data);
    else {
	// Otherwise, we are NOT at the lowest level tree; the data members of these nodes are themselves other trees,
	// so to find the next tree we use the key at this level to *look up* the binary tree at the next level down
	foint nextLevel;
	TREETYPE *nextTree;
	if(!TreeLookup(tree, keys[currentDepth], &nextLevel)) {
	    nextTree = TreeAlloc(h->cmpKey, h->copyKey, h->freeKey, h->copyInfo, h->freeInfo);
	    nextLevel.v = nextTree;
	    TreeInsert(tree, keys[currentDepth], nextLevel);
	}
	else
	    nextTree = nextLevel.v;
	assert(nextTree);
	HTreeInsertHelper(h, currentDepth+1, nextTree, keys, data);
    }
}

// key is an array with exactly "depth" elements, data is what you want to put at the lowest level.
void HTreeInsert(HTREE *h, foint keys[], foint data)
{
    foint fkeys[h->depth]; int i; for(i=0; i < h->depth; i++) fkeys[i] = keys[i];
    HTreeInsertHelper(h, 0, h->tree, fkeys, data);
}

static Boolean HTreeLookDelHelper(HTREE *h, int currentDepth, TREETYPE *tree, foint keys[], foint *pData)
{
    assert(tree && 0 <= currentDepth && currentDepth < h->depth);
    if(currentDepth == h->depth-1) // we've hit the lowest level tree; its data elements are the final elements.
	return TreeLookDel(tree, keys[currentDepth], pData);
    else {
	foint nextLevel;
	TREETYPE *nextTree;
	if(!TreeLookup(tree, keys[currentDepth], &nextLevel))
	    return false;
	else
	    nextTree = nextLevel.v;
	assert(nextTree);
	return HTreeLookDelHelper(h, currentDepth+1, nextTree, keys, pData);
    }
}

Boolean HTreeLookDel(HTREE *h, foint keys[], foint *pData)
{
    foint fkeys[h->depth]; int i; for(i=0; i < h->depth; i++) fkeys[i] = keys[i];
    return HTreeLookDelHelper(h, 0, h->tree, fkeys, pData);
}

static int HTreeSizesHelper(HTREE *h, int currentDepth, TREETYPE *tree, foint keys[], int sizes[])
{
    assert(tree && 0 <= currentDepth && currentDepth < h->depth);
    sizes[currentDepth] = tree->n;
    if(currentDepth == h->depth-1) // we're hit the lowest level tree; its data elements are the final elements.
	return 1;
    else {
	foint nextLevel;
	TREETYPE *nextTree;
	if(!TreeLookup(tree, keys[currentDepth], &nextLevel))
	    return 1;
	else
	    nextTree = nextLevel.v;
	assert(nextTree);
	return 1 + HTreeSizesHelper(h, currentDepth+1, nextTree, keys, sizes);
    }
}

int HTreeSizes(HTREE *h, foint keys[], int sizes[])
{
    foint fkeys[h->depth]; int i; for(i=0; i < h->depth; i++) fkeys[i] = keys[i];
    return HTreeSizesHelper(h, 0, h->tree, fkeys, sizes);
}


static void HTreeFreeHelper(HTREE *h, int currentDepth, TREETYPE *tree);
static HTREE *_TraverseH;
static int _TraverseDepth;
static void TraverseFree(foint key, foint data) {
    assert(_TraverseDepth < _TraverseH->depth);
    TREETYPE *t = data.v;
    int depth = _TraverseDepth;
    HTreeFreeHelper(_TraverseH, _TraverseDepth+1, t);
    _TraverseDepth = depth;
}

static void HTreeFreeHelper(HTREE *h, int currentDepth, TREETYPE *tree)
{
    assert(tree && 0 <= currentDepth && currentDepth < h->depth);
    if(currentDepth == h->depth-1) // we're hit the lowest level tree; its data elements are the final elements.
	TreeFree(tree);
    else {
	_TraverseH = h; _TraverseDepth = currentDepth;
	TreeTraverse(tree, (pFointTraverseFcn) TraverseFree);
    }
}

void HTreeFree(HTREE *h)
{
    HTreeFreeHelper(h, 0, h->tree);
}
#ifdef __cplusplus
} // end extern "C"
#endif
