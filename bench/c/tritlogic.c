// C baseline: wide trit logic — 1M digit-parallel NAND/consensus ops,
// emulating trits in binary (two binary ops per trit op).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define K 1000000
static unsigned long long steps;
int main(void) {
    int8_t *a = malloc(K), *b = malloc(K), *d = malloc(K);
    uint64_t checksum = 0;
    for (int i = 0; i < K; i++) { a[i] = (i % 3) - 1; b[i] = ((i * 7) % 3) - 1; }
    for (int i = 0; i < K; i++) {
        int8_t mn = a[i] < b[i] ? a[i] : b[i];
        d[i] = -mn; steps++;                // ternary NAND: -(min(a,b))
        if (d[i] == checksum % 3) checksum += d[i];
    }
    printf("%llu steps=%llu\n", (unsigned long long)checksum, steps);
    return 0;
}
