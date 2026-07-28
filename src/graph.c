// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#ifdef __cplusplus
extern "C" {
#endif
#include "misc.h"
#include "sets.h"
#include "graph.h"
#include "queue.h"
#include "rand48.h"
#include "Oalloc.h"
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "mem-debug.h"

#define MIN_EDGELIST 1024

/*************************************************************************
**
**                            The Basics
**
*************************************************************************/

static void GraphFreeInternals(GRAPH *G);

GRAPH *GraphAlloc(GRAPH *G, unsigned int n, Boolean directed, Boolean supportNodeNames, GraphEdgeWeightFn edgeWeightFn)
{
    static Boolean needStartup = 1;

    if(G) GraphFreeInternals(G);
    else G = Calloc(1, sizeof(GRAPH));

    if(needStartup)
    {
	needStartup = 0;
	SetStartup();
    }
    G->directed = directed;
    G->n = n;
    G->A = NULL;
    G->degree = Calloc(n, sizeof(G->degree[0]));
    G->maxEdges = MIN_EDGELIST;
    G->edgeList = Malloc(2*G->maxEdges*sizeof(int));
    G->numEdges = 0;
    G->neighbor = Calloc(n, sizeof(G->neighbor[0]));
#if SORT_NEIGHBORS
    G->sorted = SetAlloc(G->n);
#endif
    G->supportNodeNames = supportNodeNames;
    G->edgeWeightFn = edgeWeightFn;
    return G;
}

GRAPH *GraphSelfAlloc(unsigned int n, Boolean directed, Boolean supportNodeNames, GraphEdgeWeightFn edgeWeightFn)
{
    GRAPH *G = GraphAlloc(NULL, n, directed, supportNodeNames, edgeWeightFn);
    G->selfAllowed=true;
    return G;
}

GRAPH *GraphAllocateNeighborLists(GRAPH *G, unsigned *maxDegree) // YANG
{
    // go through all the nodes and pre-allocate the correct length neighbor lists, and set G->maxDegree[i] for each
    // to be the same as the parameter above maxDegree[i]
    Apology("Sorry, GraphAllocateNeighborLists not yet implemented");
    return (GRAPH*) NULL;
}

GRAPH *GraphMakeWeighted(GRAPH *G)
{
    assert(G);
    assert(!SORT_NEIGHBORS);
    G->weight = Calloc(G->n, sizeof(G->weight[0]));
    return G;
}


// Free everything except the GRAPH * itself
static void GraphFreeInternals(GRAPH *G)
{
    int i;
    for(i=0; i<G->n; i++)
    {
	if(G->neighbor[i])Free(G->neighbor[i]);
	if(G->weight && G->weight[i]) Free(G->weight[i]);
    }
    if(G->degree) Free(G->degree);
    if(G->edgeList) Free(G->edgeList);
    if(G->neighbor) Free(G->neighbor);
    if(G->weight) Free(G->weight);
    if(G->name) {
	for(i=0;i<G->n;i++) Free(G->name[i]);
	Free(G->name);
    }
    if(G->nameDict) TreeFree(G->nameDict);
}

void GraphFree(GRAPH *G) {
    GraphFreeInternals(G);
    Free(G);
}

static void GraphNameWarn(const char *s) {
    static char warned;
    if(!warned) Warning("%s called on graph with names; not copying names", s);
    warned=1;
}

GRAPH *GraphCopy(GRAPH *G)
{
    int i;
    if(G->supportNodeNames) GraphNameWarn("GraphCopy");
    GRAPH *Gc = GraphAlloc(NULL, G->n, G->directed, false, G->edgeWeightFn);
    Gc->degree = Calloc(G->n, sizeof(Gc->degree[0]));
    for(i=0;i<G->n;i++) Gc->degree[i] = G->degree[i];

    Gc->useComplement = G->useComplement;
    Gc->directed = G->directed;
    Gc->edgeList[2*i] = G->edgeList[2*i];
    Gc->edgeList[2*i+1] = G->edgeList[2*i+1];
    Gc->n = G->n;
    return Gc;
}

#if SORT_NEIGHBORS
// Used when qsort'ing the neighbors when graph is sparse.
static int IntCmp(const void *a, const void *b)
{
    const int *i = (const int*)a, *j = (const int*)b;
    return (*i)-(*j);
}

static GRAPH *GraphSort(GRAPH *G)
{
    if(G->weight) Apology("Sorry GraphSort not yet implemented for weighted graphs");
    int v;
    for(v=0; v<G->n; v++) if(!SetIn(G->sorted, v)) 
    {
	qsort(G->neighbor[v], G->degree[v], sizeof(G->degree[0]), IntCmp);
	SetAdd(G->sorted, v);
    }
    return G;
}
#else
#define GraphSort(x)
#endif

GRAPH *GraphConnect(GRAPH *G, unsigned i, unsigned j)
{
    assert(!SORT_NEIGHBORS);
    assert(0 <= i && i < G->n && 0 <= j && j < G->n);
    if(i==j) assert(G->selfAllowed);
    if(GraphAreConnected(G, i, j)) return G;
    // YANG: change this to only realloc if necessary, and just add 1, don't double the size since this should be rare.
    G->neighbor[i] = Realloc(G->neighbor[i], (G->degree[i]+1)*sizeof(G->neighbor[i][0]));
    if(j!=i) G->neighbor[j] = Realloc(G->neighbor[j], (G->degree[j]+1)*sizeof(G->neighbor[j][0]));
    if(G->weight) {
	G->weight[i] = Realloc(G->weight[i], (G->degree[i]+1)*sizeof(G->weight[i][0]));
	if(j!=i) G->weight[j] = Realloc(G->weight[j], (G->degree[j]+1)*sizeof(G->weight[j][0]));
    }
    assert(G->neighbor[i]);
    if(!G->directed) assert(G->neighbor[j]);
    G->neighbor[i][G->degree[i]] = j;
    if(!G->directed)G->neighbor[j][G->degree[j]] = i;
    if(G->weight) { // should we increment? Set to 1 if zero? Leave it the same if nonzero???
	G->weight[i][G->degree[i]] = 1;
	if(!G->directed)G->weight[j][G->degree[j]] = 1;
    }
#if SORT_NEIGHBORS
    SetDelete(G->sorted, i);
    if(!G->directed) SetDelete(G->sorted, j);
#endif
    assert(G->numEdges <= G->maxEdges);
    if(G->numEdges == G->maxEdges)
    {
	G->maxEdges = 2*G->maxEdges-1; // -1 to reduce chance of overflow near 2GB and 4GB.
	G->edgeList = Realloc(G->edgeList, 2*G->maxEdges*sizeof(G->edgeList[0]));
	assert(G->edgeList);
    }
    G->edgeList[2*G->numEdges] = i;
    G->edgeList[2*G->numEdges+1] = j;
    G->numEdges++;
    ++G->degree[i];
    if(j!=i&&!G->directed) ++G->degree[j];
    return G;
}
double GraphSetWeight(GRAPH *G, unsigned i, unsigned j, double w)
{
    assert(w>0);
    GraphConnect(G,i,j); // this will allocate G->weight[i] and G->weight[j] if necessary.
    assert(G->weight);

    int k=0;
    double oldWeight;

    while(G->neighbor[i][k] != j) k++;
    assert(k < G->degree[i] && G->neighbor[i][k] == j);
    oldWeight = G->weight[i][k];
    G->weight[i][k] = w;

    if(j!=i&&!G->directed) {
	k=0;
	while(G->neighbor[j][k] != i) k++;
	assert(k < G->degree[j] && G->neighbor[j][k] == i);
	assert(oldWeight == G->weight[j][k]);
	G->weight[j][k] = w;
    }
    return oldWeight;
}

double GraphGetWeight(GRAPH *G, unsigned i, unsigned j)
{
    if(!GraphAreConnected(G,i,j)) return 0.0;

    if (G->edgeWeightFn) {
        return G->edgeWeightFn(i, j);
    }

    if (!G->weight) return 1.0;

    int k = 0;
    while(G->neighbor[i][k] != j) k++;
    assert(k < G->degree[i] && G->neighbor[i][k] == j);
    double w = G->weight[i][k];
    assert(w>0);

    if (j!=i) {
        k=0;
        while(G->neighbor[j][k] != i) k++;
        assert(k < G->degree[j] && G->neighbor[j][k] == i);
        assert(G->weight[j][k] == w);
    }
    return w;
}

GRAPH *GraphEdgesAllDelete(GRAPH *G)
{
    int i;
    for(i=0; i < G->n; i++)
    {
	G->degree[i] = 0;
#if SORT_NEIGHBORS
	SetDelete(G->sorted, i);
#endif
	/* Don't need to realloc/free neighbors, it'll happen automatically once we start re-adding edges */
    }
    G->numEdges = 0;
    G->maxEdges = MIN_EDGELIST;
    G->edgeList = Realloc(G->edgeList, 2*G->maxEdges*sizeof(G->edgeList[0]));
    return G;
}

GRAPH *GraphDisconnect(GRAPH *G, unsigned i, unsigned j) //only deletes edge from i to j if the graph is directed
{
    int k;
    if(i==j) assert(G->selfAllowed);
    assert(0 <= i && i < G->n && 0 <= j && j < G->n);
    if(!GraphAreConnected(G, i, j))
	return G;
    --G->degree[i];
    if(j!=i&&!G->directed) --G->degree[j];

    Boolean found=false;
    for(k=0; k < G->numEdges; k++)
    {
        if(G->edgeList[2*k] == j && G->edgeList[2*k+1]==i && !G->directed)
        {
            unsigned temp = G->edgeList[2*k];
            G->edgeList[2*k] = G->edgeList[2*k+1];
            G->edgeList[2*k+1] = temp;
        }
	if(G->edgeList[2*k] == i && G->edgeList[2*k+1]==j)
	{
	    G->numEdges--;
	    G->edgeList[2*k] = G->edgeList[2*G->numEdges];
	    G->edgeList[2*k+1] = G->edgeList[2*G->numEdges+1];
	    found=true;
	    break;
	}
    }
    assert(found);

    /* now find and delete each other's neighbors--they MUST exist since we checked above */
    if(!G->directed){
        k=0;
        while(G->neighbor[i][k] != j) k++; // this MUST halt since (i,j) are neighbors (checked above)
        assert(k <= G->degree[i] && G->neighbor[i][k] == j); /* this is the new degree, so using "<=" is correct */
        G->neighbor[i][k] = G->neighbor[i][G->degree[i]];
        if(G->weight) G->weight[i][k] = G->weight[i][G->degree[i]];

        if(j!=i) {
            k=0;
            while(G->neighbor[j][k] != i) k++;
            assert(k <= G->degree[j] && G->neighbor[j][k] == i);
            G->neighbor[j][k] = G->neighbor[j][G->degree[j]];
            if(G->weight) G->weight[j][k] = G->weight[j][G->degree[j]];
        }
    }
#if SORT_NEIGHBORS
    SetDelete(G->sorted, i);
    if(!G->directed) SetDelete(G->sorted, j);
#endif
    return G;
}

static Boolean _rawConnected(GRAPH *G, int i, int j) 
{
#if PARANOID_ASSERTS
    assert(0 <= i && i < G->n && 0 <= j && j < G->n);
#endif
#if SORT_NEIGHBORS
    if(SetIn(G->sorted, i))
	return !!bsearch(&j, G->neighbor[i], G->degree[i], sizeof(G->neighbor[0]), IntCmp);
    else if(j!=i && SetIn(G->sorted, j)&&!G->directed)
	return !!bsearch(&i, G->neighbor[j], G->degree[j], sizeof(G->neighbor[0]), IntCmp);
    else
#endif
    {
	int k, n;
        unsigned *neighbors;
        n = G->degree[i];
        neighbors = G->neighbor[i];
        for(k=0; k<n; k++)
            if(neighbors[k] == j)
                return true;
        return false;
    }
    return false;
}
Boolean GraphAreConnected(GRAPH *G, int i, int j)
{
    if(G->useComplement) return !_rawConnected(G,i,j);
    else		 return  _rawConnected(G,i,j);
}

void GraphRandomEdge(GRAPH *G, int *u, int *v)
{
    if(G->useComplement) {
	do { *u=G->n*drand48(); *v=G->n*drand48(); } while((*u==*v && !G->selfAllowed) || _rawConnected(G,*u,*v));
    } else {
	int e = G->numEdges*drand48();
	*u=G->edgeList[2*e]; *v=G->edgeList[2*e+1];
    }
}

int GraphRandomNeighbor(GRAPH *G, int u)
{
    if(G->useComplement) {
	assert(G->degree[u] < G->n);
	int v;
	do { v = G->n * drand48(); } while((u==v && !G->selfAllowed) || _rawConnected(G,u,v));
	return v;
    } else {
	assert(G->degree[u] > 0);
	return G->neighbor[u][(int)(G->degree[u] * drand48())];
    }
}

int GraphNextNeighbor(GRAPH *G, int u, int *buf)
{
    assert(0 <= *buf && *buf <= G->n);
    if(G->useComplement) {
	while(*buf < G->n && _rawConnected(G,u,*buf)) (*buf)++;
	if(*buf == G->n) return -1;
	else return (*buf)++;
    } else {
	if(*buf == G->degree[u]) return -1;
	else return G->neighbor[u][(*buf)++];
    }
}

// Basic idea: loop through ALL neighbors v of i, increment count if j is also connected to v (including self-loops)
// This works even if self-loops are allowed, because if (u,u) and (u,v) both exist, then u is neighbor to both
unsigned GraphNumCommonNeighbors(GRAPH *G, unsigned i, unsigned j)
{
    assert(0 <= i && i < G->n && 0 <= j && j < G->n);
    int numCommon1 = 0, numCommon2 = 0; // for G', these are actually the number of NON-common
    if(i==j) {
	if(G->useComplement) numCommon1 = G->n - G->degree[i];
	else numCommon1 = G->degree[i]; // it's the same node, so number of common neighbors is all neighbors
    }
    unsigned k, n;
    // ensure i has the shorter list, j has the larger
    if(G->degree[i] > G->degree[j]) { int tmp=j; j=i; i=tmp; }
    n = G->degree[i];
    for(k=0; k<n; k++)
	if(_rawConnected(G, j, G->neighbor[i][k])) ++numCommon1;
    // for G complement, we need to check neighbors of BOTH i and j, and then subtract the sum from G->n
    if(G->useComplement) {
        n = G->degree[j];
        for(k=0; k<n; k++)
            if(_rawConnected(G, i, G->neighbor[j][k])) ++numCommon1;
        // Inverting it is a bit tricky: if self-loops are ALLOWED, then max possible value is G->n, and any
        // self-loops that actually exist above become non-connnected in G', so they SHOULD be subtracted.
        numCommon1 = G->n - numCommon1;
        // However, if self-loops are NOT allowed, then max possible value is (G->n - 2), so bump it down by 2 below
        // if(!G->selfAllowed) numCommon1 -= 2;
    }
    if(numCommon1 && numCommon2) {
	assert(numCommon1 == numCommon2);
	numCommon2=0;
    }
    assert(numCommon1 == 0 || numCommon2 == 0);
    if(G->useComplement) return numCommon1 + numCommon2 - 2*G->selfAllowed;
    else return numCommon1 + numCommon2;
}

#ifndef GraphNumEdges
int GraphNumEdges(GRAPH *G)
{
    assert(!G->directed);
    int total=0, i;
    for(i=0; i<G->n; i++)
	total += G->degree[i];
    assert(total % 2 == 0); // should be divisible by 2
    assert(G->numEdges == total/2);
    return G->numEdges;
}
#endif

void GraphPrintAdjMatrix(FILE *fp, GRAPH *G)
{
    int i, j;
    fprintf(fp, "%d\n", G->n);
    for(i=0; i<G->n; i++)
    {
	fprintf(fp, "%d", !!GraphAreConnected(G,i,0));
	for(j=1; j<G->n; j++)
	    fprintf(fp, "%d ", !!GraphAreConnected(G,i,j));
	fprintf(fp, "\n");
    }
}


GRAPH *GraphReadAdjMatrix(GRAPH *G, FILE *fp, Boolean directed)
{
    int i,j,n;
    if(fscanf(fp, "%d", &n) != 1)
	Fatal("GraphReadAdjMatrix: reading 'n' failed");
    assert(n >= 0);
    G = GraphAlloc(NULL, n, directed, false, NULL); // no SUPPORT_NODE_NAMES at the moment
    for(i=0; i<n; i++) for(j=0; j<n; j++)
    {
	int connected;
	if(fscanf(fp, "%d", &connected) != 1)
	    Fatal("GraphReadAdjMatrix: reading entry(%d,%d) failed", i, j);
	if(connected) {
	    if(i==j) {
		static Boolean warned;
		if(!warned) Warning("GraphReadAdjMatrix: node %d has a self-loop; assuming they are allowed",i);
		warned = G->selfAllowed = true;
	    }
	    GraphConnect(G,i,j);
	}
    }
    GraphSort(G);
    return G;
}


void GraphPrintAdjList(FILE *fp, GRAPH *G)
{
    int i, j;
    fprintf(fp, "%d\n", G->n);
    for(i=0; i<G->n; i++)
    {
	fprintf(fp, "%d ", G->degree[i]);
	for(j=0; j<G->degree[i] - 1; j++)
	    fprintf(fp, "%d ", G->neighbor[i][j]);
	fprintf(fp, "%d\n", G->neighbor[i][j]);
    }
}


GRAPH *GraphReadAdjList(GRAPH *G, FILE *fp, Boolean directed)
{
    int n, i, j, d;
    if(fscanf(fp, "%d", &n) != 1)
	Fatal("GraphReadAdjList: failed to read 'n'");
    assert(n >= 0);
    if(G) assert(n == G->n);
    else G = GraphAlloc(NULL, n, directed, false, NULL); // no SUPPORT_NODE_NAMES at the moment
    for(i=0; i<n; i++)
    {
	if(fscanf(fp, "%d", &d) != 1)
	    Fatal("node %d: expecting degree, but couldn't find an integer", i);
	for(j=0; j<d; j++)
	{
	    int neigh;
	    if(fscanf(fp, "%d", &neigh) != 1)
		Fatal("node %d, degree %d, ran out of integers on neighbor #%d", i, d, j);
	    else
	    {
		assert(0 <= neigh && neigh < n);
		if(neigh == i) {
		    static Boolean warned;
		    if(!warned) Warning("GraphReadAdjList: node %d has a self-loop; assuming they are allowed", i);
		    warned = G->selfAllowed = true;
		}
		GraphConnect(G, i, neigh);
	    }
	}
    }
    GraphSort(G);
    return G;
}

// Parses `tok` (already whitespace-trimmed, e.g. by a prior "%s" scanf conversion) as a
// non-negative integer that fits in an unsigned int, Fatal()ing on a leading '-', non-numeric
// text, or a value too large. strtoul is used instead of sscanf's %d/%u because, unlike those,
// it is required by the standard to report its own overflow (via errno==ERANGE) rather than
// invoking undefined behavior--but since strtoul's own return type (unsigned long) can be wider
// than unsigned int, a value can clear that check yet still be too big for our target, so it also
// gets an explicit bound check against UINT_MAX.
static unsigned ParseNonNegUint(const char *tok, unsigned lineNum)
{
    if(tok[0]=='-')
	Fatal("GraphAddEdgeList: line %u must be a non-negative integer, but is \"%s\"", lineNum, tok);
    char *end;
    errno = 0;
    unsigned long val = strtoul(tok, &end, 10); // 10 = base-10
    if(end==tok || *end != '\0')
	Fatal("GraphAddEdgeList: line %u must be a non-negative integer, but is \"%s\"", lineNum, tok);
    if(errno==ERANGE || val > UINT_MAX)
	Fatal("GraphAddEdgeList: line %u: value \"%s\" is too large to fit in an unsigned int", lineNum, tok);
    return (unsigned)val;
}

// Reads an edge-list file and builds a fresh GRAPH from it (G is passed to GraphAlloc, so
// NULL or an existing GRAPH* to be reinitialized are both fine, same convention as GraphReadEdgeList).
// Unlike GraphReadEdgeList, this makes exactly ONE allocation per node's neighbor (and weight)
// array and one allocation for the edge list: it reads the file TWICE. Pass 1 discovers the
// number of nodes and the exact (upper-bound) degree of each node; that lets us size every
// array at its final length before pass 2 fills them in, so GraphConnect's per-edge Realloc
// is never invoked. G->A (the adjacency-matrix SET**) is unused elsewhere in this file, so it
// is left untouched (NULL) here too--only G->neighbor is maintained.
//
// File format is the same as GraphReadEdgeList's, with the same optional header: if line 1 is
// a single whitespace-delimited token, it must be a non-negative integer giving the number of
// nodes (authoritative--Fatal()s if a larger node id shows up later); if--and only if--line 1
// was such a header, line 2 may likewise be a single integer giving the number of edges. That
// count is read and sanity-checked but never relied on, since pass 1 always determines the
// real count itself.
//
// NOTE: fp must be seekable; pass 2 begins with rewind(fp), so this cannot be used on a pipe/stdin.
GRAPH *GraphAddEdgeList(GRAPH *G, FILE *fp, Boolean directed, Boolean supportNodeNames, Boolean weighted)
{
    const int numExpected[2] = {2, 3}; // indexed by [weighted]
    // Always %s%s, even for plain integer node ids: %d/%u via sscanf has undefined behavior on
    // overflow, so numeric ids are captured as text here and validated/converted explicitly by
    // ParseNonNegUint below instead.
    const char *fmt[2] = {"%s%s ", "%s%s%f "}; // indexed by [weighted]
    char line[BUFSIZ];
    unsigned lineNum;
    Boolean haveHeaderN=false, haveHeaderM=false, selfSeen=false;
    unsigned headerN=0, headerM=0;

    TREETYPE *nameDict=NULL;
    char **names=NULL;
    unsigned namesCap=0;

    unsigned *degCap=NULL;   // degCap[v]: exact number of times v will be connected in pass 2 (upper bound; duplicate edges in the input are counted here but silently skipped in pass 2, same as GraphConnect always did)
    unsigned degCapAlloc=0;
    unsigned numNodes=0;     // authoritative once pass 1 finishes
    unsigned numEdgeLines=0; // upper bound on the final G->numEdges

    Note("2pass: first pass reading EdgeList");
    // ---------------- PASS 1: header + exact degree-counting ----------------
    lineNum = 0;
    while(fgets(line, sizeof(line), fp))
    {
	++lineNum;
	int len = strlen(line);
	while(len>0 && isspace((unsigned char)line[len-1])) line[--len]='\0';
	if(len==0) continue;

	float w;
	char v1[BUFSIZ], v2[BUFSIZ];
	int numRead = sscanf(line, fmt[weighted], v1, v2, &w);

	if(numRead==1 && lineNum<=2 && (lineNum==1 || haveHeaderN)) {
	    unsigned val = ParseNonNegUint(v1, lineNum);
	    if(lineNum==1) {
		headerN = val; haveHeaderN = true;
		Note("first header line claims %u nodes", headerN);
		namesCap = MAX(headerN,1);
		degCap = Calloc(namesCap, sizeof(degCap[0])); degCapAlloc = headerN;
		if(supportNodeNames) names = Malloc(namesCap*sizeof(names[0]));
	    } else {
		headerM = val; haveHeaderM = true; // read for sanity-checking only; pass 1 always computes the real edge count
		Note("second header line claims %u edges", headerM);
	    }
	    continue;
	}
	if(numRead != numExpected[weighted])
	    Fatal("GraphAddEdgeList: line %d must contain 2 %s%s, but instead is\n%s\n", lineNum,
		(supportNodeNames?"strings":"ints"), (weighted?" and a weight":""), line);

	unsigned i, j;
	if(supportNodeNames)
	{
	    if(!nameDict) nameDict = TreeAlloc((pCmpFcn)strcmp, (pFointCopyFcn)strdup, (pFointFreeFcn)free, NULL, NULL);
	    if(namesCap==0) { namesCap = MIN_EDGELIST; names = Malloc(namesCap*sizeof(names[0])); }
	    foint f1, f2;
	    Boolean new1 = !TreeLookup(nameDict, (foint)v1, &f1);
	    Boolean new2 = !TreeLookup(nameDict, (foint)v2, &f2);
	    unsigned newCount = new1 + new2; // how many of v1,v2 are actually new--0, 1, or 2--checked once, not assumed worst-case
	    if(haveHeaderN && numNodes+newCount > headerN)
		Fatal("GraphAddEdgeList: header declared only %u nodes but another distinct name appeared on line %d", headerN, lineNum);
	    while(numNodes+newCount > namesCap) { namesCap *= 2; names = Realloc(names, namesCap*sizeof(names[0])); }
	    if(new1) { names[numNodes]=Strdup(v1); f1.i=numNodes++; TreeInsert(nameDict,(foint)v1,f1); }
	    if(new2) { names[numNodes]=Strdup(v2); f2.i=numNodes++; TreeInsert(nameDict,(foint)v2,f2); }
	    i = f1.i; j = f2.i;
	}
	else {
	    i = ParseNonNegUint(v1, lineNum);
	    j = ParseNonNegUint(v2, lineNum);
	    if(haveHeaderN && MAX(i,j)+1 > headerN)
		Fatal("GraphAddEdgeList: header declared only %u nodes but node %u appeared on line %d", headerN, MAX(i,j), lineNum);
	    numNodes = MAX(numNodes, MAX(i,j)+1);
	}

	if(i==j && !selfSeen) {
	    if(supportNodeNames) Warning("GraphAddEdgeList: line %d has a self-loop (%s to itself); assuming self-loops are allowed", lineNum, v1);
	    else Warning("GraphAddEdgeList: line %d has a self-loop (%u to itself); assuming self-loops are allowed", lineNum, i);
	    selfSeen = true;
	}

	unsigned needed = MAX(i,j)+1;
	if(needed > degCapAlloc) { // only grows this small per-node counting array, never the neighbor lists themselves
	    unsigned newCap = MAX(2*degCapAlloc, needed);
	    degCap = Realloc(degCap, newCap*sizeof(degCap[0]));
	    memset(degCap+degCapAlloc, 0, (newCap-degCapAlloc)*sizeof(degCap[0]));
	    degCapAlloc = newCap;
	}
	++degCap[i];
	if(!directed && j!=i) ++degCap[j];
	++numEdgeLines;
    }
    if(haveHeaderN) numNodes = headerN;
    if(haveHeaderM && headerM != numEdgeLines)
	Warning("GraphAddEdgeList: header declared %u edges but the file actually contains %u", headerM, numEdgeLines);

    Note("2pass: found %u nodes and %u edges", numNodes, numEdgeLines);
    // ---------------- allocate G and give every array its final, exact size ----------------
    G = GraphAlloc(G, numNodes, directed, supportNodeNames, NULL); // degree[]=0, neighbor[]=NULL (Calloc'd)
    if(weighted) GraphMakeWeighted(G);
    G->selfAllowed = selfSeen;
    unsigned v;
    for(v=0; v<numNodes; v++) if(degCap[v]) {
	G->neighbor[v] = Malloc(degCap[v]*sizeof(G->neighbor[v][0]));
	if(weighted) G->weight[v] = Malloc(degCap[v]*sizeof(G->weight[v][0]));
    }
    G->maxEdges = MAX(numEdgeLines,1);
    G->edgeList = Realloc(G->edgeList, 2*G->maxEdges*sizeof(G->edgeList[0])); // one Realloc total, not one per edge
    if(supportNodeNames) {
	G->name = Realloc(names, MAX(numNodes,1)*sizeof(names[0])); // shrink to exact size, once
	G->nameDict = nameDict;
    }
    Free(degCap);

    // ---------------- PASS 2: rewind and connect every edge, with zero further realloc's ----------------
    rewind(fp);
    lineNum = 0;
    haveHeaderN = false; // replay the exact same header-detection logic as pass 1
    Note("2pass: second pass reading EdgeList");
    while(fgets(line, sizeof(line), fp))
    {
	++lineNum;
	int len = strlen(line);
	while(len>0 && isspace((unsigned char)line[len-1])) line[--len]='\0';
	if(len==0) continue;

	float w=1;
	char v1[BUFSIZ], v2[BUFSIZ];
	int numRead = sscanf(line, fmt[weighted], v1, v2, &w);
	if(numRead==1 && lineNum<=2 && (lineNum==1 || haveHeaderN)) { if(lineNum==1) haveHeaderN=true; continue; }

	unsigned i, j;
	if(supportNodeNames) { i = GraphNodeName2Int(G, v1); j = GraphNodeName2Int(G, v2); }
	else { i = ParseNonNegUint(v1, lineNum); j = ParseNonNegUint(v2, lineNum); }
	if(weighted) assert(w>0.0);

	if(GraphAreConnected(G, i, j)) continue; // duplicate edge in the input; skip, same semantics as GraphConnect
	G->neighbor[i][G->degree[i]] = j;
	if(weighted) G->weight[i][G->degree[i]] = w;
	G->degree[i]++;
	if(!directed && j!=i) {
	    G->neighbor[j][G->degree[j]] = i;
	    if(weighted) G->weight[j][G->degree[j]] = w;
	    G->degree[j]++;
	}
	G->edgeList[2*G->numEdges] = i;
	G->edgeList[2*G->numEdges+1] = j;
	G->numEdges++;
    }

    assert(G->numEdges <= G->maxEdges);
    GraphSort(G);
    return G;
}

GRAPH *GraphFromEdgeList(GRAPH *G, unsigned n, unsigned m, unsigned *pairs, Boolean directed, float *weights)
{
    int i;
    if(!G) G = GraphAlloc(NULL, n, directed, false, NULL); // will set names later
    if(weights) GraphMakeWeighted(G);
    assert(n == G->n);
    assert(G->degree);
    if(G->weight) assert(weights);
    for(i=0;i<n;i++)
	assert(!G->neighbor[i]);
    for(i=0; i<m; i++) {
	if(pairs[2*i] == pairs[2*i+1] && !G->selfAllowed) {
	    static Boolean warned;
	    if(!warned) Warning("GraphFromEdgeList: node %d has a self-loop; assuming they are allowed", pairs[2*i]);
	    warned = G->selfAllowed = true;
	}
        assert(pairs[2*i] < n && pairs[2*i+1]<n);
	GraphConnect(G, pairs[2*i], pairs[2*i+1]);
	if(weights) {assert(weights[i]!=0.0); GraphSetWeight(G, pairs[2*i], pairs[2*i+1], weights[i]);}
    }
    assert(G->neighbor);
    GraphSort(G);
    return G;
}

// Returns a *constant* string; you need to dup it if you want to keep it
char *HashString(char *s)
{
    static char *hash;
    static int buflen;
    int n = strlen(s);
    if(buflen < n+1){
	buflen = 2*n+1;
	hash = realloc(hash, buflen); // realloc accepts NULL
	assert(hash);
    }
    return hash;
}

GRAPH *GraphReadEdgeList(GRAPH *G, FILE *fp, Boolean directed, Boolean supportNodeNames, Boolean weighted)
{
    unsigned numNodes=0, lineNum=0;
    unsigned numEdges=0, maxEdges=MIN_EDGELIST; // these will be increased as necessary during reading
    unsigned *pairs = Malloc(2*maxEdges*sizeof(pairs[0]));
    float *fweight = NULL;
    if(weighted) fweight=Malloc(maxEdges*sizeof(fweight[0]));

    // SUPPORT_NODE_NAMES
    unsigned maxNodes=MIN_EDGELIST;
    char **names = NULL;
    TREETYPE *nameDict = NULL;
    if(supportNodeNames)
    {
	names = Malloc(maxNodes*sizeof(char*));
	nameDict = TreeAlloc((pCmpFcn)strcmp, (pFointCopyFcn)strdup, (pFointFreeFcn)free, NULL, NULL);
    }

    char line[BUFSIZ];
    static Boolean selfWarned;
    while(fgets(line, sizeof(line), fp))
    {
	++lineNum;
	// nuke all whitespace, including DOS carriage returns, from the end of the line
	int len = strlen(line);
	while(isspace(line[len-1])) line[--len]='\0';
	float w;
	assert(numEdges <= maxEdges);
	if(numEdges >= maxEdges)
	{
	    maxEdges = 2*maxEdges-1; // -1 to reduce chance of overflow near 2GB and 4GB.
	    unsigned newBytes = 2*maxEdges*sizeof(pairs[0]);
	    if(newBytes >= (1U<<30)) {
		Warning("about to Reallac(%u) bytes--might segfault; implement GraphAddEdgeList to avoid this", newBytes);
	    }
	    pairs = Realloc(pairs, newBytes);
	    if(weighted) fweight = Realloc(fweight, maxEdges*sizeof(fweight[0]));
	}
	const char numExpected[2] = {2, 3}, // fmt[][] below has dimensions [supportNames][weighted]
	    *fmt[2][2] = {{"%d%d ", "%d%d%f "}, {"%s%s ", "%s%s%f "}};
	// name and foints are used only if supportNodeNames is true
	union {int i; char name[BUFSIZ];} v1, v2;
	foint f1, f2;
	// Note: if !supportNodeNames, a binary integer will be written into the name unions
	int numRead = sscanf(line, fmt[supportNodeNames][weighted], v1.name, v2.name, &w);
	if(numRead==1) { // the first two lines may encode _numNodes and _numEdges, respectively
	    static unsigned _numNodes=-1, _numEdges=-1;
	    if(lineNum==1) { sscanf(line, "%u", &_numNodes); continue;}
	    if(lineNum==2) { sscanf(line, "%u", &_numEdges); continue;}
	}
	if(numRead != numExpected[weighted]) {
	    Fatal("GraphReadEdgeList: line %d must contain 2 %s%s, but instead is\n%s\n", lineNum,
		(supportNodeNames ? "strings":"ints"), (weighted ? " and a weight":""), line);
	}
	if(supportNodeNames)
	{
	    assert(numNodes <= maxNodes);
	    if(numNodes+2 >= maxNodes) // -2 for a bit of extra space
	    {
		maxNodes *=2;
		names = Realloc(names, maxNodes*sizeof(names[0]));
	    }
	    if(strcmp(v1.name,v2.name)==0 && !selfWarned) {
		Warning("GraphReadEdgeList: line %d has self-loop (%s to itself); assuming they are allowed",numEdges,v1.name);
		Warning("GraphReadEdgeList: (another warning will appear below from \"GraphFromEdgeList\")");
		selfWarned = true;
	    }
	    if(!TreeLookup(nameDict, (foint)v1.name, &f1))
	    {
		names[numNodes] = Strdup(v1.name);
		f1.i = numNodes++;
		TreeInsert(nameDict, (foint)v1.name, f1);
	    }
	    if(!TreeLookup(nameDict, (foint)v2.name, &f2))
	    {
		names[numNodes] = Strdup(v2.name);
		f2.i = numNodes++;
		TreeInsert(nameDict, (foint)v2.name, f2);
	    }
	    v1.i = f1.i; v2.i = f2.i;
	}
	else {
	    if(v1.i==v2.i && !selfWarned) {
		Warning("GraphReadEdgeList: line %d has self-loop (%d to itself); assuming they are allowed",numEdges,v1.i);
		Warning("GraphReadEdgeList: (another warning will appear below from \"GraphFromEdgeList\")");
		selfWarned = true;
	    }
	    // if !supportNodeNames, fscanf wrote the integers into the char* pointers
	    numNodes = MAX(numNodes, v1.i+1);
	    numNodes = MAX(numNodes, v2.i+1);
	}
	pairs[2*numEdges] = v1.i;
	pairs[2*numEdges+1] = v2.i;
	if(weighted) { assert(w>0.0); fweight[numEdges] = w;}
	numEdges++;
    }
    if(supportNodeNames)
    {
	//printf("TREETYPE Dictionary Dump\n");
	unsigned i;
	for(i=0; i<numNodes;i++)
	{
	    foint info;
	    if(!TreeLookup(nameDict, (foint)names[i], &info))
		Fatal("couldn't find int for name '%s'", names[i]);
	    assert(i == info.i);
	    //printf("%d is %s which in turn is %d\n", i, names[i], info.i);
	}
    }

    G = GraphAlloc(G, numNodes, directed, selfWarned, NULL);
    GraphFromEdgeList(G, numNodes, numEdges, pairs, directed, fweight);
    G->supportNodeNames = supportNodeNames;
    if(supportNodeNames) {
	G->nameDict = nameDict;
	G->name = names;
    }
    Free(pairs);
    if(weighted) Free(fweight);
    assert(G->maxEdges <= maxEdges);
    assert(G->numEdges <= numEdges);
    assert(G->neighbor);
    GraphSort(G);
    return G;
}

int GraphNodeName2Int(GRAPH *G, char *name)
{
    foint info;
    if(!TreeLookup(G->nameDict, (foint)name, &info))
	Fatal("TreeLookup couldn't find an int for name '%s'", name);
    return info.i;
}

void GraphPrintConnections(FILE *fp, GRAPH *G)
{
    int i, j;
    fprintf(fp, "%d\n", G->n);
    for(i=0; i<G->n; i++) for(j=0; j<G->degree[i]; j++)
	fprintf(fp, "%d %d\n", i, G->neighbor[i][j]);
}

GRAPH *GraphReadConnections(GRAPH *G, FILE *fp, Boolean directed)
{
    int n, i, j, d;
    if(fscanf(fp, "%d", &n) != 1)
	Fatal("GraphReadConnections: failed to read 'n'");
    assert(n >= 0);
    if(!G) G = GraphAlloc(NULL, n, directed, false, NULL);

    while((d=fscanf(fp, "%d %d", &i, &j)) == 2)
    {
	if(i==-1 && j==-1)
	{
	    d=0;
	    break;
	}
	assert(0 <= i && i < G->n);
	assert(0 <= j && j < G->n);
	if(i==j) {
	    static Boolean warned;
	    if(!warned) Warning("GraphReadConnections: node %d has a self-loop; assuming they are allowed", i);
	    warned = G->selfAllowed = true;
	}
	GraphConnect(G, i, j);
    }
    if(d > 0)
	Fatal("expecting no more integers, but got %d integers", d);
    GraphSort(G);
    return G;
}


GRAPH *GraphComplement(GRAPH *G)
{
    if(G->directed) assert("Sorry, complement doesn't work for directed graphs right now");
    int i, j;
    if(G->supportNodeNames) GraphNameWarn("GraphComplement");
    GRAPH *Gbar = GraphAlloc(NULL, G->n, G->directed, false, G->edgeWeightFn);
    Gbar->selfAllowed = G->selfAllowed;
    assert(Gbar->n == G->n);
    for(i=0; i < G->n; i++) for(j=0; j < G->n; j++)
        if(!GraphAreConnected(G, i, j)&&i!=j)
            GraphConnect(Gbar, i, j);
    if(G->selfAllowed) for(i=0; i < G->n; i++) if(!GraphAreConnected(G,i,i)) GraphConnect(Gbar,i,i);
    if(G->directed==0) GraphSort(Gbar);
    return Gbar;
}


GRAPH *GraphUnion(GRAPH *G1, GRAPH *G2)
{
    int i, j, n = G1->n;

    if(G1->n != G2->n)
	return NULL;

    assert(G1->selfAllowed == G2->selfAllowed);
    assert(G1->directed == G2->directed);
    GRAPH *dest=NULL;
    if(!G1->selfAllowed) dest = GraphAlloc(NULL, n, G1->directed, G1->supportNodeNames, G1->edgeWeightFn);
    else dest = GraphSelfAlloc(n, G1->directed, G1->supportNodeNames, G1->edgeWeightFn);
    if(G1->supportNodeNames || G2->supportNodeNames) GraphNameWarn("GraphUnion");

    for(i=0; i < n; i++) for(j=0; j < n; j++)
	if(GraphAreConnected(G1, i, j) || GraphAreConnected(G2, i, j))
	    GraphConnect(dest, i ,j);
    if(G1->selfAllowed)
	for(i=0; i < G1->n; i++) if(GraphAreConnected(G1, i, i) || GraphAreConnected(G2, i, i)) GraphConnect(dest, i ,i);
    GraphSort(dest);
    return dest;
}


int GraphBFS(GRAPH *G, int root, int distance, int *nodeArray, int *distArray)
{
    QUEUE *BFSQ;
    int i, count = 0;

    assert(0 <= root && root < G->n);
    assert(distance >= 0);
    assert(nodeArray != NULL);
    assert(distArray != NULL);

    if(distance == 0) /* We could let the rest of the routine run, but why bother? */
    {
	nodeArray[0] = root;
	distArray[root] = 0;
	return 1;
    }

    for(i=0; i<G->n; i++)
	nodeArray[i] = distArray[i] = -1;

    distArray[root] = 0;
    BFSQ = QueueAlloc(G->n);
    QueuePut(BFSQ, (foint)root);
    while(QueueSize(BFSQ) > 0)
    {
	int v = QueueGet(BFSQ).i;

	/* At this point, distArray[v] should be assigned (when v was appended
	 * onto the queue), but v hasn't been "visited" or "counted" yet.
	 */

	assert(0 <= v && v < G->n);
	assert(0 <= distArray[v] && distArray[v] < G->n);

	assert(nodeArray[count] == -1);
	nodeArray[count] = v;
	count++;

	if(distArray[v] < distance) /* v's neighbors will be within BFS distance */
	{
	    int j;
	    for(j=0; j < G->degree[v]; j++)
                if((G->neighbor[v][j]) == v) assert(G->selfAllowed); // nothing to do; don't add self to a BFS
                else if(distArray[G->neighbor[v][j]] == -1) /* some of the neighbors might have already been visited */
                {
                    distArray[G->neighbor[v][j]] = distArray[v] + 1;
                    QueuePut(BFSQ, (foint)(G->neighbor[v][j]));
                }
	}
    }
    QueueFree(BFSQ);
    return count;
}

static Boolean _GraphCCatLeastKHelper(GRAPH *G, SET* visited, int v, int *k);
Boolean GraphCCatLeastK(GRAPH *G, int v, int k) {
    SET* visited = SetAlloc(G->n);
    Boolean result = _GraphCCatLeastKHelper(G, visited, v, &k);
    SetFree(visited);
    return result;
}

/* visited holds previously visited nodes, v holds the current vertex, k holds the remaining count
** Visit the current node
** if the remaining count reached 0
**      return true
** For each adjacent node
**      if it hasn't been visited
**          recursive call to dfs the node
** return false if the CC wasn't at least k
*/
static Boolean _GraphCCatLeastKHelper(GRAPH *G, SET* visited, int v, int *k) {
    assert(!G->directed);
    SetAdd(visited, v);
    *k -= 1;
    if (*k <= 0) return true;
    int i;
    for (i = 0; i < G->degree[v]; i++) {
        if ((G->neighbor[v][i])==v) assert(G->selfAllowed); // nothing to do, don't add self in a DFS
        else if (!SetIn(visited, G->neighbor[v][i])) {
            Boolean result = _GraphCCatLeastKHelper(G, visited, G->neighbor[v][i], k);
            if (result)
                return result;
        }
    }
    return false;
}

/* At top-level call, set (*pn)=0. The visited array does *not* need to be clear, but everything needs to be allocated.
** We return the number of elements in Varray.
*/
int GraphVisitCC(GRAPH *G, unsigned int v, SET *visited, unsigned int *Varray, int *pn)
{
    assert(v < SetMaxSize(visited));
    if(!SetIn(visited,v))
    {
	SetAdd(visited, v);
	Varray[(*pn)++] = v;
    	int i;
	for(i=0; i < G->degree[v]; i++)
	    if((G->neighbor[v][i])==v) assert(G->selfAllowed);
	    else GraphVisitCC(G, G->neighbor[v][i], visited, Varray, pn);
    }
    return *pn;
}

/* doesn't allow Gv == G */
GRAPH *GraphInduced(GRAPH *G, SET *V)
{
    unsigned array[G->n], nV = SetToArray(array, V), i, j;
    if(G->supportNodeNames) GraphNameWarn("GraphInduced");
    GRAPH *Gv = GraphAlloc(NULL, nV, G->directed, false, G->edgeWeightFn);
    Gv->selfAllowed = G->selfAllowed;
    for(i=0; i < nV; i++) for(j=0; j < nV; j++)
	if(GraphAreConnected(G, array[i], array[j]))
	    GraphConnect(Gv, i, j);
    if(G->selfAllowed) for(i=0; i < nV; i++) if(GraphAreConnected(G, array[i], array[i])) GraphConnect(Gv, i, i);
    GraphSort(Gv);
    return Gv;
}

/* allows Gv == G */
GRAPH *GraphInduced_NoVertexDelete(GRAPH *G, SET *V)
{
    unsigned array[G->n], nV = SetToArray(array, V), i, j;
    if(G->supportNodeNames) GraphNameWarn("GraphInduced_NoVertexDelete");
    GRAPH *Gv = GraphAlloc(NULL, G->n, G->directed, false, G->edgeWeightFn);
    Gv->selfAllowed = G->selfAllowed;

    for(i=0; i < nV; i++) for(j=0; j < nV; j++)
	if(GraphAreConnected(G, array[i], array[j]))
	    GraphConnect(Gv, array[i], array[j]);
    if(G->selfAllowed) for(i=0; i < nV; i++) if(GraphAreConnected(G, array[i], array[i])) GraphConnect(Gv, array[i], array[i]);
    GraphSort(Gv);
    return Gv;
}

/*
** A reasonably fast search for triangles.
*/

/*
** This is a helper function for Contains Kn, but you can use it.  It
** tells you if adding this edge will cause a triangle.  Note that
** this function ONLY works if self-loops do not exist!
*/

Boolean GraphConnectingCausesK3(GRAPH *G, int i, int j)
{
    assert(G->directed==0);
    int numIntersect;
    SET *C = SetAlloc(G->n);
    SetIntersect(C, G->A[i], G->A[j]);
    numIntersect = SetCardinality(C);
    SetFree(C);
    return numIntersect != 0;
}

Boolean GraphContainsK3(GRAPH *G)
{
    assert(G->directed==0);
    int i,j;
    for(i=0; i < G->n; i++)
	for(j=i+1; j < G->n; j++)
	    if(GraphAreConnected(G,i,j) && GraphConnectingCausesK3(G,i,j))
		return true;
    return false;
}


/**************************************************************************
**
** These are the Clique and Indep set (exponential time) algorithms.
** They work for general graphs.
**
**************************************************************************/

/*
** We check to see if these particular k vertices are a clique by
** bitwise-anding together their adjacency lists (with self-loop added
** to each).  Return false of it's not a clique, otherwise return the set
** of nodes.
** Note that if the graph has changed since the previous call, we may miss some cliques.
*/
SET *Graph_IsCombClique(CLIQUE *c)
{
    SET *intersect;
    int i;
    SetEmpty(c->set);
    for(i=0; i < c->cliqueSize; i++)
	SetAdd(c->set, c->inducedArray[c->combArray[i]]);

    intersect = SetCopy(NULL, c->set);

    for(i=0; i < c->cliqueSize; i++)
    {
	int node = c->inducedArray[c->combArray[i]];
	/* Temporarily add in self-loop for intersection purposes */
	SetAdd(c->G->A[node], node);
	SetIntersect(intersect, intersect, c->G->A[node]);
	SetDelete(c->G->A[node], node);
	if(!SetEq(intersect, c->set))
	{
	    SetFree(intersect);
	    return NULL;
	}
    }
    assert(SetEq(intersect, c->set));
    SetFree(intersect);
    return c->set;
}

SET *GraphKnNext(CLIQUE *c)
{
    SET *s;
    while(CombinNext(c->combin))
	if((s = Graph_IsCombClique(c)))
	    return s;
    return NULL;
}

CLIQUE *GraphKnFirst(GRAPH *G, int k)
{
    assert(G->directed==0);
    int i, nDegk = G->n;
    SET *setDegk;
    CLIQUE *c;

    assert(k <= G->n);
    if(k == 0)
	return NULL;

    setDegk = SetAlloc(G->n);
    c = (CLIQUE*)Calloc(1,sizeof(CLIQUE));
    c->G = GraphCopy(G);
    c->cliqueSize = k;
    c->inducedArray = Calloc(G->n, sizeof(c->inducedArray[0]));

#if 1
    /*
    ** First reduce the potential number of vertices needed to check. A
    ** necessary condition for a vertex to be a member of a Kk is to have
    ** degree >= k-1, and all edges go to other vertices of degree >= k-1.
    ** Iterate inducing subgraphs 'til we cain't induce no more...
    */
    while(nDegk >= k)
    {
	int prevNDegk = nDegk;  /* just to check for changes */
	nDegk = 0;
	SetEmpty(setDegk);
	for(i=0; i < c->G->n; i++)
	    if(c->G->degree[i] >= k-1)
	    {
		++nDegk;
		SetAdd(setDegk, i);
	    }
	if(nDegk == prevNDegk)  /* nobody eliminated */
	    break;
	GRAPH *tmp = GraphInduced_NoVertexDelete(c->G, setDegk);
	GraphFree(c->G);
	c->G = tmp;
    }

    if(nDegk < k)
    {
	Free(c->inducedArray);
	GraphFree(c->G);
	Free(c);
	SetFree(setDegk);
	return NULL;
    }
    i = SetToArray(c->inducedArray, setDegk);
    assert(i == nDegk);
#else
    nDegk = G->n;
    for(i=0; i < nDegk; i++)
	c->inducedArray[i] = i;
#endif
    SetEmpty(setDegk);
    c->set = setDegk; /* re-use setDegK */
    c->combArray = Calloc(k, sizeof(c->combArray[0]));
    c->combin = CombinZeroth(nDegk, k, c->combArray);
    if(Graph_IsCombClique(c) || GraphKnNext(c))
	return c;
    /* else */
    GraphCliqueFree(c);
    return NULL;
}

void GraphCliqueFree(CLIQUE *c)
{
    CombinFree(c->combin);
    Free(c->combArray);
    Free(c->inducedArray);
    GraphFree(c->G);
    SetFree(c->set);
    Free(c);
}

CLIQUE *GraphInFirst(GRAPH *G, int n)
{
    assert(G->directed==0);
    GRAPH *Gbar = GraphComplement(G);
    CLIQUE *c = GraphKnFirst(Gbar, n);
    GraphFree(Gbar);
    return c;
}


Boolean GraphKnContains(GRAPH *G, int n)
{
    assert(G->directed==0);
    CLIQUE *c;
    assert(n>=0);
    if(n < 2)
	return true;
    if(n > G->n)
	return false;
    c = GraphKnFirst(G, n);
    if(c)
	GraphCliqueFree(c);
    return c != NULL;
}

Boolean GraphInContains(GRAPH *G, int n)
{
    assert(G->directed==0);
    GRAPH *Gbar = GraphComplement(G);
    Boolean b = GraphKnContains(Gbar, n);
    GraphFree(Gbar);
    return b;
}

/**************************************************************************
**
**  Graph Isomorphism
**
**************************************************************************/

static GRAPH *isoG1, *isoG2;

static Boolean _permutationIdentical(int n, int perm[n])
{
    assert(!isoG1->directed&&!isoG2->directed);
    int i, j;
    for(i=0; i<n; i++)
	if(isoG1->degree[i] != isoG2->degree[perm[i]])
	    return false;

    for(i=0; i<n; i++) for(j=i+1; j<n; j++)
	/* The !GraphAreConnected is just to turn a bitstring into a boolean */
	if(!GraphAreConnected(isoG1, i,j) !=
	    !GraphAreConnected(isoG2, perm[i], perm[j]))
	    return false;   /* non-isomorphic */
    // Note they don't both have to ALLOW self loops, but if one does, then both need to have or not the same loops
    if(isoG1->selfAllowed || isoG2->selfAllowed) for(i=0; i<n; i++)
       if(!GraphAreConnected(isoG1, i,i) != !GraphAreConnected(isoG2, perm[i], perm[i])) return false;   /* non-isomorphic */
    return true;   /* isomorphic! */
}

Boolean GraphsIsomorphic(int *perm, GRAPH *G1, GRAPH *G2)
{
    assert(!G1->directed&&!G2->directed);
    static int recursionDepth;
    ++recursionDepth;
    assert(recursionDepth <= G1->n + 1);
    Boolean self = (G1->selfAllowed || G2->selfAllowed);
    int i, n = G1->n, degreeCount1[n+self], degreeCount2[n+self];

    SET *degreeOnce;

    /*
    ** First some simple tests.
    */
    if(G1->n != G2->n) {--recursionDepth; return false;}

    if(n < 2) {--recursionDepth; return true;}

    /*
    ** Ensure each degree occurs the same number of times in each... and the count can be == n if selfAllowed is true
    */
    for(i=0; i<n+self; i++) degreeCount1[i] = degreeCount2[i] = 0;
    for(i=0; i<n; i++) {
	++degreeCount1[G1->degree[i]];
	++degreeCount2[G2->degree[i]];
    }
    for(i=0; i<n+self; i++) if(degreeCount1[i] != degreeCount2[i]) {--recursionDepth; return false;}

    /*
    ** Let degree d appear only once.  Then there is exactly one vertex
    ** v1 in G1 with degree d, and exactly one vertex v2 in G2 with degree d.
    ** G1 and G2 are isomorphic only if the neighborhood of v1 is isomorphic
    ** to the neighborhood of v2.
    */
    degreeOnce = SetAlloc(n+self);
    for(i=0; i<n+self; i++) if(degreeCount1[i] == 1) SetAdd(degreeOnce, i);
    for(i=0; i<n+self; i++)
    {
	/* Find out if the degree of vertex i in G1 appears only once */
	if(SetIn(degreeOnce, G1->degree[i]))
	{
	    int j, degree = G1->degree[i];
	    GRAPH *neighG1i, *neighG2j;

	    /* find the (unique) vertex in G2 that has the same degree */
	    for(j=0; j < n; j++) if(G2->degree[j] == degree) break;
	    assert(j < n);
            // remove self-loops from the set to induce on...
            SET *G1Ai = SetCopy(NULL, G1->A[i]), *G2Aj = SetCopy(NULL, G2->A[j]);
            SetDelete(G1Ai, i); neighG1i = GraphInduced(G1, G1->A[i]);
            SetDelete(G2Aj, j); neighG2j = GraphInduced(G2, G2->A[j]);


	    /*
	    ** Note: this recursion works only as long as _permutationIdentical doesn't call GraphsIsomorphic.
	    ** (if it does, isoG1 and isoG2 get messed up). Also, notice that it's fine that we re-use the perm[]
	    ** array on this recursion since the array doesn't actually get used until the bottom of this function
	    ** when calling CombinAllPermutatiotns().
	    */
	    j = GraphsIsomorphic(perm, neighG1i, neighG2j);
	    GraphFree(neighG1i);
	    GraphFree(neighG2j);
	    if(!j) {--recursionDepth; return false;}
	    /* Otherwise they *might* be isomorphic, so keep going */
	}
    }
    SetFree(degreeOnce);

    /*
    ** Oh well, fire up the exponential search.
    ** CombinAllPermutations will return 0 iff all permutations were
    ** tried; the function _permutationIdentical should return non-zero
    ** when it finds an identical permutation, and that non-zero value
    ** will be returned here, indicating an identical permutation was
    ** found, ie, that the graphs are isomorphic.
    */
    isoG1 = G1; isoG2 = G2;
    --recursionDepth;
    return !!CombinAllPermutations(n, perm, _permutationIdentical);
}
#ifdef __cplusplus
} // end extern "C"
#endif
