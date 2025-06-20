#include <stdio.h>
#include "misc.h"
#include "tinygraph.h"

static int _testNum;

int main(int argc, char *argv[])
{
    int BFSsize, i, j, n, testNum;
    TINY_GRAPH *G, *Complete, *GG, *GGC, *U,*Gbar, *U2;
    srand48(time(NULL));
    for(testNum=0;testNum<10000;testNum++)
    {
		n = 6 + lrand48() % 3; // ensure G->n is at least 2
		G = TinyGraphAlloc(n,(lrand48()%2==0),(lrand48()%2==0));
		GG = TinyGraphComplement(NULL,G);
		Complete = TinyGraphCopy(NULL,GG);
		assert(n==G->n);
		assert(testNum == _testNum++);
		printf("#%d n %d self %d directed %d\n", testNum, G->n, G->selfLoops, G->directed); fflush(stdout);

		for(int k=2*G->n; k>=0; k--)
		{
			i = lrand48() % G->n;
			j = lrand48() % G->n;
			if(!G->selfLoops) while(j==i) j = lrand48() % G->n;
			TinyGraphConnect(G,i,j);
			TinyGraphDisconnect(GG,i,j);
		}
		for(int i=0;i<10;i++)
		{
			int x=lrand48()%n, y=lrand48()%n;
			TinyGraphSwapNodes(G,x,y);
			TinyGraphSwapNodes(G,y,x);
		}
		Gbar = TinyGraphComplement(NULL, G);
		GGC = TinyGraphCopy(NULL, GG);
		assert(GGC->selfLoops == G->selfLoops);
		assert(Gbar->selfLoops == G->selfLoops);
		for(i=0; i<G->n; i++)
			assert(Gbar->A[i] == GGC->A[i]);
		U = TinyGraphUnion(NULL,GGC,Gbar);
		U2 = TinyGraphUnion(NULL,GGC,G);
		assert(U->selfLoops == G->selfLoops);
		assert(U2->selfLoops == G->selfLoops);
		assert(Complete->selfLoops == G->selfLoops);
		for(i=0; i<G->n; i++)
			assert(Gbar->A[i] == U->A[i]);
			assert(Complete->A[i] == U2->A[i]);
    }
    printf("DONE!\nGraph for BFS (selfLoops: %d, directed: %d)\n",G->selfLoops,G->directed);
	TinyGraphPrintAdjMatrix(stderr,G);
    while(!feof(stdin))
    {
	int root, distance, nodeArray[MAX_TSET], distArray[MAX_TSET];
	printf("enter BFS start node, distance: "); fflush(stdout);
	if(scanf("%d%d", &root, &distance) != 2)
		 break;
	BFSsize = TinyGraphBFS(G, root, distance, nodeArray, distArray);
	for(i=0; i<BFSsize; i++)
	    printf("%d(%d) ", nodeArray[i], distArray[nodeArray[i]]);
	printf("\n");
    }
	return 0;
}
