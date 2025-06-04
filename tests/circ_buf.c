#include "misc.h"
#include "circ_buf.h"

#define N 10

int main() {
    CIRC_BUF *c = CircBufAlloc(N);
    double val;
    while(scanf("%lf", &val)==1) {
	CircBufAdd(c, val);
	printf("%g\n", CircBufMean(c));
    }
}
