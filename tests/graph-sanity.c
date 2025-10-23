// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#include <stdio.h>
#include "misc.h"
#include "graph.h"
#include "sets.h"

int main(int argc, char *argv[])
{
    //ENABLE_MEM_DEBUG();
    int BFSsize, i, j;
    Boolean sparse=false, supportNames = true;
    GRAPH *G = GraphReadEdgeList(stdin, sparse, supportNames,false);
    GRAPH *Gbar = GraphComplement(G);
    GRAPH *GG = GraphComplement(Gbar);
    GraphFree(Gbar);
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
