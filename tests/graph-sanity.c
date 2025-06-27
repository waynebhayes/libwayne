#include <stdio.h>
#include "misc.h"
#include "graph.h"
#include "sets.h"

int main(int argc, char *argv[])
{
    //ENABLE_MEM_DEBUG();
    int BFSsize, i, j;
    Boolean self=false, supportNames = true;
    GRAPH *G = GraphReadEdgeList(stdin, self, false, false);
    GRAPH *Gbar = GraphComplement(G);
    GRAPH *GG = GraphComplement(Gbar);
    GraphFree(Gbar);
    printf("Checking sanity of Complement(Complement(G))...");
    assert(GG->n == G->n);
    for(i=0; i<G->n; i++)
    {
	assert(GraphDegree(GG,i) == GraphDegree(G,i));
	assert(SetEq(GG->A[i], G->A[i]));
    }
    puts("passed!");
    printf("Now count connected components via BFS:");

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
    printf("\nGraph has %d connected components using BFS\n", CC);
    GraphFree(GG);

    SET *visited = SetAlloc(G->n);
    BFSsize=CC=0;
    for(i=0; i<G->n;i++) {
	if(!SetIn(visited,i)) ++CC;
	GraphVisitCC(G, i, visited, nodeArray, &BFSsize);
    }
    printf("Graph has %d connected components using GraphVisitCC\n", CC);
    GraphFree(G);
    SetFree(visited);

    return 0;
}
