// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

// see getRight; to be used on a locative which could be the right child of another node
AVLTREENODE* const getNodeFromLocative(AVLTREENODE** loc)
{
	return (AVLTREENODE*)((uintptr_t)*loc & ~3);
}

// see setRight; to be used on a locative which could be the right child of another node
void setNodeFromLocative(AVLTREENODE** loc, AVLTREENODE* node)
{
	*loc = (AVLTREENODE*)( ((uintptr_t)node & ~3) | ((uintptr_t)*loc & 3) );
}

// & ~3 = discard last 2 bits
AVLTREENODE* const getRight(AVLTREENODE* node)
{
	return (AVLTREENODE*)((uintptr_t)node->right & ~3);
}

// Give `right` the balance of node->right, then set node->right to `right`
void setRight(AVLTREENODE* node, AVLTREENODE* right)
{
	node->right = (AVLTREENODE*)( ((uintptr_t)right & ~3) | ((uintptr_t)node->right & 3) );
}

// & 3 = discard everything but the last 2 bits
// | -4 = turn all bits before the last 2 to 1: this only happens for numbers which should be negative (2nd LSB = 1)
const char getBalance(AVLTREENODE* node)
{
	const char balance = (uintptr_t)node->right & 3;
	return balance | (-4 * ((balance & 2) >> 1));
}

// Discard irrelevant bits in each then combine
void setBalance(AVLTREENODE* node, char balance)
{
	node->right = (AVLTREENODE*)( ((uintptr_t)node->right & ~3) | (balance & 3) );
}

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
#define RightAssignLocative(P,p,q) (((P)=&(q->right)),((p)=getRight((q))))

// Rotate the tree around node n by d = +/-1
static void Rotate(AVLTREENODE** N, AVLTREENODE* n, int d) {
	AVLTREENODE *p, **P, *tp, **TP;

	if (d==-1)
	{
		RightAssignLocative(P,p,n);
		AssignLocative(TP,tp,getRight(n)->left);
		setNodeFromLocative(N, p); setNodeFromLocative(P, tp); *TP=n;
	}
	else
	{
		assert(d==1);
		AssignLocative(P,p,n->left);
		RightAssignLocative(TP,tp,n->left);
		setNodeFromLocative(N, p); *P=tp; setNodeFromLocative(TP, n);
	}
}


#define MaxHeight (int)(log(tree->n+2)*4.7852-0.3277)
#ifdef VERBOSE
#define Print AvlPrint(tree->root, 0, 0)
void AvlPrint(AVLTREENODE *node, unsigned char level, char side)
{
	if (node == NULL) return;
	else if (level == 0) printf("(%i)%i\n", getBalance(node), node->info);
	else
	{
		for (int i = 0; i < level-1; i++)
		{
			printf("  ");
		}

		printf("|->(%c:%i)%i\n", side, getBalance(node), node->info);
	}

	AvlPrint(node->left, level + 1, 'L');
	AvlPrint(getRight(node), level + 1, 'R');
}


// returns height of subtree to calculate balance where needed
unsigned char CheckBalance(AVLTREENODE *node, unsigned char level)
{
	if (getRight(node) && node->left)
	{
		unsigned char left=CheckBalance(node->left,level+1);
		unsigned char right=CheckBalance(getRight(node),level+1);
		level=left;
		if (left>right) { level=left; assert(getBalance(node)==-1); }
		else if (left<right) { level=right; assert(getBalance(node)==1); }
		else assert(getBalance(node)==0);
		assert(abs(left - right) < 2);
	}
	else if (node->left==NULL && getRight(node)==NULL)
		assert(getBalance(node)==0);
	else if (getRight(node)==NULL)
	{
		assert(getBalance(node)==-1);
		level=CheckBalance(node->left,level+1);
	}
	else
	{
		assert(getBalance(node)==1);
		level=CheckBalance(getRight(node),level+1);
	}

	return level;
}
#endif


// This is the macro that implements the pseudo-code (d,Q) <- K::P in Lewis+Denenberg Algorithm 7.2 (page 227)
#define AssignBalance(d, q, K, p) { int ABcmp = tree->cmpKey((K), (p)->key); \
    (d) = (ABcmp == 0 ? 0:(ABcmp < 0 ? -1:1)); \
    if(ABcmp < 0) { (q) = (p)->left; } else \
    if(ABcmp > 0) { (q) = getRight((p)); } else { (q) = (p); } \
    }


