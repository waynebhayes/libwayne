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
** Wayne Hayes, wayne@cs.utoronto.ca (preffered), or wayne@cs.toronto.edu
*/

/* AVL tree algorithms from Lewis & Denenberg
*/
#include <string.h>
#include "avltree.h"
#include <math.h> // for logarithm

static foint CopyInt(foint i)
{
    return i;
}

static void FreeInt(foint i) {}

static int CmpInt(foint i, foint j) { return i.i - j.i; }


AVLTREE *AvlTreeAlloc(pCmpFcn cmpKey,
    pFointCopyFcn copyKey, pFointFreeFcn freeKey,
    pFointCopyFcn copyInfo, pFointFreeFcn freeInfo)
{
    AVLTREE *tree = Malloc(sizeof(AVLTREE));
    tree->root = NULL;
    tree->cmpKey = cmpKey ? cmpKey : CmpInt;
    tree->copyKey = copyKey ? copyKey : CopyInt;
    tree->freeKey = freeKey ? freeKey : FreeInt;
    tree->copyInfo = copyInfo ? copyInfo : CopyInt;
    tree->freeInfo = freeInfo ? freeInfo : FreeInt;
    tree->n = 0;
    return tree;
}

// NOTE: in all the below, a capital letter variable name is the locative (address of) the same name in lower case.
// This macro assigns the pointer p to q along with the associated locative (which is listed first)
// Note the order of these two assignments is important, do not swap them.
#define AssignLocative(P,p,q) (((P)=&(q)),((p)=(q)))

// Rotate the tree around locative node P by d = +/-1
static void Rotate(AVLTREENODE **P, AVLTREENODE *p, int d) {
    AVLTREENODE **CP, *cp, **GCP, *gcp; // temporary copies of child + grandchild of P for swapping
    if(d==-1) {
	AssignLocative(CP,cp,p->right);
	AssignLocative(GCP,gcp,p->right->left);
	*P=p->right; *CP=p->right->left; *GCP=p;
    } else {
	assert(d==1);
	AssignLocative(CP,cp,p->left);
	AssignLocative(GCP,gcp,p->left->right);
	*P=p->left; *CP=p->left->right; *GCP=p;
    }
}

void AvlTreeInsert(AVLTREE *tree, foint key, foint info)
{
    AVLTREENODE *p = tree->root, **P = &(tree->root), // P is a locative used to trace the path
	*a,**A, *b,**B, *c,**C, *r,**R; // temporary node pointers and their locatives
    int cmp; // temporary storage of a key comparison
    Boolean critNodeFound=false;

    while(p && (cmp = tree->cmpKey(key, p->key)))
    {
	if(p->balance != 0) {
	    critNodeFound=true;
	    AssignLocative(A,a,p); // locative A points to a critical node
	}
	if(cmp < 0) AssignLocative(P,p,p->left);
	else        AssignLocative(P,p,p->right);
    }
    if(p) // key already exists, just update info
    {
	assert(cmp==0);
	assert(cmp == tree->cmpKey(key, p->key));
	tree->freeInfo(p->info);
	p->info = tree->copyInfo(info);
	return;
    }

    p = (AVLTREENODE*) Calloc(1,sizeof(AVLTREENODE)); // insert a new leaf
    p->key = tree->copyKey(key);
    p->info = tree->copyInfo(info);
    p->left = p->right = NULL;
    p->balance = 0;

// This is the macro that implements the pseudo-code (d,Q) <- K::P in Lewis+Denenberg Algorithm 7.2 (page 227)
#define AssignBalance(d,Q,q,K,P,p) { int ABcmp=tree->cmpKey((K),(p)->key);\
    (d) = (ABcmp==0?0: (ABcmp<0?-1:1)); \
    if(ABcmp<0){AssignLocative(Q,q,p->left );}else \
    if(ABcmp>0){AssignLocative(Q,q,p->right);}else \
             {AssignLocative(Q,q,p       );}\
    }

    // Now rotate and adjust balance at critical node, if any
    if(!critNodeFound) { *P=p; AssignLocative(R,r,p);}
    else {
	int d1,d2,d3;
	AssignBalance(d1,C,c,key,A,a); // C is a locative that points to the CHILD of the critical node
	if(a->balance != d1) { a->balance=0; AssignLocative(R,r,p); } // no rotation necessary
	else { // rotation necessary
	    AssignBalance(d2,B,b,key,C,c); // B is child of C in search direction
	    if(d2==d1) { // d2!=0, single rotation
		a->balance = 0;
		AssignLocative(R,r,b);
		Rotate(A, a, -d1);
	    }
	    else { // d2==-d1, double rotation
		AssignBalance(d3,R,r,key,B,b);
		if(d3==d2) {
		    a->balance = 0;
		    c->balance = d1;
		} else if(d3==-d2) a->balance=d2;
		else a->balance=0; // d3=0, B=R is a leaf
		Rotate(C,c, -d2);
		Rotate(A,a, -d1);
	    }
	}
    }
    // Adjust balances of nodes of balance 0 along the rest of the path
    while(tree->cmpKey(r->key,key)) AssignBalance(r->balance,R,r,key,R,r);
    tree->n++;
}


