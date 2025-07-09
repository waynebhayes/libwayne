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
#include "mem-debug.h"

/*************************************************************************
**
**                            The Basics
**
*************************************************************************/

GRAPH *GraphAlloc(unsigned n, Boolean self, Boolean directed, Boolean weighted)
{
    static Boolean needStartup = 1;
    int i;
    GRAPH *G = Calloc(1, sizeof(GRAPH));
    if(needStartup)
    {
	needStartup = 0;
	SetStartup();
    }
    G->n = n;
    G->self = self;
    G->directed = directed;
    if(weighted) Apology("sorry no weighted graphs yet in libwayne/graph.c");
    G->m = 0;
    G->A = Calloc(n, sizeof(G->A[0]));
    for(i=0; i<n; i++) {
	G->A[i] = SetAlloc(n);
	//SetUseMaxCrossover(G->A[i]); // needed only if BitvecNext doesn't work
    }
    return G;
}

GRAPH *GraphComputeCumDegree(GRAPH *G)
{
    if(!G->cumDegree) {
	G->cumDegree = Calloc((2+G->n),sizeof(G->cumDegree[0])); // extra+1 for G->n, and another for DEADBEEF
	assert(G->m < (1U << (8*sizeof(SET_ELEMENT_TYPE)-1))); // must be < 2^31 to avoid overflow when multiplying by 2
    }
    G->cumDegree[0] = 0;
    unsigned i;
    for(i=0; i<G->n; i++) // includes element for G->n
	G->cumDegree[i+1] = G->cumDegree[i] + GraphDegree(G, i); // up to BUT NOT including i
    if(i!=G->n || G->cumDegree[i] != 2*G->m) // 2*m since each edge appears TWICE: once from each end
	Fatal("GraphAllocCumDegree corrupt; possibly due to being called by multiple threads simultaneously?");
    G->cumDegree[G->n+1] = 0xDEADBEEF;
    return G;
}

void GraphFree(GRAPH *G)
{
    unsigned i;
    if(G->weight) Free(G->weight);
    if(G->name) {
	for(i=0;i<G->n;i++) Free(G->name[i]);
	Free(G->name);
    }
    if(G->nameDict) BinTreeFree(G->nameDict);
    if(G->cumDegree) Free(G->cumDegree);
    for(i=0; i<G->n; i++) SetFree(G->A[i]);
    Free(G->A);
    Free(G);
}

GRAPH *GraphConnect(GRAPH *G, int i, int j)
{
    if(G->useComplement) Apology("Sorry haven't implemented useComplement yet");
    assert(0 <= i && i < G->n && 0 <= j && j < G->n);
    if(GraphAreConnected(G, i, j))
    {
	if(i!=j && !G->directed) assert(GraphAreConnected(G, j, i));
	return G;
    }
    assert(!SetIn(G->A[i], j));
    SetAdd(G->A[i], j);
    G->m++;
    if(i==j) { assert(G->self); G->numSelf++; }
    else if(!G->directed) {
	assert(!SetIn(G->A[j], i));
	SetAdd(G->A[j], i);
    }
    return G;
}

GRAPH *GraphDisconnect(GRAPH *G, int i, int j)
{
    if(G->useComplement) Apology("Sorry haven't implemented useComplement yet");
    assert(0 <= i && i < G->n && 0 <= j && j < G->n);
    if(!GraphAreConnected(G, i, j))
    {
	if(i!=j && !G->directed) assert(!GraphAreConnected(G, j, i));
	return G;
    }
    assert(SetIn(G->A[i], j));
    SetDelete(G->A[i], j);
    G->m--;
    if(i==j) { assert(G->self); G->numSelf--; }
    else if(!G->directed) {
	assert(SetIn(G->A[j], i));
	SetDelete(G->A[j], i);
    }
    return G;
}

Boolean GraphAreConnect(GRAPH *G, unsigned i, unsigned j)
{
    return (SetIn(G->A[i], j) != G->useComplement);
}

GRAPH *GraphCopy(GRAPH *G)
{
    Apology("no GraphCopy yet");
    int i;
    GRAPH *Gc = GraphAlloc(G->n, G->self, G->directed, !!G->weight);
    Gc->useComplement = G->useComplement;
    Gc->m = G->m;
    Gc->n = G->n;
    return Gc;
}