foint* const AvlTreeInsert(AVLTREE *tree, foint key, foint info)
{
    AVLTREENODE *p = tree->root, **P = &(tree->root), // P is a locative used to trace the path
	*a = NULL, **A = NULL, // Critical node
	*r = NULL; // Starting point for nodes that need their balance recalculated
    int cmp; // Temporary storage of a key comparison

	#ifdef VERBOSE
	int traversal_steps = 0;
	#endif

	if (p && getBalance(p) != 0)
		AssignLocative(A,a,tree->root);

    while(p && (cmp = tree->cmpKey(key, p->key)))
    {
		#ifdef VERBOSE
		traversal_steps++;
		#endif
		
		if(cmp < 0)
		{
			if (p->left && getBalance(p->left) != 0)
				AssignLocative(A,a,p->left);

			AssignLocative(P,p,p->left);
		}
		else
		{
			if (getRight(p) && getBalance(getRight(p)) != 0)
				RightAssignLocative(A,a,p);

			RightAssignLocative(P,p,p);
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
		return &p->info;
    }

    p = (AVLTREENODE*) Calloc(1,sizeof(AVLTREENODE)); // insert a new leaf
    p->key = tree->copyKey(key);
    p->info = tree->copyInfo(info);
    p->left = p->right = NULL; // also sets balance to 0
	setNodeFromLocative(P, p);

    // Now rotate and adjust balance at critical node, if any
	if(a == NULL) r=tree->root;
	else
	{
		char d1,d2,d3;
		AVLTREENODE *c, **C = NULL; // Children of critical node

		// C is a locative that points to the CHILD of the critical node
		{ 
			int ABcmp = tree->cmpKey(key, a->key);
			d1 = (ABcmp == 0 ? 0:(ABcmp < 0 ? -1:1));
			if(ABcmp < 0) { AssignLocative(C, c, a->left); } else 
			if(ABcmp > 0) { RightAssignLocative(C, c, a);} else { c = a; }
		}

		if(getBalance(a) != d1) // no rotation necessary
		{
			setBalance(a, 0); r=c;
		}
		else
		{ // rotation necessary
			AVLTREENODE *b;
			AssignBalance(d2,b,key,c); // B is child of C in search direction

			if(d2==d1) // d2!=0, single rotation
			{ 
				setBalance(a, 0);
				r=b;
				Rotate(A,a,-d1);
			}
			else // d2==-d1, double rotation
			{
				AssignBalance(d3,r,key,b);
				if(d3==d2)
				{
					setBalance(a, 0);
					setBalance(c, d1);
				} 
				else if(d3==-d2) setBalance(a, d2);
				else setBalance(a, 0); // d3=0, B=R is a leaf
				Rotate(C,c,-d2);
				Rotate(A,a,-d1);
			}
		}
	}

    // Adjust balances of nodes of balance 0 along the rest of the path
    while(tree->cmpKey(r->key,key)) 
	{
		int ABcmp = tree->cmpKey(key, r->key);
		setBalance(r, ABcmp == 0 ? 0:(ABcmp < 0 ? -1:1));
		if(ABcmp < 0) { r = r->left; } else
		if(ABcmp > 0) { r = getRight(r); }
	}

	#ifdef VERBOSE
	CheckBalance(tree->root, 0);
	#endif

    tree->n++;
	return &p->info;
}


foint* const AvlTreeLookup(AVLTREE *tree, foint key)
{
    AVLTREENODE *p = tree->root;

    while(p)
    {
		int cmp = tree->cmpKey(key, p->key);
		if(cmp == 0) return &p->info;
		else if(cmp < 0) p = p->left;
		else             p = getRight(p);
	}
    
    return NULL;
}


const Boolean AvlTreeDelete(AVLTREE *tree, foint key)
{
	AVLTREENODE *p = tree->root, **P = &(tree->root); AVLTREENODE *parent = tree->root;

    while(p)
    {
		int cmp = tree->cmpKey(key, p->key);
		if(cmp == 0) break;

		parent = p;
		if(cmp < 0) AssignLocative(P,p,p->left);
		else RightAssignLocative(P,p,p);
	}
    
    if(!p) return false; // node not found, nothing deleted
	assert(tree->n > 0);

	// At this point, p points to the node we want to delete
	if (getRight(p) == NULL) 
		setNodeFromLocative(P, p->left);
	else // Find the inorder successor. Q is a locative
	{
		AVLTREENODE *q, **Q;
		RightAssignLocative(Q,q,p);
		parent = q; // The "parent" only marks the node whose balance will change at this point

		while (q->left != NULL)
		{
			parent = q;
			AssignLocative(Q,q,q->left);
		}

		setNodeFromLocative(P, q); setNodeFromLocative(Q, q->right);
		q->left=p->left; q->right=p->right; // Directly using p->right OK because we want the balance as well
	}

	tree->freeKey(p->key);
	tree->freeInfo(p->info);
	Free(p);
	tree->n--;

	if (parent == p) // Edge case where <= 2 nodes are left and we delete the root
	{
		#ifdef VERBOSE
		if (tree->n > 0) CheckBalance(tree->root, 0);
		#endif
		return true; 
	}

	// Update balance then rotate if necessary
	if (getBalance(parent) == 0)
		setBalance(parent, (getRight(parent)==*P || parent==*P) ? -1:1);
	else
	{ // The height of the subtree has changed; need to check balance potentially all the way to the root
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
				p = getRight(p);
			}
		}

		#ifdef VERBOSE
		int worst = MaxHeight;
		printf("Backtracking deletion took %i STEPS to complete with %i ELEMENTS; worst case is %i\n", traversal_steps, tree->n, worst);
		assert(traversal_steps <= worst);
		#endif
		
		parent = NULL; // NOTE: as of this point, parent = previous node along path back up to root
		while (StackSize(nodePath) > 0)
		{
			P = StackPop(nodePath).v; p = getNodeFromLocative(P);

			// C is a locative that points to the CHILD of the critical node
			char d1,d2,d3; // Balance factors
			AVLTREENODE *c = NULL, **C = NULL;
			d1 = getBalance(p);
			if(d1 < 0) AssignLocative(C, c, p->left);
			else if (d1 > 0) RightAssignLocative(C, c, p);

			if (getBalance(p) == 0) // Node now imbalanced, height not changed
			{
				setBalance(p, (getRight(p)==parent) ? -1:1);
				break;
			}
			else if (c == NULL) // Imbalance removed
			{
				setBalance(p, 0);
			}
			else if (getBalance(c) == 0 && parent == c)
			{ // Height decreased where previously there was an imbalance, although there may still be
				if (p->left && getRight(p)) setBalance(p, 0);
			} 
			else // Two nodes are imbalanced, rotation likely needed
			{
				AVLTREENODE *b;

				if (p->left && getRight(p))
				{
					b=p->left; parent=getRight(p);
					d2=d3=0;
					// NOTE: in this scope only d2 = height of left subtree, d3 = height of right subtree
					// b and parent navigate the left and right subtrees respectively

					while (b != NULL || parent != NULL)
					{
						if (b != NULL) 
						{
							b=(getBalance(b) < 0) ? b->left : getRight(b);
							d2++;
						}

						if (parent != NULL)
						{
							parent=(getBalance(parent) < 0) ? parent->left : getRight(parent);
							d3++;
						}
					}
					
					if (d2 == d3)
					{ // This node is actually now balanced, no rotation needed
						setBalance(p, 0); parent = p;
						continue;
					}
				}

				// Rotation is caused by height change on the opposite side
				if (getBalance(c) == 0) d2 = getBalance(p);
				else d2 = getBalance(c);
				b = d2 > 0 ? getRight(c) : c->left;

				if (b == NULL)  // Deleted node was just before leaf, rotation not possible
					setBalance(c, 0);
				else if(d2==d1) // d2!=0, single rotation
				{ 
					if ( getBalance(c) == 0)
					{
						setBalance(c, -d1);
						setBalance(p, d1);
						Rotate(P, p, -d1);
						break;
					}

					setBalance(c, 0); setBalance(p, 0);
					Rotate(P, p, -d1);
					parent = c; continue;
				}
				else // d2==-d1, double rotation
				{
					d3 = getBalance(b);
					if(d3==d2)
					{
						setBalance(p, 0);
						setBalance(c, d1);
					} 
					else if(d3==-d2) { setBalance(p, d2); setBalance(c, 0); }
					else { setBalance(p, 0); setBalance(c, 0); } // d3=0, B=R is a leaf
					setBalance(b, 0);
					Rotate(C, c, -d2);
					Rotate(P, p, -d1);
					parent = b; continue;
				}
			}
			parent = p;
		}

		StackFree(nodePath);
	}

	#ifdef VERBOSE
	if (tree->n > 0) CheckBalance(tree->root, 0);
	#endif

	return true;
}


const Boolean SAvlTreeLookup(AVLTREE *tree, foint key, foint* pInfo)
{
	foint* result = AvlTreeLookup(tree, key);

	if (result != NULL) 
	{
		*pInfo = *result;
		return true;
	}
	else
		return false;
}


static int AvlTreeTraverseHelper (foint globals, AVLTREENODE *p, pFointTraverseFcn f)
{
    int cont = 1;
    if(p) {
	if(p->left) cont = AvlTreeTraverseHelper(globals, p->left, f);
	if(cont) cont = f(globals, p->key, p->info);
	if(cont && getRight(p)) cont = AvlTreeTraverseHelper(globals, getRight(p), f);
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
	if(getRight(p)) { assert(cmpKey(p->key, getRight(p)->key)<0); AvlTreeSanityHelper(getRight(p), cmpKey); }
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
	AvlTreeFreeHelper(tree, getRight(t));
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
