// C baseline: serial pointer chase — 1M dependent hops over a shuffled
// index array. Pure serial latency workload; no parallelism exists.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define K 65536
static unsigned long long steps;
int main(void) {
    uint32_t *next = malloc(K * sizeof(uint32_t));
    for (uint32_t i = 0; i < K; i++) next[i] = (i * 48271 + 1) % K;
    uint32_t p = 0;
    for (int i = 0; i < 1000000; i++) { p = next[p]; steps++; }
    printf("%u steps=%llu\n", p, steps);
    return 0;
}