double GraphSetWeight(GRAPH *G, unsigned i, unsigned j, float w) { Apology("no weights yet"); return 0.0; }
double GraphGetWeight(GRAPH *G, unsigned i, unsigned j)          { Apology("no weights yet"); return 0.0; }

GRAPH *GraphSort(GRAPH *G)
{
    int i;
    for(i=0; i < G->n; i++) SetSort(G->A[i]);
    return G;
}

GRAPH *GraphEdgesAllDelete(GRAPH *G)
{
    int i;
    for(i=0; i < G->n; i++) SetEmpty(G->A[i]);
    G->numSelf = G->m = 0;
    return G;
}

SET_ELEMENT_TYPE GraphRandomEdge(GRAPH *G, int *u, int *v)
{
    SET_ELEMENT_TYPE edge;
#if DUMB
    if(*u==-1) Apology("sorry, picking a specific edge not implemented in dumb RandomEdge");
    do { *u=G->n*drand48(); *v=G->n*drand48(); } while((*u==*v && !G->self) || !GraphAreConnected(G,*u,*v));
    return 0;
#else
    SET_ELEMENT_TYPE i, j;
    if(*u==-1) {
	assert(*v>=0 && *v < 2*G->m);
	edge = *v;
	// Code to warn if user doesn't seem to realize edge can be >=G->m
	static Boolean neverGtGm = true;
	if(neverGtGm) {
	    if(*v>=G->m) neverGtGm=false;
	    else {
		static int numLessGm;
		++numLessGm;
		if(numLessGm > G->m)
		    Fatal("GraphRandomEdge: specified edge should be <2*G->m, but has been <G->m too frequently");
	    }
	}
    }
    else
	edge = drand48() * (2*G->m); // target edge--don't forget each edge appears TWICE
    assert(G->cumDegree && G->cumDegree[G->n+1] == 0xDEADBEEF);
#if LINEAR_SEARCH
    for(i=0;i<G->n;i++) if(edge < G->cumDegree[i+1]) break;
#else
    // binary search
    int tries=0;
    int low=0, high=G->n, mid=G->n/2; // inclusive
    while(low<high) {
	assert(++tries < 1000);
	mid = (high+low)/2;
	assert(low <= mid && mid <= high);
	if(edge > G->cumDegree[mid]) low=mid+1;
	else high=mid-1;
    }
    i=mid; // not sure why this is sometimes 1 or 2 off
    for(;i<G->n;i++) if(edge <  G->cumDegree[i+1]) break;
    for(;i>=0  ;i--) if(edge >= G->cumDegree[i]) break;
#endif
    assert(0<=i && i<G->n);
    assert(G->cumDegree[i]<=edge && edge < G->cumDegree[i+1] && edge-G->cumDegree[i]<GraphDegree(G,i));
    if(*u==-1) {
	*v = GraphNeighbor(G,i,edge-G->cumDegree[i]);
    } else
	*v = GraphRandomNeighbor(G,i);
    *u = i;
    assert(GraphAreConnected(G,*u,*v) && (!G->directed && GraphAreConnected(G,*v,*u)));
#endif
    return edge;
}

int GraphRandomNeighbor(GRAPH *G, int u)
{
    assert(GraphDegree(G,u)>0);
    return SetRandomElement(G->A[u]);
}

int GraphNeighbor(GRAPH *G, int u, int n) {
    return SetElement(G->A[u], n);
}

unsigned GraphNextNeighbor(GRAPH *G, unsigned u, unsigned *buf)
{
    if(G->useComplement) {
	Apology("Sorry, no GraphNextNeighbor for complement graphs");
	if(*buf == G->n) return G->n;
	else return (*buf)++;
    } else {
	return SetNextElement(G->A[u], buf); // don't touch buf here, SetNextElement will adjust it
    }
}

