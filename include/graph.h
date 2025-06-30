#ifdef __cplusplus
extern "C" {
#endif
#ifndef _GRAPH_H
#define _GRAPH_H

#include "misc.h"
#include "sets.h"
#include "combin.h"
#include <stdio.h>
#include "bintree.h" // to support node names

typedef struct _Graph {
    /* vertices numbered 0..n-1 inclusive */
    unsigned n, m, numSelf;
    SET **A; /* Adjacency "Matrix", as a dynamically allocated array[G->n] of SETs */
    Boolean useComplement; // when true, calls to GraphAreConnected are inverted
    Boolean self; // self-loops allowed iff this is true
    Boolean directed; // if undirected, each off-diagonal edge will appear twice, as is conventional.
    float *weight; // weights of the edges in the edgeList, or NULL pointer if unweighted
    BINTREE *nameDict;	// string to int map
    char **name;	// int to string map (inverse of the above)
} GRAPH;

GRAPH *GraphAlloc(unsigned n, Boolean self, Boolean directed, Boolean weighted);
GRAPH *GraphAssignNames(GRAPH *G, BINTREE *nameDict); // (re)-assign the (string names <-> nodes) mapping
void GraphFree(GRAPH *G);
GRAPH *GraphSort(GRAPH *G); // optimize graph for efficiency (happens automatically but this forces it)
GRAPH *GraphEdgesAllDelete(GRAPH *G);
double GraphSetWeight(GRAPH *G, unsigned i, unsigned j, float w);
double GraphGetWeight(GRAPH *G, unsigned i, unsigned j);
//unsigned GraphNumCommonNeighbors(GRAPH *G, unsigned i, unsigned j); // can include pair(i,j) only if self-loops exist
GRAPH *GraphComplement(GRAPH *G);
GRAPH *GraphUnion(GRAPH *G1, GRAPH *G2);
#define GraphDegree(G,v) ((G)->useComplement ? (G)->n-SetCardinality((G)->A[v]) : SetCardinality((G)->A[v]))
#define GraphNumEdges(G) ((G)->useComplement ? ((((G)->n)*((G)->n-1))/2 - (G)->m) : (G)->m)
GRAPH *GraphCopy(GRAPH *G); // (deep) copy a graph
GRAPH *GraphConnect(GRAPH *G, int i, int j);
GRAPH *GraphDisconnect(GRAPH *G, int i, int j);

int GraphNeighbor(GRAPH *G, int u, int n); // return the nth neighbor of u, where 0<=n<=Degree(u)
int GraphRandomNeighbor(GRAPH *G, int u); // A return a neighbor of u chosen uniformly at random

// buf must be a pointer to a pre-allocated integer. When called with *buf=0, return u's first neighbor. 
// Otherwise return next neighbor (caller should not modify *buf except to reset by setting *buf to 0).
int GraphNextNeighbor(GRAPH *G, int u, int *buf); // It's done when *buf==GraphDegree(u) (and it returns 0 then)

// A return a random edge (u,v) written into the pointers. Also return the unique ID of that edge which is for INTERNAL
// USE ONLY. (It will have 0 <= edge < 2*G->m, and it is only to be used when passing back into the same function.)
// If *u is -1, then *v is a specific edge ID as returned previously by a call to this function, or any value <2*G->m.
SET_ELEMENT_TYPE GraphRandomEdge(GRAPH *G, int *u, int *v);

/* Returns number of nodes in the the distance-d neighborhood, including seed.
 * nodeArray[0] always= seed, distArray[seed] always= 0.
 * distance = 0 implies no BFS, seed as only member of nodeArray, and return value 1.
 * Use distance >= G->n to ensure BFS will go out as far as possible.
 * Both arrays should have at least G->n elements; caller is reponsible for allocating them.
 * For each nodeArray[i], distArray[nodeArray[i]] is the distance from seed.
 * (Note that distArray[j] has an entry for EVERY node in the graph; only those
 * with indices j \in nodeArray have meaning.)
 */
int GraphBFS(GRAPH *G, int seed, int distance, int *nodeArray, int *distArray);

/* Uses DFS on G starting at node v to see if the current connected component has at least k nodes.
** More efficient than a full DFS. */
Boolean GraphCCatLeastK(GRAPH *G, int v, int k);

/* Full DFS on whatever connected component v is in.  On top-level call, you should set (*pn)=0.
** All pointers (G, visited, *Varray) must be *allocated*.
** We will populate Varray with the elements and also set their visited state to true.
** The Varray will be nuked starting at position (*pn), and visited does *not* need to be clear,
** so you can call this function multiple times for each connected component and in the end all
** the visited values should be true, and Varray will have all the elements, ordered by which
** connected component they are in.  We return *pn at the end.
*/
int GraphVisitCC(GRAPH *G, unsigned int v, SET *visited, unsigned int *Varray, int *pn);


/*
** GraphInduced_NoVertexDelete doesn't delete any vertices, it only deletes
** edges whose ends don't both appear in V.  GraphInduced builds an entirely
** new graph in which also the vertices not in V are deleted; vertices
** are renumbered but ordering is conserved.
*/
GRAPH *GraphInduced_NoVertexDelete(GRAPH *G, SET *V);
GRAPH *GraphInduced(GRAPH *G, SET *V);

void GraphPrintAdjMatrix(FILE *fp, GRAPH *G);
void GraphPrintAdjList(FILE *fp, GRAPH *G);

GRAPH *GraphReadEdgeList(FILE *fp, Boolean self, Boolean directed, Boolean weighted);
int GraphNodeName2Int(GRAPH *G, char *name);
void GraphPrintConnections(FILE *fp, GRAPH *G);
GRAPH *GraphReadConnections(FILE *fp, Boolean sparse);
#define GraphAreConnected(G,i,j) SetIn(G->A[i],(j))

/*
** The following subroutines should be used with caution, because they take
** exponential time.  They look for cliques and independent sets of size n,
** respectively.  They return 0 if the requested property doesn't exist, or
** else a SET containing the nodes with the property requested.  Calling
** the appropriate GraphNext* function returns the next in the list of
** Kn's or In's. 0 is returned when none are left.
** The underlying graph G should NOT be changed while these routines are
** "in motion".
*/
typedef struct _clique {
    GRAPH *G;
    SET *set;
    unsigned cliqueSize, *combArray, *inducedArray;
    COMBIN* combin;
} CLIQUE;
CLIQUE *GraphKnFirst(GRAPH *G, int n);
Boolean GraphKnContains(GRAPH *G, int n);
SET *GraphKnNext(CLIQUE*);
CLIQUE *GraphInFirst(GRAPH *G, int n);
Boolean GraphInContains(GRAPH *G, int n);
#define GraphInNext GraphKnNext
void GraphCliqueFree(CLIQUE *);

/*
** Isomorphism algorithm for general graphs G1, G2.
** This algorithm uses some simple tests in an attempt to avoid the
** exponential algorithm.  It checks the number of nodes, the number of
** times each degree appears, and then kranks up the exponential time
** algorithm.
*/
Boolean GraphsIsomorphic(int *perm, GRAPH *G1, GRAPH *G2);

#endif /* _GRAPH_H */
#ifdef __cplusplus
} // end extern "C"
#endif
