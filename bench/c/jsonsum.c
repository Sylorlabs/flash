// C baseline: serial record scan — walk 512K records, sum a field.
// Memory/serial-bound; no parallelism exists.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define K 524288
static unsigned long long steps;
int main(void) {
    uint32_t *rec = malloc(K * sizeof(uint32_t));
    for (uint32_t i = 0; i < K; i++) rec[i] = (i * 31 + 7) % 251;
    uint64_t sum = 0;
    for (int i = 0; i < K; i++) { sum += rec[i]; steps++; }
    printf("%llu steps=%llu\n", (unsigned long long)sum, steps);
    return 0;
}
