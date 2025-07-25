#include <stdio.h>
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

#include <stack.h>

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

// Rotate the tree around node n by d = +/-1
static void Rotate(AVLTREENODE** N, AVLTREENODE* n, int d) {
	AVLTREENODE *p, **P, *tp, **TP;

	if (d==-1)
	{
		AssignLocative(P,p,n->right);
		AssignLocative(TP,tp,n->right->left);
		*N=p; *P=tp; *TP=n;
	}
	else
	{
		assert(d==1);
		AssignLocative(P,p,n->left);
		AssignLocative(TP,tp,n->left->right);
		*N=p; *P=tp; *TP=n;
	}
}


#define MaxHeight (int)(log(tree->n+2)*4.7852-0.3277)
#ifdef VERBOSE
#define Print AvlPrint(tree->root, 0, 0)
void AvlPrint(AVLTREENODE* node, unsigned char level, char side)
{
	if (node == NULL) return;
	else if (level == 0) printf("(%i)%s\n", node->balance, node->info.s);
	else
	{
		for (int i = 0; i < level-1; i++)
		{
			printf("  ");
		}

		printf("|->(%c:%i)%s\n", side, node->balance, node->info.s);
	}

	AvlPrint(node->left, level + 1, 'L');
	AvlPrint(node->right, level + 1, 'R');
}
#endif


// This is the macro that implements the pseudo-code (d,Q) <- K::P in Lewis+Denenberg Algorithm 7.2 (page 227)
#define AssignBalance(d, q, K, p) { int ABcmp = tree->cmpKey((K), (p)->key); \
    (d) = (ABcmp == 0 ? 0:(ABcmp < 0 ? -1:1)); \
    if(ABcmp < 0) { (q) = (p)->left; } else \
    if(ABcmp > 0) { (q) = (p)->right;} else { (q) = (p); } \
    }


void AvlTreeInsert(AVLTREE *tree, foint key, foint info)
{
    AVLTREENODE *p = tree->root, **P = &(tree->root),// P is a locative used to trace the path
	*a, **A, *b, *c, **C, *r; // temporary node pointers
    int cmp; // temporary storage of a key comparison
    Boolean critNodeFound=false;

	#ifdef VERBOSE
	int traversal_steps = 0;
	#endif

	if (p && p->balance != 0)
	{
		critNodeFound=true;
		AssignLocative(A,a,tree->root);
	}

    while(p && (cmp = tree->cmpKey(key, p->key)))
    {
		#ifdef VERBOSE
		traversal_steps++;
		#endif
		
		if(cmp < 0)
		{
			if (p->left && p->left->balance != 0)
			{
				critNodeFound=true;
				AssignLocative(A,a,p->left);
			}
			AssignLocative(P,p,p->left);
		}
		else
		{
			if (p->right && p->right->balance != 0)
			{
				critNodeFound=true;
				AssignLocative(A,a,p->right);
			}
			AssignLocative(P,p,p->right);
		}
    }

	#ifdef VERBOSE
	int worst = MaxHeight;
	printf("Insertion took %i STEPS to complete with %i ELEMENTS already present; worst case is %i\n", traversal_steps, tree->n, worst);
	//assert(traversal_steps >= MinHeight && traversal_steps <= worst);
	assert(traversal_steps <= worst);
	#endif

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
	*P = p;

    // Now rotate and adjust balance at critical node, if any
	if(!critNodeFound) r=tree->root;
	else
	{
		int d1,d2,d3;

		// C is a locative that points to the CHILD of the critical node
		{ 
			int ABcmp = tree->cmpKey(key, a->key);
			d1 = (ABcmp == 0 ? 0:(ABcmp < 0 ? -1:1));
			if(ABcmp < 0) { AssignLocative(C, c, a->left); } else 
			if(ABcmp > 0) { AssignLocative(C, c, a->right);} else { c = a; }
		}

		if(a->balance != d1) // no rotation necessary
		{
			a->balance=0; r=c;
		}
		else
		{ // rotation necessary
			AssignBalance(d2,b,key,c); // B is child of C in search direction
			if(d2==d1) // d2!=0, single rotation
			{ 
				a->balance=0;
				r=b;
				Rotate(A,a,-d1);
			}
			else // d2==-d1, double rotation
			{
				AssignBalance(d3,r,key,b);
				if(d3==d2)
				{
					a->balance=0;
					c->balance=d1;
				} 
				else if(d3==-d2) a->balance=d2;
				else a->balance=0; // d3=0, B=R is a leaf
				Rotate(C,c,-d2);
				Rotate(A,a,-d1);
			}
		}
	}

    // Adjust balances of nodes of balance 0 along the rest of the path
    while(tree->cmpKey(r->key,key)) 
	{
		AssignBalance(r->balance,r,key,r);
	}

    tree->n++;
}