#if 0
// Basic idea: loop through ALL neighbors v of i, increment count if j is also connected to v (including self-loops)
// This works even if self-loops are allowed, because if (u,u) and (u,v) both exist, then u is neighbor to both
unsigned GraphNumCommonNeighbors(GRAPH *G, unsigned i, unsigned j)
{
    assert(0 <= i && i < G->n && 0 <= j && j < G->n);
    int numCommon1 = 0, numCommon2 = 0; // for G', these are actually the number of NON-common
    if(i==j) {
	if(G->useComplement) numCommon1 = G->n - GraphDegree(G,i);
	else numCommon1 = GraphDegree(G,i); // it's the same node, so number of common neighbors is all neighbors
    }
    if(G->sparse>=true) // loop version is MUCH faster on large sparse graphs, so use it if possible
    {
	unsigned k, n;
	// ensure i has the shorter list, j has the larger
	if(GraphDegree(G,i) > GraphDegree(G,j)) { int tmp=j; j=i; i=tmp; }
	n = GraphDegree(G,i);
	for(k=0; k<n; k++)
	    Apology("noCN"); // if(GraphAreConnected(G, G->neighbor[i][k], j)) ++numCommon1;
	// for G complement, we need to check neighbors of BOTH i and j, and then subtract the sum from G->n
	if(G->useComplement) {
	    n = GraphDegree(G,j);
	    for(k=0; k<n; k++)
		Apology("noCN"); //if(GraphAreConnected(G, G->neighbor[j][k], i)) ++numCommon1;
	    // Inverting it is a bit tricky: if self-loops are ALLOWED, then max possible value is G->n, and any
	    // self-loops that actually exist above become non-connnected in G', so they SHOULD be subtracted.
	    numCommon1 = G->n - numCommon1;
	    // However, if self-loops are NOT allowed, then max possible value is (G->n - 2), so bump it down by 2 below
	    // if(!G->self) numCommon1 -= 2;
	}
    }
    if(G->sparse != true) // ie, if it's 0 or 2
    {
	assert(!G->sparse||G->sparse==both);
	static SET *combined; // for G this is the SetIntersect; for G' it'll be the SetUnion
	if(!combined) { combined = SetAlloc(G->n); SetUseMaxCrossover(combined);}
	SetReset(combined);
	if(G->useComplement) { 
	    SetUnion(combined, G->A[i], G->A[j]);
	    numCommon2 = G->n - SetCardinality(combined);
	}
	else {
	    SetIntersect(combined, G->A[i], G->A[j]);
	    numCommon2 = SetCardinality(combined);
	}
    }
    if(numCommon1 && numCommon2) {
	assert(numCommon1 == numCommon2);
	numCommon2=0;
    }
    assert(numCommon1 == 0 || numCommon2 == 0);
    if(G->useComplement) return numCommon1 + numCommon2 - 2*G->self;
    else return numCommon1 + numCommon2;
}
#endif

