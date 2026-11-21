// C baseline: 64x64 matvec, M=64 vectors — scalar multiply-accumulate.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define N 64
#define M 64
static unsigned long long steps;
int main(void) {
    int32_t *A = malloc(N * N * sizeof(int32_t));
    int32_t *x = malloc(N * sizeof(int32_t));
    int32_t *y = malloc(N * sizeof(int32_t));
    uint64_t checksum = 0;
    for (int i = 0; i < N * N; i++) A[i] = (i * 7 + 3) % 97;
    for (int i = 0; i < N; i++) x[i] = (i * 5 + 1) % 89;
    for (int m = 0; m < M; m++) {
        for (int i = 0; i < N; i++) {
            int32_t s = 0;
            for (int j = 0; j < N; j++) { s += A[i * N + j] * x[j]; steps++; }
            y[i] = s;
        }
        checksum += y[0];
    }
    printf("%llu steps=%llu\n", (unsigned long long)checksum, steps);
    return 0;
}
