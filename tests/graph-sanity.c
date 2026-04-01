// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#include <stdio.h>
#include <math.h>
#include "misc.h"
#include "graph.h"
#include "sets.h"

static double test_weight(unsigned int u, unsigned int v)
{
    return 1000.0 + (double)(u + v);
}
static int IntCmp(const void *a, const void *b)
{
    const int *i = (const int*)a, *j = (const int*)b;
    return (*i)-(*j);
}
int main(int argc, char *argv[])
{
    printf("graph-sanity.c sanity tests running ...\n");
    //ENABLE_MEM_DEBUG();
    int BFSsize, i, j;
    Boolean supportNames = false,directed=false;
    GRAPH *G = GraphReadEdgeList(NULL, stdin, directed, supportNames,false);
    fprintf(stderr, "creating Gbar..."); 
    GRAPH *Gbar = GraphComplement(G);
    fprintf(stderr, "done! creating GG..."); 
    GRAPH *GG = GraphComplement(Gbar);
    fprintf(stderr, "DONE!\nChecking sanity of Complement(Complement(G))...\n");
    assert(GG->n == G->n);
    assert(G->n == Gbar->n);
    assert(G->selfAllowed == Gbar->selfAllowed);
    assert(G->directed == Gbar->directed);
    assert(GG->selfAllowed == G->selfAllowed);
    assert(GG->directed == G->directed);
    for(i=0; i<G->n; i++)
    {
        //printf("%d %d %d %d %d\n",i,Gbar->degree[i],G->n,G->degree[i],Gbar->directed,G->directed);
        assert(Gbar->degree[i]==(G->n)-(G->degree[i]));
        assert(GG->degree[i] == G->degree[i]);
	    qsort(G->neighbor[i], G->degree[i], sizeof(G->neighbor[i][0]), IntCmp);
        qsort(GG->neighbor[i], GG->degree[i], sizeof(GG->neighbor[i][0]), IntCmp);
        for(j=0; j<G->degree[i]; j++)
            assert(GG->neighbor[i][j] == G->neighbor[i][j]);
    }
    fprintf(stderr, "passed!\n");
    fprintf(stderr, "Testing random connect/disconnect...");

    for(int k=2*G->n; k>=0; k--)
    {
	i = lrand48() % G->n;
	j = lrand48() % G->n;
	if(!G->selfAllowed) while(j==i) j = lrand48() % G->n;
	GraphConnect(G,i,j);
	GraphDisconnect(Gbar,i,j);
    }
    fprintf(stderr, "Connected random edges done, checking sanity of GraphUnion\n");
    GRAPH *GU = GraphUnion(Gbar, G);
    for(int i=0; i<GU->n; i++)
        assert(GU->degree[i] == G->n);
    fprintf(stderr, "GraphUnion passed! Now count connected components via BFS:\n");
    GraphFree(Gbar);
    int root, distance, nodeArray[GG->n], distArray[GG->n], CC=0;
    Boolean touched[GG->n];
    for(i=0; i<GG->n; i++) touched[i] = false;

    for(root=0; root < GG->n; root++)
    {
	if(!touched[root])
	{
	    BFSsize = GraphBFS(GG, root, GG->n, nodeArray, distArray);
	    printf (" %d", BFSsize);
	    for(i=0; i<BFSsize; i++)
	    {
		touched[nodeArray[i]] = true;
		//printf("%d(%d) ", nodeArray[i], distArray[nodeArray[i]]);
	    }
	    ++CC;
	}
    }
    fprintf(stderr, "\nGraph has %d connected components using BFS\n", CC);
    GraphFree(GG);

    SET *visited = SetAlloc(G->n);
    BFSsize=CC=0;
    for(i=0; i<G->n;i++) {
	if(!SetIn(visited,i)) ++CC;
	GraphVisitCC(G, i, visited, nodeArray, &BFSsize);
    }
    fprintf(stderr, "Graph has %d connected components using GraphVisitCC\n", CC);
    GraphFree(G);
    SetFree(visited);

    // Test callback weight function pointer
    GRAPH *callbackGraph = GraphAlloc(NULL, 3, false, false, test_weight);
    if (!callbackGraph) {
        fprintf(stderr, "Error: GraphAlloc returned NULL for callbackGraph\n");
        return 1;
    }
    GraphMakeWeighted(callbackGraph);
    double fallbackWeight = 42.0;
    GraphSetWeight(callbackGraph, 0, 1, fallbackWeight);
    fprintf(stderr, "Set fallback weight %.2f on edge (0,1)\n", fallbackWeight);

    double callbackWeightAB = GraphGetWeight(callbackGraph, 0, 1);
    double callbackWeightBA = GraphGetWeight(callbackGraph, 1, 0);
    if (!isfinite(callbackWeightAB) || !isfinite(callbackWeightBA)) {
        fprintf(stderr, "Error: Non-finite weight returned by GraphGetWeight\n");
        GraphFree(callbackGraph);
        return 1;
    }
    printf("Callback weight (0,1): %.2f\n", callbackWeightAB);
    printf("Callback weight (1,0): %.2f\n", callbackWeightBA);
    assert(callbackWeightAB == test_weight(0, 1));
    assert(callbackWeightBA == test_weight(1, 0));
    assert(callbackWeightAB != fallbackWeight);

    printf("Callback weight test passed!\n");
    GraphFree(callbackGraph);

    return 0;
}
