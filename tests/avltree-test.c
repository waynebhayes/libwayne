#include "misc.h"
#include "avltree.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define lineLen 40

/* Usage: ./avltree-test avltree-test.in
Enter keys to lookup; EOF to exit this loop */

int main(int argc, char *argv[])
{
    assert(argc == 2);

    FILE *fp = fopen(argv[1], "r");
    AVLTREE *tree = AvlTreeAlloc((pCmpFcn)strcmp, (pFointCopyFcn)strdup, (pFointFreeFcn)free, NULL, NULL);
    static char buf[lineLen], bufs[1000*BUFSIZ][lineLen];
    foint key, data;
    int lines=0;

    while(fscanf(fp, "%s %d", bufs[lines], &data.i) == 2) {
	AvlTreeInsert(tree, (foint)(key.s=bufs[lines++]), data);
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

    while(lines>0) {
	AvlTreeSanityCheck(tree);
	--lines;
	printf("Deleting <%s>; n=%d... ", bufs[lines], tree->n);
	Boolean exists = AvlTreeLookup(tree, (foint)(key.s=bufs[lines]), &data);
	assert(exists == AvlTreeDelete(tree, (foint)(key.s=bufs[lines])));
	printf("Deleted! n=%d\n", tree->n);
	AvlTreeSanityCheck(tree);
    }
    assert(lines == 0 && tree->n == 0);
    AvlTreeSanityCheck(tree);
    AvlTreeFree(tree);
    return 0;
}
