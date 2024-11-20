#include <stdio.h>
#include "misc.h"
#include "rand48.h"
#include "graph.h"
#include "sets.h"

/************************** Community routines *******************/
typedef struct _community {
    int id, n;
    SET *nodes;
    GRAPH *G; // the graph we came from
} COMMUNITY;

COMMUNITY *CommunityAlloc(GRAPH *G) {
    COMMUNITY *C = Calloc(sizeof(COMMUNITY), 1);
    C->id = 0; // not sure if this will be handy or not...
    C->n = 0;
    C->G = G;
    C->nodes = SetAlloc(G->n);
    return C;
}

COMMUNITY *CommunityAddNode(COMMUNITY *C, int i) {
    assert(!SetIn(C->nodes, i));
    SetAdd(C->nodes, i);
    C->n++;
    return C;
}

COMMUNITY *CommunityDelNode(COMMUNITY *C, int i) {
    assert(SetIn(C->nodes, i));
    SetDelete(C->nodes, i);
    C->n--;
    return C;
}

int CommunityEdgesIn(COMMUNITY *C) {
    int *memberList = Calloc(C->n, sizeof(int));
    int i, j, n = SetToArray(memberList, C->nodes);
    assert(n == C->n);
    int numEdges=0;
    printf("CommunityEdgesIn: checking community with %d nodes: ", n);
    for(i=0; i<n; i++) {
	//printf("member %d=%d\n", i, memberList[i]);
	for(j=i+1; j<n; j++) {
	    int u = memberList[i], v = memberList[j];
	    assert(u < C->G->n && v < C->G->n);
	    if(GraphAreConnected(C->G,u,v)) ++numEdges;
	}
    }
    Free(memberList);
    printf("numEdges %d\n", numEdges);
    return numEdges;
}

/******************** Sets of Communities (partition) ***********/
typedef struct _communitySet {
    unsigned n; // current number of commnuties
    GRAPH *G; // the graph we came from
    COMMUNITY **C; // array of pointers to COMMUNITY
} PARTITION;

PARTITION *PartitionAlloc(GRAPH *G) {
    PARTITION *P = Calloc(sizeof(PARTITION), 1);
    P->n = G->n;
    P->C = Calloc(sizeof(COMMUNITY**), G->n);
    int i;
    for(i=0; i<P->n; i++) P->C[i] = CommunityAlloc(G);
    return P;
}

double ScorePartition(PARTITION *P) {
    return 0;
}

int main(int argc, char *argv[])
{
    int i, j;
    srand48(time(0));
#if 0
    for(i=0;i<10;i++) {
	double r = drand48();
	printf("%g\n", r);
    }
#endif

    Boolean sparse=false, supportNames = true;
    FILE *fp = Fopen(argv[1], "r"); // edge list file is given on the command line
    GRAPH *G = GraphReadEdgeList(fp, sparse, supportNames, false);
    printf("G has %d nodes\n", G->n);

    PARTITION *P = PartitionAlloc(G);

    int numCommunities = 10; // communities numbered 0 through numCommunities-1 inclusive
    printf("Creating %d communities\n", numCommunities);

    P->n = numCommunities;
    for(i=0; i<G->n; i++) {
	int which = (int)(drand48() * numCommunities);
	CommunityAddNode(P->C[which], i);
	// printf("%d->%d, ", i, which);
    }
    //puts("");

    for(i=0; i<numCommunities; i++) CommunityEdgesIn(P->C[i]);
    puts("");
    double s = ScorePartition(P);
    printf("Partition score is %g\n", s);

    return 0;
}
