// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#ifdef __cplusplus
extern "C" {
#endif
#ifndef _TREE_H
#define _TREE_H

// Include this

#if !defined(HTREE_USES_AVL) && !defined(TREE_USES_AVL)
#define TREE_USES_AVL 1 // set to 0 to use old "copy to balance" bintree -- which appears faster though uses more RAM
#else
#define TREE_USES_AVL 0
#endif

#if TREE_USES_AVL
#include "avltree.h"
#define TREETYPE AVLTREE
#define TreeAlloc AvlTreeAlloc
#define TreeInsert AvlTreeInsert
#define TreeLookup AvlTreeLookup
#define TreeLookDel AvlTreeLookDel
#define TreeDelete AvlTreeDelete
#define TreeTraverse AvlTreeTraverse
#define TreeFree AvlTreeFree
#else
#include "bintree.h"
#define TREETYPE BINTREE
#define TreeAlloc BinTreeAlloc
#define TreeInsert BinTreeInsert
#define TreeLookup BinTreeLookup
#define TreeLookDel BinTreeLookDel
#define TreeTraverse BinTreeTraverse
#define TreeFree BinTreeFree
#endif

#endif  /* _TREE_H */
#ifdef __cplusplus
} // end extern "C"
#endif
