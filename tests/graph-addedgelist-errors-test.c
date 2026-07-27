// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
// Demonstrates GraphAddEdgeList's input-validation Fatal() paths. Each case below is expected to
// print a "Fatal error: ..." message and exit(1); anything else (a clean return, or a crash with
// no message) indicates a regression. Must be run with this "tests" directory as the current
// working directory, since it opens its *.in files by relative name.
//   ./graph-addedgelist-errors-test 1   -- line 1 looks like a header but isn't a valid integer
//   ./graph-addedgelist-errors-test 2   -- an edge names a node id beyond the declared header count
#include <stdio.h>
#include <stdlib.h>
#include "misc.h"
#include "graph.h"

int main(int argc, char *argv[])
{
    if(argc != 2 || (argv[1][0]!='1' && argv[1][0]!='2'))
	Fatal("usage: %s <1|2>", argv[0]);
    const char *fname = (argv[1][0]=='1') ? "graph-addedgelist-badheader.in" : "graph-addedgelist-oversize.in";
    FILE *fp = fopen(fname, "r");
    if(!fp) Fatal("graph-addedgelist-errors-test: can't open input file '%s' (run this from inside the tests/ directory)", fname);
    GRAPH *G = GraphAddEdgeList(NULL, fp, false, false, false);
    printf("UNEXPECTED SUCCESS: should have hit Fatal() before this point (n=%u)\n", G->n);
    return 0;
}