#ifndef GraphNumEdges
int GraphNumEdges(GRAPH *G)
{
    int total=0, i;
    for(i=0; i<G->n; i++)
	total += GraphDegree(G,i);
    assert(total % 2 == 0); // should be divisible by 2
    assert(G->m == total/2);
    return G->m;
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

static GRAPH *GraphFromEdgeList(unsigned n, unsigned m, unsigned *pairs, float *weights)
{
    int i;
    GRAPH *G = GraphAlloc(n, false, false, !!weights);
    assert(n == G->n);
    if(G->weight) assert(weights);
    for(i=0; i<m; i++) {
	if(pairs[2*i] == pairs[2*i+1] && !G->self) {
	    static Boolean warned;
	    if(!warned) Warning("GraphFromEdgeList: node %d has a self-loop; assuming they are allowed", pairs[2*i]);
	    warned = G->self = true;
	}
	GraphConnect(G, pairs[2*i], pairs[2*i+1]);
	if(weights) {assert(weights[i]!=0.0); GraphSetWeight(G, pairs[2*i], pairs[2*i+1], weights[i]);}
    }
    return G;
}

unsigned GraphNodeNameToInt(GRAPH *G, char name[]) {
    foint node, ID;
    node.s = name;
    if(!BinTreeLookup(G->nameDict, node, &ID)) Fatal("GraphNodeNameToInt: node name \"%s\" not found", name);
    return ID.ui;
}

// The old one that can read through the list only once
#define MIN_EDGELIST 1024
static GRAPH *GraphReadEdgeListOnePass(FILE *fp, Boolean self, Boolean directed, Boolean weighted)
{
    unsigned numNodes=0, numNodesFirstLine=0;
    unsigned numEdges=0, maxEdges=MIN_EDGELIST; // these will be increased as necessary during reading
    unsigned *pairs = Malloc(2*maxEdges*sizeof(pairs[0]));
    float *fweight = NULL;
    if(weighted) fweight=Malloc(maxEdges*sizeof(fweight[0]));
    const Boolean supportNodeNames=true; // always true now

    // SUPPORT_NODE_NAMES
    unsigned maxNodes=MIN_EDGELIST;
    char **names = NULL;
    BINTREE *nameDict = NULL;
    names = Malloc(maxNodes*sizeof(char*));
    nameDict = BinTreeAlloc((pCmpFcn)strcmp, (pFointCopyFcn)strdup, (pFointFreeFcn)free, NULL, NULL);

    char line[BUFSIZ];
    static Boolean selfWarned;
    while(fgets(line, sizeof(line), fp))
    {
	if(numEdges == 0) { // check to see if first line has a single integer (which would be the number of nodes)
	    char dummy[BUFSIZ];
	    if(sscanf(line, "%u %s ", &numNodesFirstLine, dummy) == 1) // try to get 2 things, see if we get only one integer
		continue; // success, there was an integer, so move on to the next line
	}
	// nuke all whitespace, including DOS carriage returns, from the end of the line
	int len = strlen(line);
	while(isspace(line[len-1])) line[--len]='\0';
	float w;
	assert(numEdges <= maxEdges);
	if(numEdges >= maxEdges)
	{
	    maxEdges = 2*maxEdges-1; // -1 to reduce chance of overflow near 2GB and 4GB.
	    pairs = Realloc(pairs, 2*maxEdges*sizeof(pairs[0]));
	    if(weighted) fweight = Realloc(fweight, maxEdges*sizeof(fweight[0]));
	}
	const char numExpected[2] = {2, 3}, // fmt[][] below has dimensions [supportNames][weighted]
	    *fmt[2][2] = {{"%d%d ", "%d%d%f "}, {"%s%s ", "%s%s%f "}};
	// name and foints are used only if supportNodeNames is true
	union {unsigned ui; int i; char name[BUFSIZ];} v1, v2;
	foint f1, f2;
	if(supportNodeNames)
	{
	    assert(numNodes <= maxNodes);
	    if(numNodes+2 >= maxNodes) // -2 for a bit of extra space
	    {
		maxNodes *=2;
		names = Realloc(names, maxNodes*sizeof(names[0]));
	    }
	}
	// Note: if !supportNodeNames, a binary integer will be written into the name unions
	if(sscanf(line, fmt[supportNodeNames][weighted], v1.name, v2.name, &w) != numExpected[weighted])
	    Fatal("GraphReadEdgeListOnePass: line %d must contain 2 %s%s, but instead is\n%s\n", numEdges+1,
		(supportNodeNames ? "strings":"ints"), (weighted ? " and a weight":""), line);
	if(strcmp(v1.name,v2.name)==0 && !selfWarned) {
	    Warning("GraphReadEdgeListOnePass: line %d has self-loop (%s to itself); assuming they are allowed", numEdges+1, v1.name);
	    Warning("GraphReadEdgeListOnePass: (another warning will appear below from \"GraphFromEdgeList\")");
	    selfWarned = true;
	}
	if(supportNodeNames) {
	    if(!BinTreeLookup(nameDict, (foint)v1.name, &f1))
	    {
		names[numNodes] = Strdup(v1.name);
		f1.ui = numNodes++;
		BinTreeInsert(nameDict, (foint)v1.name, f1);
	    }
	    if(!BinTreeLookup(nameDict, (foint)v2.name, &f2))
	    {
		names[numNodes] = Strdup(v2.name);
		f2.ui = numNodes++;
		BinTreeInsert(nameDict, (foint)v2.name, f2);
	    }
	    v1.ui = f1.ui; v2.ui = f2.ui;
	}
	else {
	    // if !supportNodeNames, fscanf wrote the integers into the char* pointers
	    numNodes = MAX(numNodes, v1.ui);
	    numNodes = MAX(numNodes, v2.ui);
	}
	pairs[2*numEdges] = v1.ui;
	pairs[2*numEdges+1] = v2.ui;
	if(weighted) { assert(w>0.0); fweight[numEdges] = w;}
	if(pairs[2*numEdges] > pairs[2*numEdges+1])
	{
	    unsigned tmp = pairs[2*numEdges];
	    pairs[2*numEdges] = pairs[2*numEdges+1];
	    pairs[2*numEdges+1] = tmp;
	}
	assert(pairs[2*numEdges] < pairs[2*numEdges+1]+selfWarned);
	numEdges++;
    }
    if(supportNodeNames)
    {
	//printf("BINTREE Dictionary Dump\n");
	unsigned i;
	for(i=0; i<numNodes;i++)
	{
	    foint info;
	    if(!BinTreeLookup(nameDict, (foint)names[i], &info))
		Fatal("couldn't find int for name '%s'", names[i]);
	    assert(i == info.ui);
	    //printf("%u is %s which in turn is %u\n", i, names[i], info.ui);
	}
    }
    else
	numNodes++;	// increase it by one since so far it's simply been the biggest integer seen on the input.

    GRAPH *G = GraphFromEdgeList(numNodes, numEdges, pairs, fweight);
    G->nameDict = nameDict;
    G->name = names;
    Free(pairs);
    if(weighted) Free(fweight);
    assert(G->m <= maxEdges && G->m == numEdges);
    GraphSort(G);
    GraphComputeCumDegree(G);
    return G;
}

GRAPH *GraphReadEdgeList(FILE *fp, Boolean self, Boolean directed, Boolean weighted)
{
    if(fseek(fp, 0L, SEEK_END) != 0) // Must use old, one-pass method
	return GraphReadEdgeListOnePass(fp, self, directed, weighted);
    assert(fseek(fp, 0L, SEEK_SET) == 0);

    char line[BUFSIZ], *s;
    unsigned numNodes=0, numEdges=0; // these will be increased as necessary during reading
    static Boolean selfWarned;
    const Boolean supportNodeNames = true;

    BINTREE *nameDict = BinTreeAlloc((pCmpFcn)strcmp, (pFointCopyFcn)strdup, (pFointFreeFcn)free, NULL, NULL);
    const char numExpected[2] = {2, 3}, // fmt[][] below has dimensions [supportNames][weighted]
	*fmt[2][2] = {{"%d%d ", "%d%d%f "}, {"%s%s ", "%s%s%f "}};
    // name and foints are used for supportNodeNames
    union {unsigned ui; int i; char name[BUFSIZ];} v1, v2;
    foint f1, f2;

    // Check to see if the first line has an integer on it, in which case it's the number of nodes
    s=fgets(line, sizeof(line), fp);
    assert(s);
    // Nuke leading whitespace
    while(*s && isspace(*s)) ++s;
    int len = strlen(s), numDigits=0, i;
    // nuke all whitespace, including DOS carriage returns, from the end of the line
    while(isspace(s[len-1])) s[--len]='\0';
    assert(len == strlen(s));
    for(i=0;i<len;i++) if(isdigit(s[i])) ++numDigits;
    if(numDigits == len) {
	numNodes = atol(s); // the first line is the number of nodes
	// Note("Got n=%d from first line", numNodes);
    }
    else {
	do { // read through the file to get the names and number of nodes.
	    // FIRST LINE IS ALREADY READ... copy reading code here, get names + numNodes, then rewind
	    // to connect all the actual node pairs.
	    // Note: if !supportNodeNames, a binary integer will be written into the name unions
	    float w;
	    if(sscanf(line, fmt[supportNodeNames][weighted], v1.name, v2.name, &w) != numExpected[weighted])
		Fatal("GraphReadEdgeList: line %d must contain 2 %s%s, but instead is\n%s\n", numEdges+1,
		    (supportNodeNames ? "strings":"ints"), (weighted ? " and a weight":""), line);
	    if(strcmp(v1.name,v2.name)==0 && !selfWarned) {
		Warning("GraphReadEdgeList: self-loop on line %d (%s); assuming they are allowed", numEdges+1, v1.name);
		Warning("GraphReadEdgeList: (another warning will appear below from \"GraphFromEdgeList\")");
		selfWarned = true;
	    }
	    if(!BinTreeLookup(nameDict, (foint)v1.name, &f1))
	    {
		f1.ui = numNodes++;
		BinTreeInsert(nameDict, (foint)v1.name, f1);
	    }
	    if(!BinTreeLookup(nameDict, (foint)v2.name, &f2))
	    {
		f2.ui = numNodes++;
		BinTreeInsert(nameDict, (foint)v2.name, f2);
	    }
	    ++numEdges;
	} while(fgets(line, sizeof(line), fp));
	// After all that, we finally have numNodes. Go back and read the edges.
	assert(numNodes == nameDict->n);
	if(fseek(fp, 0L, SEEK_SET) != 0) { // rewind(fp);
	    perror("rewind/fseek(0):");
	    Apology("input must be a file on disk, not a pipe");
	}
	// Note("Got n=%d, m=%d from first read-through", numNodes, numEdges);
    }
    // At this point, we definitely have numNodes; numEdges is correct if we read the file, otherwise it's 0

    GRAPH *G = GraphAlloc(numNodes, self, directed, weighted);
    G->name = Calloc(numNodes, sizeof(char*));
    G->nameDict = nameDict;

    int edgeNum=0, nodeNum=0; // use different variables than the first read-through.
    while(fgets(line, sizeof(line), fp))
    {
	// nuke all whitespace, including DOS carriage returns, from the end of the line
	float w;
	len = strlen(line);
	while(isspace(line[len-1])) line[--len]='\0';
	if(sscanf(line, fmt[supportNodeNames][weighted], v1.name, v2.name, &w) != numExpected[weighted])
	    Fatal("GraphReadEdgeList: line %d must contain 2 %s%s, but instead is\n%s\n", edgeNum+1+!numEdges,
		(supportNodeNames ? "strings":"ints"), (weighted ? " and a weight":""), line);
	if(strcmp(v1.name,v2.name)==0 && !selfWarned) {
	    Warning("GraphReadEdgeList: line %d has self-loop (%s to itself); assuming they are allowed", edgeNum+1+!numEdges, v1.name);
	    Warning("GraphReadEdgeList: (another warning will appear below from \"GraphFromEdgeList\")");
	    selfWarned = true;
	}
	Boolean found = BinTreeLookup(nameDict, (foint)v1.name, &f1);
	if(numEdges) assert(found);
	else if(!found) {
	    f1.ui = nodeNum++;
	    BinTreeInsert(nameDict, (foint)v1.name, f1);
	}
	G->name[f1.ui] = Strdup(v1.name);
	found = BinTreeLookup(nameDict, (foint)v2.name, &f2);
	if(numEdges) assert(found);
	else if(!found) {
	    f2.ui = nodeNum++;
	    BinTreeInsert(nameDict, (foint)v2.name, f2);
	}
	G->name[f2.ui] = Strdup(v2.name);
	v1.ui = f1.ui; v2.ui = f2.ui;
	GraphConnect(G, v1.ui, v2.ui);
	++edgeNum;
    }
    // Note("Got m=%d", edgeNum);
    assert(G->m == edgeNum);
    if(numEdges) assert(G->m == numEdges);

    //printf("BINTREE Dictionary Dump\n");
    for(i=0; i<numNodes;i++)
    {
	foint info;
	if(!BinTreeLookup(nameDict, (foint)G->name[i], &info))
	    Fatal("couldn't find int for name '%s'", G->name[i]);
	assert(i == info.ui);
	//printf("%u is %s which in turn is %u\n", i, G->name[i], info.ui);
    }
    GraphSort(G);
    GraphComputeCumDegree(G);
    return G;
}

int GraphNodeName2Int(GRAPH *G, char *name)
{
    foint info;
    if(!BinTreeLookup(G->nameDict, (foint)name, &info))
	Fatal("BinTreeLookup couldn't find an int for name '%s'", name);
    return info.ui;
}


GRAPH *GraphComplement(GRAPH *G)
{
    int i, j;
    GRAPH *Gbar = GraphAlloc(G->n, G->self, G->directed, !!G->weight);
    Gbar->self = G->self;

    assert(Gbar->n == G->n);

    for(i=0; i < G->n; i++) for(j=i+1; j < G->n; j++)
	if(!GraphAreConnected(G, i, j))
	    GraphConnect(Gbar, i, j);
    if(G->self) for(i=0; i < G->n; i++) if(!GraphAreConnected(G,i,i)) GraphConnect(Gbar,i,i);
    return Gbar;
}


GRAPH *GraphUnion(GRAPH *G1, GRAPH *G2)
{
    int i, j, n = G1->n;

    if(G1->n != G2->n)
	return NULL;

    assert(G1->directed == G2->directed);
    assert(G1->self == G2->self);

    GRAPH *dest = GraphAlloc(n, G1->self, G1->directed, !!G1->weight);

    for(i=0; i < n; i++) for(j=i+1; j < n; j++)
	if(GraphAreConnected(G1, i, j) || GraphAreConnected(G2, i, j))
	    GraphConnect(dest, i ,j);
    if(G1->self)
	for(i=0; i < G1->n; i++) if(GraphAreConnected(G1, i, i) || GraphAreConnected(G2, i, i)) GraphConnect(dest, i ,i);
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
	int v = QueueGet(BFSQ).ui;

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
	    for(j=0; j < G->n; j++) if(GraphAreConnected(G, v, j))
	    {
		if(v==j) assert(G->self); // nothing to do; don't add self to a BFS
		else if(distArray[j] == -1)
		{
		    distArray[j] = distArray[v] + 1;
		    QueuePut(BFSQ, (foint)j);
		}
	    }
	}
    }
    QueueFree(BFSQ);
    return count;
}

