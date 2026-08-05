// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
// Sanity tests for GraphAddEdgeList (the two-pass, exact-preallocation edge-list reader added to src/graph.c).
// Must be run with this "tests" directory as the current working directory, since it opens its
// *.in files by relative name. See the header comment in graph-addedgelist-errors-test.c for the
// companion tests that exercise GraphAddEdgeList's Fatal() input-validation paths.
#include <stdio.h>
#include <string.h>
#include "misc.h"
#include "graph.h"

static FILE *open_or_die(const char *name)
{
    FILE *fp = fopen(name, "r");
    if(!fp) Fatal("graph-addedgelist-test: can't open input file '%s' (run this from inside the tests/ directory)", name);
    return fp;
}

// Looks up the weight of edge i->j directly in i's own neighbor/weight arrays. Unlike
// GraphGetWeight, this does not assume the reverse edge j->i also exists, so it is safe
// to use on directed graphs.
static double out_weight(GRAPH *G, unsigned i, unsigned j)
{
    unsigned k;
    for(k=0; k<G->degree[i]; k++) if(G->neighbor[i][k]==j) return G->weight[i][k];
    Fatal("out_weight: no edge %u->%u found", i, j);
    return -1;
}

int main(void)
{
    FILE *fp;
    GRAPH *G;

    // Test 1: plain undirected int edge list, no header. Edges: 0-1, 1-2, 2-0, 0-3
    fp = open_or_die("graph-addedgelist-test1.in");
    G = GraphAddEdgeList(NULL, fp, false, false, false);
    fclose(fp);
    assert(G->n==4 && G->numEdges==4);
    assert(G->degree[0]==3 && G->degree[1]==2 && G->degree[2]==2 && G->degree[3]==1);
    assert(GraphAreConnected(G,0,1) && GraphAreConnected(G,1,2) && GraphAreConnected(G,2,0) && GraphAreConnected(G,0,3));
    printf("test1 (plain undirected, no header) PASSED\n");

    // Test 2: same graph, but a header declares n=5 (one isolated extra node) and m=4
    fp = open_or_die("graph-addedgelist-test2.in");
    G = GraphAddEdgeList(NULL, fp, false, false, false);
    fclose(fp);
    assert(G->n==5 && G->numEdges==4 && G->degree[4]==0);
    printf("test2 (header n=5, m=4) PASSED\n");

    // Test 3: node names, weighted, directed. alice->bob(2.5), bob->carol(1.0), alice->carol(3.0)
    fp = open_or_die("graph-addedgelist-test3.in");
    G = GraphAddEdgeList(NULL, fp, true, true, true);
    fclose(fp);
    assert(G->n==3 && G->numEdges==3);
    unsigned alice = GraphNodeName2Int(G,"alice"), bob = GraphNodeName2Int(G,"bob"), carol = GraphNodeName2Int(G,"carol");
    assert(G->degree[alice]==2 && G->degree[bob]==1 && G->degree[carol]==0);
    assert(out_weight(G,alice,bob)==2.5 && out_weight(G,bob,carol)==1.0 && out_weight(G,alice,carol)==3.0);
    printf("test3 (names + weighted + directed) PASSED\n");

    // Test 4: undirected, with a self-loop and duplicate edges, which GraphAddEdgeList must
    // handle exactly like GraphConnect does (self-loop allowed once seen; duplicates skipped).
    fp = open_or_die("graph-addedgelist-test4.in");
    G = GraphAddEdgeList(NULL, fp, false, false, false);
    fclose(fp);
    assert(G->n==2 && G->numEdges==2 && G->selfAllowed);
    assert(G->degree[0]==2 && G->degree[1]==1); // node 0: self-loop + edge to 1; the repeated "0 1" and "1 0" lines are duplicates and are skipped
    printf("test4 (self-loop + duplicate edges) PASSED\n");

    // Test 5: header gives only n (no edge-count line). Edges: 0-1, 1-2
    fp = open_or_die("graph-addedgelist-test5.in");
    G = GraphAddEdgeList(NULL, fp, false, false, false);
    fclose(fp);
    assert(G->n==3 && G->numEdges==2 && G->degree[0]==1 && G->degree[1]==2 && G->degree[2]==1);
    printf("test5 (header n only, no m) PASSED\n");

    printf("ALL GraphAddEdgeList SANITY TESTS PASSED\n");
    return 0;
}
