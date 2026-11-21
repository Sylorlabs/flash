// C baseline: "routing" workload = serial coordinate sweeps, each cell
// visited once — stands in for lattice occupancy marking. 256K cells.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define K 262144
static unsigned long long steps;
int main(void) {
    uint8_t *lat = calloc(K, 1);
    uint64_t checksum = 0;
    for (int r = 0; r < 64; r++) {
        for (int i = 0; i < K; i++) {
            lat[i] = 1;
            checksum += lat[i] & 1; steps++;
        }
    }
    printf("%llu steps=%llu\n", (unsigned long long)checksum, steps);
    return 0;
}
