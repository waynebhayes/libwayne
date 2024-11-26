#include <stdio.h>
#include "misc.h"
#include "rand48.h"
#include "graph.h"
#include "sets.h"

/************************** Community routines *******************/
typedef struct _community {
    int id, n;
    SET *nodeSet;
    GRAPH *G; // the graph we came from
} COMMUNITY;

COMMUNITY *CommunityAlloc(GRAPH *G) {
    COMMUNITY *C = Calloc(sizeof(COMMUNITY), 1);
    C->id = 0; // not sure if this will be handy or not...
    C->n = 0;
    C->G = G;
    C->nodeSet = SetAlloc(G->n);
    return C;
}

void CommunityFree(COMMUNITY *C) {
    SetFree(C->nodeSet);
    Free(C);
}

// It's not an error to add a member multiple times
COMMUNITY *CommunityAddNode(COMMUNITY *C, int i) {
    if(!SetIn(C->nodeSet, i)) {
	SetAdd(C->nodeSet, i);
	C->n++;
	assert(C->n <= C->G->n);
	assert(SetCardinality(C->nodeSet) == C->n);
    }
    return C;
}

// It's not an error to delete a node that's already not there
COMMUNITY *CommunityDelNode(COMMUNITY *C, int i) {
    if(SetIn(C->nodeSet, i)) {
	assert(C->n > 0);
	SetDelete(C->nodeSet, i);
	C->n--;
    }
    return C;
}

int CommunityEdgeCount(COMMUNITY *C) {
    int *memberList = Calloc(C->n, sizeof(int));
    int i, j, n = SetToArray(memberList, C->nodeSet);
    assert(n == C->n);
    int numEdges=0;
    //printf("CommunityEdgeCount: checking community with %d nodes: ", n);
    for(i=0; i<n; i++) {
	//printf("member %d=%d\n", i, memberList[i]);
	for(j=i+1; j<n; j++) {
	    int u = memberList[i], v = memberList[j];
	    assert(u < C->G->n && v < C->G->n);
	    if(GraphAreConnected(C->G,u,v)) ++numEdges;
	}
    }
    Free(memberList);
    //printf("numEdges %d\n", numEdges);
    return numEdges;
}

/******************** Sets of non-overlapping Communities (partition) ***********/
typedef struct _communitySet {
    unsigned n; // current number of non-empty communities
    GRAPH *G; // the graph we came from
    COMMUNITY **C; // array of pointers to COMMUNITY
} PARTITION;

// Returns an empty partition containing no communities (they're not even allocated)
PARTITION *PartitionAlloc(GRAPH *G) {
    PARTITION *P = Calloc(sizeof(PARTITION), 1);
    P->G=G;
    P->C = Calloc(sizeof(COMMUNITY**), G->n);
    return P;
}
PARTITION *PartitionAddCommunity(PARTITION *P, COMMUNITY *C) {
    assert(P->n < P->G->n);
    int i;
    for(i=0; i<P->G->n; i++) if(!P->C[i]) {P->C[i] = C; P->n++; return P;} // find empty community slot
    assert(i<P->G->n);
    return NULL;
}
void PartitionFree(PARTITION *P) {
    int i;
    for(i=0; i<P->n; i++) if(P->C[i]) CommunityFree(P->C[i]);
    Free(P->C);
    Free(P);
}

double ScorePartitionHayes(PARTITION *P) {
    int i;
    double score=0;
    for(i=0; i<P->n; i++) if(P->C[i]) {
	int n = P->C[i]->n;
	if(n>1) {
	    int m = CommunityEdgeCount(P->C[i]);
	    score += m*m/(n*(n-1)/2.0);
	}
    }
    return score;
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
    printf("Creating %d random communities: ", numCommunities);
    for(i=0; i<numCommunities; i++) PartitionAddCommunity(P, CommunityAlloc(G));

    for(i=0; i<G->n; i++) {
	int which = (int)(drand48() * numCommunities);
	CommunityAddNode(P->C[which], i);
	//printf("%d->%d, ", i, which);
    }

    double s = ScorePartitionHayes(P);
    printf("score = %g\n", s);
    PartitionFree(P);

    printf("BFS-based communities: ");
    P = PartitionAlloc(G);
    SET *nodesUsed=SetAlloc(G->n); // cumulative set of nodes that have been put into a partition

    int nodeArray[G->n], distArray[G->n];
    while(SetCardinality(nodesUsed) < G->n) {
	int seed;
	do { seed = (int)(drand48() * G->n); }
	    while(SetIn(nodesUsed, seed));
	int numAdded=0, distance = 4; // should be far enough
	int n=GraphBFS(G, seed, distance, nodeArray, distArray); // list of nodes within "distance" of seed
	//printf("BFS(%d[%d])=%d", seed, G->degree[seed], n);
	assert(n>0 && nodeArray[0]==seed && distArray[seed]==0);
	COMMUNITY *C = CommunityAlloc(G);
	for(i=0; i<n; i++) if(!SetIn(nodesUsed,nodeArray[i]) || drand48() > 0.5) {
	    SetAdd(nodesUsed,nodeArray[i]); CommunityAddNode(C,nodeArray[i]); ++numAdded;
	}
	printf("%d ", numAdded); fflush(stdout);
	assert(C->n >0 && C->n < G->n && C->n==numAdded);
	PartitionAddCommunity(P, C);
    }
    printf("\n%d communities, score = %g\n", P->n, ScorePartitionHayes(P));
    PartitionFree(P);

    return 0;
}