Boolean AvlTreeLookDel(AVLTREE *tree, foint key, foint *pInfo)
{
	if ((long)pInfo==1) return AvlTreeDelete(tree, key);
	else return AvlTreeLookup(tree, key, pInfo);
}


Boolean AvlTreeLookup(AVLTREE *tree, foint key, foint *pInfo)
{
    AVLTREENODE *p = tree->root;

    while(p)
    {
		int cmp = tree->cmpKey(key, p->key);
		if(cmp == 0)
		{
			if(pInfo) *pInfo = p->info;
			return true;
		}
		else if(cmp < 0) p = p->left;
		else             p = p->right;
	}
    
    return false;
}


Boolean AvlTreeDelete(AVLTREE *tree, foint key)
{
	AVLTREENODE *p = tree->root, **P = &(tree->root); AVLTREENODE *parent = tree->root;

    while(p)
    {
		int cmp = tree->cmpKey(key, p->key);
		if(cmp == 0) break;

		parent = p;
		if(cmp < 0) AssignLocative(P,p,p->left);
		else AssignLocative(P,p,p->right);
	}
    
    if(!p) return false; // node not found, nothing deleted
	assert(tree->n > 0);

	// At this point, p points to the node we want to delete
	if (p->right == NULL) *P = p->left;
	else // Find the inorder successor. Q is a locative
	{
		AVLTREENODE *q, **Q;
		AssignLocative(Q,q,p->right);
		parent = q; // The "parent" only marks the node whose balance will change at this point

		while (q->left != NULL)
		{
			parent = q;
			AssignLocative(Q,q,q->left);
		}

		*P=q; *Q=q->right;
		q->balance=p->balance;
		q->left=p->left; q->right=p->right;
	}

	tree->freeKey(p->key);
	tree->freeInfo(p->info);
	Free(p);

	// Update balance then rotate if necessary
	if (--tree->n == 0) return true;
	else if (parent->balance == 0) 
	{
		parent->balance += (parent->right==NULL) ? -1:1;
		return true;
	}
	else // The total height of the tree has changed; need to adjust balance and/or rotate down the whole tree
	{
		if (parent->right == NULL && parent->left == NULL)
			parent->balance = 0;
		else
			parent->balance += (parent->right==NULL) ? -1:1;

		STACK *nodePath = StackAlloc(MaxHeight);
		StackPush(nodePath, (foint){.v=&(tree->root)}); // Contains locatives
		p = tree->root;
		int cmp;

		#ifdef VERBOSE
		int traversal_steps = 0;
		#endif

		while ((cmp=tree->cmpKey(parent->key, p->key)))
		{
			#ifdef VERBOSE
			traversal_steps++;
			#endif

			if(cmp < 0)
			{
				StackPush(nodePath, (foint){.v=&(p->left)});
				p = p->left;
			}
			else
			{
				StackPush(nodePath, (foint){.v=&(p->right)});
				p = p->right;
			}
		}

		#ifdef VERBOSE
		int worst = MaxHeight;
		printf("Backtracking deletion took %i STEPS to complete with %i ELEMENTS; worst case is %i\n", traversal_steps, tree->n, worst);
		assert(traversal_steps <= worst);
		#endif
		
		while (StackSize(nodePath) > 0 && p->balance != 0)
		{
			int d1,d2,d3;
			AVLTREENODE *b, *c, **C;
			P = StackPop(nodePath).v; p = *P;

			// C is a locative that points to the CHILD of the critical node
			d1 = p->balance;
			p->balance = (d1 == 0 ? 0:(d1 < 0 ? -1:1));
			if(d1 < 0) AssignLocative(C, c, p->left);
			else if(d1 > 0) AssignLocative(C, c, p->right);
			else continue;

			if (c == NULL) { p->balance = 0; continue; }
			else 
			{
				if (d1 < -1)
				{
					d1 = -1; d2 = d1; c->balance = d1;
					b = c->left;
				}
				else if (d1 > 1)
				{
					d1 = 1; d2 = d1; c->balance = d1;
					b = c->right;
				}
				else
				{
					d2 = c->balance;
					b = d2 > 0 ? c->right : c->left;
					assert(d2!=0);
				}

				if(d2==d1) // d2!=0, single rotation
				{ 
					p->balance=0;
					Rotate(P, p, -d1);
				}
				else // d2==-d1, double rotation
				{
					if (b == NULL) { continue; }
					else { d3 = b->balance; }

					if(d3==d2)
					{
						p->balance=0;
						c->balance=d1;
					} 
					else if(d3==-d2) p->balance=d2;
					else p->balance=0; // d3=0, B=R is a leaf
					Rotate(C, c, -d2);
					Rotate(P, p, -d1);
				}
			}
		}

		StackFree(nodePath);
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
	assert(tree->n > 0);
	tree->n--;
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