static Boolean _GraphCCatLeastKHelper(GRAPH *G, SET* visited, int v, int *k);
Boolean GraphCCatLeastK(GRAPH *G, int v, int k) {
    SET* visited = SetAlloc(G->n); SetUseMaxCrossover(visited);
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
    SetAdd(visited, v);
    *k -= 1;
    if (*k <= 0) return true;
    int i;
    Apology("noCCatLeast");
    #if 0
    for (i = 0; i < GraphDegree(G,v); i++) {
        if (G->neighbor[v][i]==v) assert(G->self); // nothing to do, don't add self in a DFS
        else if (!SetIn(visited, G->neighbor[v][i])) {
            Boolean result = _GraphCCatLeastKHelper(G, visited, G->neighbor[v][i], k);
            if (result)
                return result;
        }
    }
    #endif
    return false;
}

/* At top-level call, set (*pn)=0. The visited array does *not* need to be clear, but everything needs to be allocated.
** We return the number of elements in Varray. Not particularly efficient.
*/
int GraphVisitCC(GRAPH *G, unsigned int u, SET *visited, unsigned int *Varray, int *pn)
{
    assert(u < SetMaxSize(visited));
    if(!SetIn(visited,u))
    {
	SetAdd(visited, u);
	Varray[(*pn)++] = u;
	unsigned i=0, v; // i = list element for u's neighbors, v=actual neighbor
	while((v=GraphNextNeighbor(G,u,&i)) < G->n) {
	    assert(GraphAreConnected(G,u,v));
	    if(u==v) assert(G->self);
	    GraphVisitCC(G, v, visited, Varray, pn);
	}
	assert(v==G->n);
    }
    return *pn;
}

