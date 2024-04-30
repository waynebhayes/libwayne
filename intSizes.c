#include <stdio.h>
#include <string.h>
#include <ctype.h>

char *space2line(const char *S) {
    char *s=strdup(S), *t=s;
    while(*s) {if(isspace(*s)) *s='_'; ++s;}
    return t;
}

#define PrintSz(x) printf("#define sizeof_%s %zu\n#define %s_width %zu\n", space2line(#x), sizeof(x), space2line(#x), 8*sizeof(x))

int main(void) {
    unsigned char c=1, nBits=1;
    while(c <<= 1) ++nBits;
    printf("#define BITS_PER_CHAR %d\n", nBits);
    PrintSz(char);
    PrintSz(short);
    PrintSz(int);
    PrintSz(long);
    PrintSz(long long);
#if __GLIBCXX_TYPE_INT_N_0 == 128
    PrintSz(__int128);
#endif
    return 0;
}
