#include <stdio.h>
#include "misc.h"
#include "graph.h"
#include "sets.h"

int main(int argc, char *argv[])
{
    int i,j;
    Boolean sparse=true, supportNames = false, weighted=true;
    GRAPH *G = GraphReadEdgeList(stdin, sparse, supportNames, weighted);
    for(i=0; i<G->n; i++) for(j=i+1;j<G->n;j++) if(GraphAreConnected(G,i,j)) printf("%d %d %f\n",i,j,GraphGetWeight(G,i,j));
    GraphFree(G);
    return 0;
}