/* doesn't allow Gv == G */
GRAPH *GraphInduced(GRAPH *G, SET *V, unsigned arraySize, unsigned *array)
{
    unsigned i,j,nV = SetCardinality(V);
    assert(arraySize >= nV);
    nV = SetToArray(array, V);
    GRAPH *Gv = GraphAlloc(nV, G->self, G->directed, !!G->weight);
    for(i=0; i < nV; i++) for(j=i+1; j < nV; j++)
	if(GraphAreConnected(G, array[i], array[j]))
	    GraphConnect(Gv, i, j);
    if(G->self) for(i=0; i < nV; i++) if(GraphAreConnected(G, array[i], array[i])) GraphConnect(Gv, i, i);
    return Gv;
}

/* allows Gv == G */
GRAPH *GraphInduced_NoVertexDelete(GRAPH *G, SET *V)
{
    unsigned array[G->n], nV = SetToArray(array, V), i, j;
    GRAPH *Gv = GraphAlloc(G->n, G->self, G->directed, !!G->weight);
    Gv->self = G->self;

    for(i=0; i < nV; i++) for(j=i+1; j < nV; j++)
	if(GraphAreConnected(G, array[i], array[j]))
	    GraphConnect(Gv, array[i], array[j]);
    if(G->self) for(i=0; i < nV; i++) if(GraphAreConnected(G, array[i], array[i])) GraphConnect(Gv, array[i], array[i]);
    return Gv;
}

