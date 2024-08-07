#include <stdio.h>
#include <stdlib.h>

char *printBits(char out[BUFSIZ], int k)
{
    unsigned char *b = (unsigned char*) &k;
    unsigned char byte;
    int i, j, l=0, start=0;

    if(k==0) {out[0]='0'; out[1]='\0';}
    else {
	for (i = sizeof(int)-1; i >= 0; i--) {
	    for (j = 7; j >= 0; j--) {
		byte = (b[i] >> j) & 1;
		if(byte) start=1;
		if(start) sprintf(out+(l++), "%u", byte);
	    }
	}
    }
    return out;
}

int main(int argc, char **argv)
{
    char buf[BUFSIZ];
    int i,n;
    n=atoi(argv[1]);
    for(i=0; i<n; i++) puts(printBits(buf, i));
}
