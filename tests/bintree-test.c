// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#include "misc.h"
#include "bintree.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define lineLen 40
#define MAX_LINES (1000*BUFSIZ) // capacity for big inputs when comparing bintree vs avltree sizes

int main(int argc, char *argv[])
{
    assert(argc == 2);
{
    FILE *fp = fopen(argv[1], "r");
    BINTREE *tree = BinTreeAlloc((pCmpFcn)strcmp, (pFointCopyFcn)strdup, (pFointFreeFcn)free, NULL, NULL);
    // static, not local: MAX_LINES*lineLen is 327MB on Linux (BUFSIZ=8192), which overflows the stack. As a static
    // array only the pages actually used get allocated, so a small input still uses little memory.
    char buf[lineLen];
    static char bufs[MAX_LINES][lineLen];
    foint key, data;
    int lines=0;

    while(lines < MAX_LINES && fscanf(fp, "%39s %d", bufs[lines], &data.i) == 2) {
	BinTreeInsert(tree, (foint)(key.s=bufs[lines++]), data);
	BinTreeSanityCheck(tree);
    }
    fclose(fp);
    BinTreeSanityCheck(tree);
    BinTreeRebalance(tree, true);
    BinTreeSanityCheck(tree);

    while(scanf("%39s", buf) == 1)
    {
	BinTreeSanityCheck(tree);
	printf("lookup %s = ", key.s=buf);
	if(BinTreeLookup(tree, (foint)(key.s), &data))
	    printf("%d\n", data.i);
	else
	  puts("nope");
	BinTreeSanityCheck(tree);
    }
    assert(false == BinTreeLookup(tree, (foint)(key.s="foo"), &data)); BinTreeSanityCheck(tree);
    assert(false == BinTreeLookup(tree, (foint)(key.s="bar"), &data)); BinTreeSanityCheck(tree);
    assert(false == BinTreeLookup(tree, (foint)(key.s="foobar"), &data)); BinTreeSanityCheck(tree);

    while(lines>0) {
	BinTreeSanityCheck(tree);
	--lines;
	printf("Deleting <%s>; n=%d, phys=%d... ", bufs[lines], tree->n, tree->physical_n);
	Boolean exists = BinTreeLookup(tree, (foint)(key.s=bufs[lines]), &data);
	assert(exists == BinTreeDelete(tree, (foint)(key.s=bufs[lines])));
	printf("Deleted! n=%d, phys=%d\n", tree->n, tree->physical_n);
	BinTreeSanityCheck(tree);
    }
    assert(lines == 0 && tree->n == 0 && tree->physical_n >= 0);
    BinTreeSanityCheck(tree);
    BinTreeFree(tree);
    return 0;
}
}