#if 0
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
    int numIntersect;
    SET *C = SetAlloc(G->n); SetUseMaxCrossover(C);
    assert(!G->sparse||G->sparse==both);
    SetIntersect(C, G->A[i], G->A[j]);
    numIntersect = SetCardinality(C);
    SetFree(C);
    return numIntersect != 0;
}

Boolean GraphContainsK3(GRAPH *G)
{
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

    assert(!c->G->sparse||c->G->sparse==both);
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
    int i, nDegk = G->n;
    SET *setDegk;
    CLIQUE *c;

    assert(k <= G->n);
    if(k == 0)
	return NULL;

    setDegk = SetAlloc(G->n); SetUseMaxCrossover(setDegk);
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
	    if(GraphDegree(c->G,i) >= k-1)
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
    GRAPH *Gbar = GraphComplement(G);
    CLIQUE *c = GraphKnFirst(Gbar, n);
    GraphFree(Gbar);
    return c;
}


Boolean GraphKnContains(GRAPH *G, int n)
{
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
    int i, j;
    for(i=0; i<n; i++)
	if(GraphDegree(isoG1,i) != GraphDegree(isoG2,perm[i]))
	    return false;

    for(i=0; i<n; i++) for(j=i+1; j<n; j++)
	/* The !GraphAreConnected is just to turn a bitstring into a boolean */
	if(!GraphAreConnected(isoG1, i,j) !=
	    !GraphAreConnected(isoG2, perm[i], perm[j]))
	    return false;   /* non-isomorphic */
    // Note they don't both have to ALLOW self loops, but if one does, then both need to have or not the same loops
    if(isoG1->self || isoG2->self) for(i=0; i<n; i++)
       if(!GraphAreConnected(isoG1, i,i) != !GraphAreConnected(isoG2, perm[i], perm[i])) return false;   /* non-isomorphic */
    return true;   /* isomorphic! */
}

