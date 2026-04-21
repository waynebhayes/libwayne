// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#include "misc.h"
#include "avltree.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define lineLen 40

int traverse(foint globals, foint key, foint data)
{
	printf("del %s:%i\n", key.s, data.i);
	return -1;
}

int main(int argc, char *argv[])
{
    assert(argc == 2);
{
    FILE *fp = fopen(argv[1], "r");
	AVLTREE *tree = AvlTreeAlloc((pCmpFcn)strcmp, (pFointCopyFcn)strdup, (pFointFreeFcn)free, NULL, NULL);
	AVLTREE *copy = AvlTreeAlloc((pCmpFcn)strcmp, (pFointCopyFcn)strdup, (pFointFreeFcn)free, NULL, NULL);
    char buf[lineLen], bufs[1000*BUFSIZ][lineLen];
    foint key, data;
    int lines=0;

    while(fscanf(fp, "%s %d", bufs[lines], &data.i) == 2) {
	foint key = (foint)(key.s=bufs[lines++]);
	AvlTreeInsert(tree, key, data);
	AvlTreeInsert(copy, key, data);
	AvlTreeSanityCheck(tree);
    }
    fclose(fp);
    AvlTreeSanityCheck(tree);
    // No rebalance needed
    AvlTreeSanityCheck(tree);

    while(scanf("%s", buf) == 1)
    {
	AvlTreeSanityCheck(tree);
	printf("lookup %s = ", key.s=buf);
	if(AvlTreeLookup(tree, (foint)(key.s), &data))
	    printf("%d\n", data.i);
	else
	  puts("nope");
	++lines;
	AvlTreeSanityCheck(tree);
    }
    assert(false == AvlTreeLookup(tree, (foint)(key.s="foo"), &data)); AvlTreeSanityCheck(tree);
    assert(false == AvlTreeLookup(tree, (foint)(key.s="bar"), &data)); AvlTreeSanityCheck(tree);
    assert(false == AvlTreeLookup(tree, (foint)(key.s="foobar"), &data)); AvlTreeSanityCheck(tree);

	AvlTreeTraverse((foint)0, copy, traverse); // should delete entire "copy" tree
	assert(copy->n == 0);

    while(lines>0) {
	AvlTreeSanityCheck(tree);
	--lines;
	printf("Deleting <%s>; n=%d... ", bufs[lines], tree->n);
	Boolean exists = AvlTreeLookup(tree, (foint)(key.s=bufs[lines]), &data);
	assert(exists == AvlTreeDelete(tree, (foint)(key.s=bufs[lines])));
	assert(AvlTreeLookup(copy, (foint)(key.s=bufs[lines]), &data) == false);
	printf("Deleted! n=%d\n", tree->n);
	AvlTreeSanityCheck(tree);
    }
    assert(lines == 0 && tree->n == 0);
    AvlTreeSanityCheck(tree);
    AvlTreeFree(tree);
    return 0;
}
}
