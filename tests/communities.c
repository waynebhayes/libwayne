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
    if(!SetIn(C->nodeSet, i)){
	SetAdd(C->nodeSet, i);
	C->n++;
	assert(C->n <= C->G->n);
	assert(SetCardinality(C->nodeSet) == C->n);
    }
    return C;
}

int NodeInDegree(COMMUNITY *C, int node){
   // Loop through neighbor of node and use the SetIn(C->NodeSet)

   int i, inDeg = 0;
   for(i=0;i<C->G->degree[node];i++){
        if(SetIn(C->nodeSet, C->G->neighbor[node][i])) inDeg++;
   }
   return inDeg;
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


int CommunityEdgeOutwards(COMMUNITY *C){
    // Use total degree - InDegree of each node in the community
    int out = 0;
    GRAPH *G = C->G;
    int *memberList = Calloc(C->n, sizeof(int));
    int i, j, k = SetToArray(memberList, C->nodeSet);
    for(i = 0; i < C->n; i++){
        int node = memberList[i];
        out += G->degree[node] - NodeInDegree(C, node);
    }
    Free(memberList);
    return out;
}

/******************** Sets of non-overlapping Communities (partition) ***********/


typedef struct _communitySet {
    unsigned n; // current number of non-empty communities
    GRAPH *G; // the graph we came from
    COMMUNITY **C; // array of pointers to COMMUNITY
    int *where; // Tells where each node belongs to which community
} PARTITION;

// Returns an empty partition containing no communities (they're not even allocated)
PARTITION *PartitionAlloc(GRAPH *G) {
    PARTITION *P = Calloc(sizeof(PARTITION), 1);
    P->G=G;
    P->C = Calloc(sizeof(COMMUNITY**), G->n);
    P->where = Calloc(sizeof(int), G->n);
    return P;
}
PARTITION *PartitionAddCommunity(PARTITION *P, COMMUNITY *C) {
    assert(P->n < P->G->n);
    int i;

    for(i=0; i<P->G->n; i++)
        if(!P->C[i]) {
            P->C[i] = C;
            P->n++;
            printf("P->n = %d\n", P->n);
            int j, n;
            int *memberList = Calloc(C->n, sizeof(int));
            i, j, n = SetToArray(memberList, C->nodeSet);
            for(j = 0; j < C->n; j++)
                P->where[memberList[j]] = i;
            Free(memberList);

            return P;// find empty community slot
        }
        return NULL;
}

PARTITION *PartitionDelCommunity(PARTITION *P, int c){
    if(P->C[c]){
        int i;
        CommunityFree(P->C[c]);
        P->n--;
        for(i = c; i < P->n; i++){
            P->C[i] = P->C[i+1];
        }
        P->C[i] = NULL;
    }
    return P;
}

void PartitionFree(PARTITION *P) {
    int i;
    for(i=0; i<P->n; i++) if(P->C[i]) {CommunityFree(P->C[i]);}
    Free(P->C);
    Free(P->where);
    Free(P);
}

void MoveRandomNode(PARTITION *P){
    int u = P->G->n * drand48();
    printf("Deleting node %d from com %d\n", u, P->where[u]);
    CommunityDelNode(P->C[P->where[u]], u);
    int newCom;

    do{double r = drand48(); newCom = P->n * r;printf("%f ", r); }
    while(newCom == P->where[u]);
    printf("Adding to newCom %d, %p\n", newCom, P->C[newCom]);
    CommunityAddNode(P->C[newCom], u);
    P->where[u] = newCom;
}

COMMUNITY *MergeCommunities(PARTITION *P, int c1, int c2){
    // Merges C1 and C2 into just C1
    printf("Merge. P->n = %d\nC1 = %d, C2 = %d\n", P->n, c1, c2);
    COMMUNITY *C1 = P->C[c1];
    COMMUNITY *C2 = P->C[c2];
    printf("C1 size = %d, C2 size = %d\nC1\n", C1->n, C2->n);
    SetPrint(C1->nodeSet);
    printf("\nC2\n");
    SetPrint(C2->nodeSet);
    SetUnion(C1->nodeSet, C2->nodeSet, C1->nodeSet);
    int *memberList = Calloc(C2->n, sizeof(int));
    int i, j, n = SetToArray(memberList, C2->nodeSet);

    for(i = 0; i < C2->n; i++)
        P->where[memberList[i]] = c1;

    Free(memberList);
    C1->n = SetCardinality(C1->nodeSet);
    SetEmpty(C2->nodeSet);

    C2->n = 0;
    printf("After merge size = %d\n", C1->n);
    SetPrint(C1->nodeSet);
    PartitionDelCommunity(P, c2);
    return C1;
}

void SplitCommunity(PARTITION *P, int c_id, int num){
    printf("Split. P->n = %d\n", P->n);
    COMMUNITY *oldCom = P->C[c_id];
    if(num < 2 || num > oldCom->n - 1){
        printf("Num = %d, oldCom->n = %d. invalid split size\n", num, oldCom->n);
        return;
    }
    COMMUNITY *newCom = CommunityAlloc(P->G);

    int *memberList = Calloc(sizeof(int), oldCom->n);
    int i, j, n = SetToArray(memberList, oldCom->nodeSet);

    for(i = oldCom->n-1; i > 0; i--){
        j = drand48() * (i+1);
        int temp = memberList[j];
        memberList[j] = memberList[i];
        memberList[i] = temp;
    }
    for(i = 0; i < num; i++){
        int u = memberList[i];
        P->where[u] = P->n;
        CommunityAddNode(newCom, u);
        CommunityDelNode(oldCom, u);
    }
    Free(memberList);
    PartitionAddCommunity(P, newCom);
}


void TestCommunityRoutines(PARTITION *P){
    printf("Calling TestCommunityRoutines\n");
    assert(P->n > 2);
    MergeCommunities(P, 1, 2);
    MergeCommunities(P, 1, 2);
    for(int i = 0; i < 20; i++)
        MoveRandomNode(P);
    printf("\nDone with test\n");
}

/*
double ScorePartitionHayes(PARTITION *P) {
    int i;
    double score=0;
    for(i=0; i<P->n; i++) if(P->C[i]) {
	int n = P->C[i]->n;
	if(n>1) {
	    int m = CommunityEdgeCount(P->C[i]);
            printf("m = %d, n = %d\n", m, n);
	    score += m*m/(n*(n-1)/2.0);
	}
        printf("\nScore for community %d = %g\n", i, score);
    }
    return score;

}
*/

/*

    Measures

*/

double IntraEdgeDensity(COMMUNITY *C, int inEdges){
   return inEdges/(C->n *(C->n - 1) / 2.0);
}

double InterEdgeDensity(COMMUNITY *C, int edgesOut){
    int tot = C->G->n - C->n;
    //printf("tot = %d, edgesOut = %d\n", tot, edgesOut);
    return (double)edgesOut/(C->n * tot);
}

double NewmanAndGirvan(int edgesOut, int edgesIn, int gDegree){
    // Eq 15 on Pg 16 on the pdf viewer
    int inEdges = edgesIn;
    int cDegree = inEdges + edgesOut;
    return inEdges/gDegree - (cDegree/(2.0*gDegree) * cDegree/(2.0*gDegree));
}

double Conductance(int edgesIn, int edgesOut){
     return (double)(edgesOut/(edgesOut + edgesIn));
}

double HayesScore(COMMUNITY *C, int inEdges){
    //printf("inEdges = %d, C->n = %d\n", inEdges, C->n);
    if(C->n < 2)
        return 0;
    return inEdges * inEdges / ((C->n * C->n-1)/2.0);
}


double ScorePartition(PARTITION *P) {
    int i, gDegree = 0;
    double total = 0.0;
    GRAPH *G = P->C[0]->G;
    for(i = 0; i < G->n; i++){
        gDegree += G->degree[i];
    }
    printf("G degree = %d\n", gDegree);
    for(i = 0; i < P->n; i++){
        // Run the expensive calculations once per community
	double score = 0.0;
        COMMUNITY *Com = P->C[i];
        //printf("Size of Community %d = %d\n", i, Com->n);
        if(Com->n > 0){
        int interEdges = CommunityEdgeOutwards(Com);
        int intraEdges = CommunityEdgeCount(Com);
        score += IntraEdgeDensity(Com, intraEdges);
        printf("Score of IaED is %g\n", score);
	double IeEd = InterEdgeDensity(Com, interEdges);
	printf("Score of IeED is %g\n", IeEd);
        score -= IeEd;
        double ng = NewmanAndGirvan(intraEdges, interEdges, gDegree);
        printf("Score of N&G is %g\n", ng);
        score += ng;
        double con = Conductance(intraEdges, interEdges);
        printf("Score of Conductance is %g\n", con);
        score -= con;
        double hayes = HayesScore(Com, intraEdges);
        printf("Hayes score = %g\n", hayes);
        score += hayes;
	printf("\nScore of community %d is %g\n\n", i, score);
        total += score;
        }
    }
    return total;
}

double HillClimbing(PARTITION *P, int tries){
    // Program just one score, print out the others
    // Reject any moves that do not improve the score
    printf("%d\n", P->n);
    int i, j;
    double best = 0, score = 0;
    // Calculate the base score of the current partition
    for(j = 0; j < P->n; j++){
        best += HayesScore(P->C[j], CommunityEdgeCount(P->C[j]));
    }
    printf("Base score = %g, tries = %d\n", best, tries);

    for(i = 0; i < tries; i++){
        for(int l = 0; l < P->n; l++)
            printf("%d, %p, size = %d\n", l, P->C[l], P->C[l]->n);
        score = 0;
        int choice = drand48() * 3;
        printf("Choice = %d\n", choice);

        if(choice == 0 && P->n > 1){
            MoveRandomNode(P);
        }

        if(choice == 1 && P->n > 1){
            int c1, c2;

            do{
                double r1 = drand48();
                double r2 = drand48();
                c1 = (int)(r1 * P->n);
                c2 = (int)(r2 * P->n);

            }
            while(c1 == c2);
            MergeCommunities(P, c1, c2);
        }

        else{
            // Possible cases of communities where n = 2 or 3 and split doesn't make sense to do
            int c, num;
            printf("In split\n");
            c = drand48() * P->n;
            //while(P->C[c]->n < 3);

            printf("C = %d\n", c);
            num = drand48() * P->C[c]->n;
            //while(num < 3);
            printf("Splitting Com %d, with num = %d\n", c, num);
            SplitCommunity(P, c, num);
        }

        for(j = 0; j < P->n; j++){
            score += HayesScore(P->C[j], CommunityEdgeCount(P->C[j]));
        }
        //ScorePartition(P);

        printf("Score = %g\n", score);
/*
        if(score > best){
            best = score;
            printf("\nAccepting new best = %g\n", best);
        }
        else{
            printf("\nRejecting score. best = %g\n", best);
        }
*/

    }

    printf("Final best = %g\n", best);
    return best;
}


int main(int argc, char *argv[])
{

    int i, j;
    srand48(time(0));

    Boolean sparse=maybe, supportNames = true;
    FILE *fp = Fopen(argv[1], "r"); // edge list file is given on the command line
    GRAPH *G = GraphReadEdgeList(fp, sparse, supportNames, false);
    printf("G has %d nodes\n", G->n);

    PARTITION *P = PartitionAlloc(G);

    int numCommunities = 10; // communities numbered 0 through numCommunities-1 inclusive
    printf("Creating %d random communities: \n", numCommunities);
    for(i=0; i<numCommunities; i++) PartitionAddCommunity(P, CommunityAlloc(G));

    for(i=0; i<G->n; i++) {
	int which = (int)(drand48() * numCommunities);
	CommunityAddNode(P->C[which], i);
	//printf("%d->%d, ", i, which);
    }


    //double s = ScorePartitionHayes(P);
    //printf("Hayes score = %g\n", s);
    //double s = ScorePartition(P);
    //printf("\nTotal Partition Score = %g\n", s);




    //PartitionFree(P);

    /*
    printf("BFS-based communities: \n");
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
	//printf("%d ", numAdded); fflush(stdout);
	assert(C->n >0 && C->n < G->n && C->n==numAdded);
	PartitionAddCommunity(P, C);
        //printf("Size of Community = %d\n", C->n);
    }
    //printf("\n%d communities, score = %g\n", P->n, ScorePartition(P));
    */


    //for(i=0; i<numCommunities; i++) CommunityEdgeCount(P->C[i]);
    //puts("");
    //s = ScorePartition(P);
    //printf("Partition score is %g\n", s);
    //for(i = 0; i < P->n; i++)
    //    printf("%d, size = %d\n", i, P->C[i]->n);
    //TestCommunityRoutines(P);


    printf("%d\n", P->n);
    HillClimbing(P, 30);
    printf("Attempting Partition Free\n");
    PartitionFree(P);
    printf("Partition Free completed\n");
    GraphFree(G);
    return 0;
}