Boolean GraphsIsomorphic(int *perm, GRAPH *G1, GRAPH *G2)
{
    static int recursionDepth;
    ++recursionDepth;
    assert(recursionDepth <= G1->n + 1);
    Boolean self = (G1->self || G2->self);
    int i, n = G1->n, degreeCount1[n+self], degreeCount2[n+self];

    SET *degreeOnce;

    /*
    ** First some simple tests.
    */
    if(G1->n != G2->n) {--recursionDepth; return false;}

    if(n < 2) {--recursionDepth; return true;}

    /*
    ** Ensure each degree occurs the same number of times in each... and the count can be == n if self is true
    */
    for(i=0; i<n+self; i++) degreeCount1[i] = degreeCount2[i] = 0;
    for(i=0; i<n; i++) {
	++degreeCount1[GraphDegree(G1,i)];
	++degreeCount2[GraphDegree(G2,i)];
    }
    for(i=0; i<n+self; i++) if(degreeCount1[i] != degreeCount2[i]) {--recursionDepth; return false;}

    /*
    ** Let degree d appear only once.  Then there is exactly one vertex
    ** v1 in G1 with degree d, and exactly one vertex v2 in G2 with degree d.
    ** G1 and G2 are isomorphic only if the neighborhood of v1 is isomorphic
    ** to the neighborhood of v2.
    */
    degreeOnce = SetAlloc(n+self); SetUseMaxCrossover(degreeOnce);
    for(i=0; i<n+self; i++) if(degreeCount1[i] == 1) SetAdd(degreeOnce, i);
    for(i=0; i<n+self; i++)
    {
	/* Find out if the degree of vertex i in G1 appears only once */
	if(SetIn(degreeOnce, GraphDegree(G1,i)))
	{
	    int j, degree = GraphDegree(G1,i);
	    GRAPH *neighG1i, *neighG2j;

	    /* find the (unique) vertex in G2 that has the same degree */
	    for(j=0; j < n; j++) if(GraphDegree(G2,j) == degree) break;
	    assert(j < n);

            // remove self-loops from the set to induce on...
            SET *G1Ai = SetCopy(NULL, G1->A[i]), *G2Aj = SetCopy(NULL, G2->A[j]);
	    unsigned nMax = MAX(G1->n,G2->n), array[nMax]; // needed just as placeholders
            SetDelete(G1Ai, i); neighG1i = GraphInduced(G1, G1->A[i], nMax, array);
            SetDelete(G2Aj, j); neighG2j = GraphInduced(G2, G2->A[j], nMax, array);

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
#endif // clique and combin stuff
#ifdef __cplusplus
} // end extern "C"
#endif
