#include <stdio.h>
#include "misc.h"
#include "graph.h"
#include "sets.h"

int main(int argc, char *argv[])
{
    int BFSsize, i, j;
    Boolean sparse=false, supportNames = true;
    GRAPH *G = GraphReadEdgeList(stdin, sparse, supportNames);
    GRAPH *Gbar = GraphComplement(NULL, G);
    GRAPH *GG = GraphComplement(NULL, Gbar);
    printf("Checking sanity of Complement(Complement(G))...");
    assert(GG->n == G->n);
    for(i=0; i<G->n; i++)
    {
	assert(GG->degree[i] == G->degree[i]);
	if(!G->sparse) assert(SetEq(GG->A[i], G->A[i]));
	else for(j=0;j<G->n; j++)
	    assert(GG->neighbor[i][j] == G->neighbor[i][j]);
    }
    puts("passed!");
    printf("Now count connected components via BFS:");

    int root, distance, nodeArray[GG->n], distArray[GG->n], CC=0;
    GG = GraphCopy(GG, G);
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

    SET *visited = SetAlloc(G->n);
    BFSsize=CC=0;
    for(i=0; i<G->n;i++) {
	if(!SetIn(visited,i)) ++CC;
	GraphVisitCC(G, i, visited, nodeArray, &BFSsize);
    }
    printf("Graph has %d connected components using GraphVisitCC\n", CC);

    return 0;
}