// if pInfo is 1, delete the element; NULL, just return Boolean if found; otherwise populate.
Boolean AvlTreeLookDel(AVLTREE *tree, foint key, foint *pInfo)
{
    AVLTREENODE *p = tree->root, **P = &(tree->root);
    while(p)
    {
	int cmp = tree->cmpKey(key, p->key);
	if(cmp == 0) {
	    if((long)pInfo==1) break; // delete the element
	    if(pInfo) *pInfo = p->info; // lookup with assign
	    return true;
	}
	else if(cmp < 0) AssignLocative(P,p,p->left);
	else             AssignLocative(P,p,p->right);
    }
    if(!p) return false; // node not found, nothing deleted
    // at this point we know the key has been found
    if((long)pInfo == 1) { // delete the element
	assert(tree->n > 0);
	// At this point, p points to the node we want to delete. If either child is NULL, then the other child moves up.
	if(p->left && p->right) *P = p->right; // two children, so replace node by its inorder successor
	else if(p->left)  *P = p->left;
	else if(p->right) *P = p->right;
	else *P = NULL;

	tree->freeKey(p->key);
	tree->freeInfo(p->info);
	Free(p);

	tree->n--;
    }
    return true;
}


static int AvlTreeTraverseHelper (foint globals, AVLTREENODE *p, pFointTraverseFcn f)
{
    int cont = 1;
    if(p) {
	if(p->left) cont = AvlTreeTraverseHelper(globals, p->left, f);
	if(cont) cont = f(globals, p->key, p->info);
	if(cont && p->right) cont = AvlTreeTraverseHelper(globals, p->right, f);
    }
    return cont;
}

int AvlTreeTraverse (foint globals, AVLTREE *tree, pFointTraverseFcn f)
{
    return AvlTreeTraverseHelper(globals, tree->root, f);
}

static int _avlTreeSanityNodeCount, _avlTreeSanityPhysicalNodeCount;
static Boolean AvlTreeSanityHelper ( AVLTREENODE *p, pCmpFcn cmpKey )
{
    if(p) {
	++_avlTreeSanityPhysicalNodeCount;
	if(p->left) { assert(cmpKey(p->left->key, p->key)<0); AvlTreeSanityHelper(p->left, cmpKey); }
	++_avlTreeSanityNodeCount;
	if(p->right) { assert(cmpKey(p->key, p->right->key)<0); AvlTreeSanityHelper(p->right, cmpKey); }
    }
    return true;
}

Boolean AvlTreeSanityCheck ( AVLTREE *tree)
{
    _avlTreeSanityNodeCount = 0;
    _avlTreeSanityPhysicalNodeCount = 0;
    assert(0 <= tree->n);
    AvlTreeSanityHelper(tree->root, tree->cmpKey);
    assert(_avlTreeSanityNodeCount == tree->n);
    assert(_avlTreeSanityPhysicalNodeCount == tree->n);
    return true;
}

#if 0
/* LookupKey: the tree is ordered on key, not info.  So we have to do a
** full traversal, O(size of tree), not O(log(size of tree)).  This is
** only used when errors occur.
*/
foint AvlTreeLookupKey(AVLTREE *tree, foint info)
{
    AVLTREENODE *p = tree->root;
    if(p)
    {
	if(tree->cmpKey(p->info, info) == 0)
	    return p->key;
	else
	{
	    foint key = AvlTreeLookupKey(p->left, info);
	    if(key)
		return key;
	    else
		return AvlTreeLookupKey(p->right, info);
	}
    }
    return NULL;
}
#endif


static void AvlTreeFreeHelper(AVLTREE *tree, AVLTREENODE *t)
{
    if(t)
    {
	AvlTreeFreeHelper(tree, t->left);
	AvlTreeFreeHelper(tree, t->right);
	tree->freeKey(t->key);
	tree->freeInfo(t->info);
	{assert(tree->n > 0); tree->n--; }
	free(t);
    }
}

void AvlTreeFree(AVLTREE *tree)
{
    AvlTreeFreeHelper(tree, tree->root);
    assert(tree->n == 0);
    free(tree);
}

#ifdef __cplusplus
} // end extern "C"
#endif
