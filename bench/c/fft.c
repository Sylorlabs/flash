// C baseline: radix-2 DFT via direct O(N^2) transform, N=1024, R=64 runs.
// (FFTs are O(N log N) but a DFT is the honest element-op baseline the
// photonic lens competes with.)
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#define N 1024
#define R 64
static unsigned long long steps;
int main(void) {
    double xr[N], xi[N], yr[N], yi[N];
    uint64_t checksum = 0;
    for (int i = 0; i < N; i++) { xr[i] = (i % 97) / 97.0; xi[i] = 0; }
    for (int r = 0; r < R; r++) {
        for (int k = 0; k < N; k++) {
            double sr = 0, si = 0;
            for (int n = 0; n < N; n++) {
                double ang = -2.0 * 3.14159265358979 * k * n / N;
                sr += xr[n] * cos(ang) - xi[n] * sin(ang);
                si += xr[n] * sin(ang) + xi[n] * cos(ang);
                steps += 2;
            }
            yr[k] = sr; yi[k] = si;
        }
        checksum += (uint64_t)(yr[0] * 1000);
    }
    printf("%llu steps=%llu\n", (unsigned long long)checksum, steps);
    return 0;
}
