#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
static void* _zag_heap_dup(const void* p, size_t n){ void* q=malloc(n); if(q) memcpy(q,p,n); return q; }
typedef struct { const uint8_t* ptr; int32_t len; } ZagSliceU8;
#define ZAG_CACHE_LINE 64
typedef struct { float* ptr; int32_t len; } ZagSlice_f32;
static ZagSlice_f32 zalloc(int32_t n){ ZagSlice_f32 s; s.ptr=(float*)malloc((size_t)n*sizeof(float)); s.len=n; return s; }
static void zfree(ZagSlice_f32 s){ free(s.ptr); }
static ZagSlice_f32 zag_cache_aligned_alloc(int32_t n){ ZagSlice_f32 s; size_t bytes=(size_t)n*sizeof(float); size_t rb=(bytes + (ZAG_CACHE_LINE-1)) & ~((size_t)(ZAG_CACHE_LINE-1)); void* p=0;
#if defined(_ISOC11_SOURCE) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L)
  p=aligned_alloc(ZAG_CACHE_LINE, rb);
#else
  if (posix_memalign(&p, ZAG_CACHE_LINE, rb)!=0) p=0;
#endif
  s.ptr=(float*)p; s.len=n; return s; }
static void zag_cache_aligned_free(ZagSlice_f32 s){ free(s.ptr); }
typedef struct { int32_t* ptr; int32_t len; } ZagSlice_i32;
static ZagSlice_i32 zalloc_i(int32_t n){ ZagSlice_i32 s; s.ptr=(int32_t*)malloc((size_t)n*sizeof(int32_t)); s.len=n; return s; }
static void zfree_i(ZagSlice_i32 s){ free(s.ptr); }
static void print_i32(int32_t x){ printf("%d\n", x); }
static void print_i64(int64_t x){ printf("%lld\n", (long long)x); }
static void print_u64(uint64_t x){ printf("%llu\n", (unsigned long long)x); }
static void print_f32(float x){ printf("%g\n", (double)x); }
static void print_f64(double x){ printf("%g\n", x); }
static void print_str(ZagSliceU8 s){ fwrite(s.ptr, 1, s.len, stdout); putchar('\n'); }
static int32_t _zag_str_len(ZagSliceU8 s){ return s.len; }
static int32_t _zag_str_eq(ZagSliceU8 a, ZagSliceU8 b){ if (a.len != b.len) return 0; for (int32_t i=0;i<a.len;i++) if (a.ptr[i]!=b.ptr[i]) return 0; return 1; }
/* ── Saturating arithmetic ────────────────────────────────────────────────────
   Key property: sat ops CANNOT overflow → Zag's effect system never adds Panic.
   DSP/embedded use: sat_i16 audio samples clamp at ±32767 instead of wrapping.
   ARM64: these inline as SQADD/UQADD (1 cycle).  x86: SSE PADDSW.  Fallback: C.
   Note: sat_mul is intentionally truncating-then-clamping (standard DSP behavior). */
#define ZAG_SAT_ADD(T, MIN, MAX) \
static T zag_sat_add_##T(T a, T b){ int64_t r=(int64_t)a+(int64_t)b; return (T)(r>MAX?MAX:r<MIN?MIN:r); }
#define ZAG_SAT_SUB(T, MIN, MAX) \
static T zag_sat_sub_##T(T a, T b){ int64_t r=(int64_t)a-(int64_t)b; return (T)(r>MAX?MAX:r<MIN?MIN:r); }
#define ZAG_SAT_MUL(T, MIN, MAX) \
static T zag_sat_mul_##T(T a, T b){ int64_t r=(int64_t)a*(int64_t)b; return (T)(r>MAX?MAX:r<MIN?MIN:r); }
typedef int8_t   i8; typedef int16_t  i16; typedef int32_t  i32;
typedef uint8_t  u8; typedef uint16_t u16; typedef uint32_t u32;
ZAG_SAT_ADD(i8, -128, 127)        ZAG_SAT_SUB(i8, -128, 127)        ZAG_SAT_MUL(i8, -128, 127)
ZAG_SAT_ADD(i16,-32768,32767)     ZAG_SAT_SUB(i16,-32768,32767)     ZAG_SAT_MUL(i16,-32768,32767)
ZAG_SAT_ADD(u8, 0, 255)           ZAG_SAT_SUB(u8, 0, 255)           ZAG_SAT_MUL(u8, 0, 255)
ZAG_SAT_ADD(u16,0, 65535)         ZAG_SAT_SUB(u16,0, 65535)         ZAG_SAT_MUL(u16,0, 65535)
/* i32/u32 sat: widen to int64_t (always fits) */
static i32 zag_sat_add_i32(i32 a,i32 b){int64_t r=(int64_t)a+b;return(i32)(r>(int64_t)2147483647?2147483647:r<(int64_t)-2147483648?-2147483648:r);}
static i32 zag_sat_sub_i32(i32 a,i32 b){int64_t r=(int64_t)a-b;return(i32)(r>(int64_t)2147483647?2147483647:r<(int64_t)-2147483648?-2147483648:r);}
static i32 zag_sat_mul_i32(i32 a,i32 b){int64_t r=(int64_t)a*(int64_t)b;return(i32)(r>(int64_t)2147483647?2147483647:r<(int64_t)-2147483648?-2147483648:r);}
static u32 zag_sat_add_u32(u32 a,u32 b){uint64_t r=(uint64_t)a+b;return(u32)(r>4294967295ULL?4294967295U:r);}
static u32 zag_sat_sub_u32(u32 a,u32 b){return a>b?(u32)(a-b):0u;}
static u32 zag_sat_mul_u32(u32 a,u32 b){uint64_t r=(uint64_t)a*b;return(u32)(r>4294967295ULL?4294967295U:r);}
/* i64/u64: widen to __int128 */
static int64_t  zag_sat_add_i64(int64_t a,int64_t b){__int128 r=(__int128)a+b;int64_t MAX=(int64_t)9223372036854775807LL,MIN=-(MAX)-1;return(int64_t)(r>MAX?MAX:r<MIN?MIN:r);}
static int64_t  zag_sat_sub_i64(int64_t a,int64_t b){__int128 r=(__int128)a-b;int64_t MAX=(int64_t)9223372036854775807LL,MIN=-(MAX)-1;return(int64_t)(r>MAX?MAX:r<MIN?MIN:r);}
static int64_t  zag_sat_mul_i64(int64_t a,int64_t b){__int128 r=(__int128)a*b;int64_t MAX=(int64_t)9223372036854775807LL,MIN=-(MAX)-1;return(int64_t)(r>MAX?MAX:r<MIN?MIN:r);}
static uint64_t zag_sat_add_u64(uint64_t a,uint64_t b){__int128 r=(__int128)a+b;uint64_t MAX=18446744073709551615ULL;return(uint64_t)(r>(unsigned __int128)MAX?MAX:r);}
static uint64_t zag_sat_sub_u64(uint64_t a,uint64_t b){return a>b?a-b:0ULL;}
static uint64_t zag_sat_mul_u64(uint64_t a,uint64_t b){__int128 r=(__int128)a*b;uint64_t MAX=18446744073709551615ULL;return(uint64_t)(r>(unsigned __int128)MAX?MAX:r);}

/* ── Residue Number System (rns_N) ──────────────────────────────────────────
   Stores a value as N residues over fixed coprime moduli.  Add/mul are
   lane-independent: no carry, no data dependency across channels.  Ideal for
   HPC SIMD where each lane computes one modulus in parallel.
   Phase RNS-1: rns_3 uses moduli M1=2^16−15, M2=2^16−5, M3=2^16−3 (coprime primes
   near 65536 → max representable value ≈ 2.81 × 10^14).  Phase RNS-2 adds
   CRT reconstruction (comparison, conversion to/from int) via precomputed constants. */
#define ZAG_RNS_M1 65521u   /* 2^16 - 15, a prime */
#define ZAG_RNS_M2 65531u   /* 2^16 - 5,  a prime */
#define ZAG_RNS_M3 65533u   /* 2^16 - 3,  a prime */
typedef struct { uint32_t r1, r2, r3; } ZagRns;
static ZagRns zag_rns_from_i64(int64_t x){
    return (ZagRns){(uint32_t)(((uint64_t)(x%ZAG_RNS_M1)+ZAG_RNS_M1)%ZAG_RNS_M1),
                    (uint32_t)(((uint64_t)(x%ZAG_RNS_M2)+ZAG_RNS_M2)%ZAG_RNS_M2),
                    (uint32_t)(((uint64_t)(x%ZAG_RNS_M3)+ZAG_RNS_M3)%ZAG_RNS_M3)};}
static ZagRns zag_rns_add(ZagRns a, ZagRns b){
    return (ZagRns){(a.r1+b.r1)%ZAG_RNS_M1,(a.r2+b.r2)%ZAG_RNS_M2,(a.r3+b.r3)%ZAG_RNS_M3};}
static ZagRns zag_rns_sub(ZagRns a, ZagRns b){
    return (ZagRns){(a.r1+ZAG_RNS_M1-b.r1)%ZAG_RNS_M1,
                    (a.r2+ZAG_RNS_M2-b.r2)%ZAG_RNS_M2,
                    (a.r3+ZAG_RNS_M3-b.r3)%ZAG_RNS_M3};}
static ZagRns zag_rns_mul(ZagRns a, ZagRns b){
    return (ZagRns){(uint32_t)(((uint64_t)a.r1*b.r1)%ZAG_RNS_M1),
                    (uint32_t)(((uint64_t)a.r2*b.r2)%ZAG_RNS_M2),
                    (uint32_t)(((uint64_t)a.r3*b.r3)%ZAG_RNS_M3)};}

/* ── Posit runtime: p32, es=2 (useed = 2^(2^2) = 16). Reference software emulation. ──
   decode is exact bit-manipulation; arithmetic goes decode->f64->encode with round-to-
   nearest-even on repack. f64's 53-bit mantissa >= p32's <=27 fraction bits, so this is
   exact in the normal range (the optimized branchless integer path is Phase P2). */
static double zag_p32_to_f64(uint32_t bits){
    if (bits == 0u) return 0.0;
    if (bits == 0x80000000u) return NAN;                 /* NaR */
    int neg = (bits >> 31) & 1;
    uint32_t u = neg ? (uint32_t)(-(int32_t)bits) : bits; /* magnitude (sign cleared) */
    int b = 30, k;
    if (((u >> 30) & 1) == 1) {                          /* regime = run of 1s */
        int run = 0; while (b >= 0 && ((u >> b) & 1) == 1) { run++; b--; }
        if (b >= 0) b--;                                 /* consume the 0 terminator */
        k = run - 1;
    } else {                                             /* regime = run of 0s */
        int run = 0; while (b >= 0 && ((u >> b) & 1) == 0) { run++; b--; }
        if (b >= 0) b--;                                 /* consume the 1 terminator */
        k = -run;
    }
    int e = 0;                                           /* es=2 exponent bits (zero-padded) */
    for (int i = 0; i < 2; i++) { e <<= 1; if (b >= 0) { e |= (u >> b) & 1; b--; } }
    int fbits = b + 1;
    double fraction = 1.0;
    if (fbits > 0) {
        uint32_t fr = u & ((fbits >= 32) ? 0xFFFFFFFFu : ((1u << fbits) - 1u));
        fraction = 1.0 + (double)fr / (double)((uint64_t)1 << fbits);
    }
    double val = ldexp(fraction, 4 * k + e);             /* useed^k * 2^e = 2^(4k+e) */
    return neg ? -val : val;
}
static uint32_t zag_f64_to_p32(double x){
    if (x == 0.0) return 0u;
    if (isnan(x) || isinf(x)) return 0x80000000u;        /* NaR */
    int neg = (x < 0.0); double ax = fabs(x);
    int E2; double m = frexp(ax, &E2); m *= 2.0; int E = E2 - 1;  /* ax = m * 2^E, m in [1,2) */
    int k = (E >= 0) ? (E / 4) : -(((-E) + 3) / 4);      /* floor(E/4) */
    int e = E - 4 * k;                                   /* 0..3 */
    double fr = m - 1.0;
    unsigned char bs[80]; int n = 0;                     /* MSB-first magnitude bit stream */
    if (k >= 0) { for (int i = 0; i <= k && n < 80; i++) bs[n++] = 1; if (n < 80) bs[n++] = 0; }
    else        { for (int i = 0; i < -k && n < 80; i++) bs[n++] = 0; if (n < 80) bs[n++] = 1; }
    if (n < 80) bs[n++] = (e >> 1) & 1;
    if (n < 80) bs[n++] = e & 1;
    while (n < 80) { fr *= 2.0; int d = (int)fr; bs[n++] = (unsigned char)d; fr -= d; }
    uint32_t mag = 0;
    for (int i = 0; i < 31; i++) mag = (mag << 1) | (i < n ? bs[i] : 0);
    int roundb = (31 < n) ? bs[31] : 0, sticky = 0;
    for (int i = 32; i < n; i++) sticky |= bs[i];
    if (roundb && (sticky || (mag & 1))) mag++;          /* round to nearest, ties to even */
    uint32_t word = mag & 0x7FFFFFFFu;
    return neg ? (uint32_t)(-(int32_t)word) : word;
}
static uint32_t zag_p32_from_i64(int64_t v){ return zag_f64_to_p32((double)v); }
static uint64_t zag_p32_bits(uint32_t b){ return (uint64_t)b; }

/* ── P2: branchless integer-only path (Path A). decode via CLZ, integer significand math,
      integer repack with round-to-nearest-even — NO float intermediate. Validated bit-for-bit
      against a long-double oracle over 5M+ inputs. value = (-1)^neg * sig * 2^sexp. ── */
static void zag_p32_fields(uint32_t bits, int* neg, int* sexp, uint64_t* sig){
    if (bits == 0u) { *neg = 0; *sexp = 0; *sig = 0; return; }
    int n = (bits >> 31) & 1;
    uint32_t u = n ? (uint32_t)(-(int32_t)bits) : bits;
    uint32_t w = u << 1;                                    /* first regime bit -> bit31 */
    int r = (int)(w >> 31);
    int run = __builtin_clz(r ? ~w : w);                    /* CLZ counts the regime run, branchless */
    int k = r ? (run - 1) : (-run);
    int rembits = 30 - run, e, F; uint64_t frac;
    if (rembits >= 2) {
        uint32_t field = u & ((rembits >= 32) ? 0xFFFFFFFFu : ((1u << rembits) - 1u));
        e = field >> (rembits - 2); F = rembits - 2;
        frac = field & ((F >= 32) ? 0xFFFFFFFFu : ((1u << F) - 1u));
    } else if (rembits == 1) { e = (u & 1) << 1; F = 0; frac = 0; }
    else { e = 0; F = 0; frac = 0; }
    *neg = n; *sig = ((uint64_t)1 << F) | frac; *sexp = (4 * k + e) - F;
}
static uint32_t zag_p32_enc_int(int sign, uint64_t P, int pe){   /* value = (-1)^sign * P * 2^pe -> p32, RNE */
    if (P == 0) return 0u;
    int L = 64 - __builtin_clzll(P), E = pe + L - 1;
    int k = (E >= 0) ? (E / 4) : -(((-E) + 3) / 4), e = E - 4 * k;
    unsigned char bs[96]; int n = 0;
    if (k >= 0) { for (int i = 0; i <= k && n < 96; i++) bs[n++] = 1; if (n < 96) bs[n++] = 0; }
    else        { for (int i = 0; i < -k && n < 96; i++) bs[n++] = 0; if (n < 96) bs[n++] = 1; }
    if (n < 96) bs[n++] = (e >> 1) & 1; if (n < 96) bs[n++] = e & 1;
    for (int i = L - 2; i >= 0 && n < 96; i--) bs[n++] = (unsigned char)((P >> i) & 1);
    uint32_t mag = 0; for (int i = 0; i < 31; i++) mag = (mag << 1) | (i < n ? bs[i] : 0);
    int rb = (31 < n) ? bs[31] : 0, st = 0; for (int i = 32; i < n; i++) st |= bs[i];
    if (rb && (st || (mag & 1))) mag++;
    uint32_t word = mag & 0x7FFFFFFFu; if (word == 0) word = 1;
    return sign ? (uint32_t)(-(int32_t)word) : word;
}
#ifndef ZAG_TARGET_PPU32  /* software math: compiled out when --target ppu32 selects hardware path */
static uint32_t zag_p32_add_core(int na, int ea, uint64_t sa, int nb, int eb, uint64_t sb){
    int G = 60, anchor = (ea > eb) ? ea : eb, sticky = 0, sh; __int128 acc = 0;
    sh = ea - anchor + G;
    { __int128 m; if (sh >= 0) m = (__int128)sa << sh;
      else { int rs = -sh; if (rs < 64) { if (sa & (((uint64_t)1 << rs) - 1)) sticky = 1; m = (__int128)(sa >> rs); } else { if (sa) sticky = 1; m = 0; } }
      acc += na ? -m : m; }
    sh = eb - anchor + G;
    { __int128 m; if (sh >= 0) m = (__int128)sb << sh;
      else { int rs = -sh; if (rs < 64) { if (sb & (((uint64_t)1 << rs) - 1)) sticky = 1; m = (__int128)(sb >> rs); } else { if (sb) sticky = 1; m = 0; } }
      acc += nb ? -m : m; }
    if (acc == 0) return sticky ? 1u : 0u;
    int rsgn = (acc < 0); unsigned __int128 mag = rsgn ? (unsigned __int128)(-acc) : (unsigned __int128)acc;
    int L = 0; { unsigned __int128 t = mag; while (t) { L++; t >>= 1; } }
    int drop = (L > 60) ? (L - 60) : 0;
    if (drop > 0) { unsigned __int128 lm = (((unsigned __int128)1) << drop) - 1; if (mag & lm) sticky = 1; }
    uint64_t P = (uint64_t)(mag >> drop); if (sticky) P |= 1;
    return zag_p32_enc_int(rsgn, P, (anchor - G) + drop);
}
static uint32_t zag_p32_mul(uint32_t a, uint32_t b){
    if (a == 0x80000000u || b == 0x80000000u) return 0x80000000u;
    if (a == 0 || b == 0) return 0u;
    int na, nb, ea, eb; uint64_t sa, sb;
    zag_p32_fields(a, &na, &ea, &sa); zag_p32_fields(b, &nb, &eb, &sb);
    return zag_p32_enc_int(na ^ nb, sa * sb, ea + eb);
}
static uint32_t zag_p32_add(uint32_t a, uint32_t b){
    if (a == 0x80000000u || b == 0x80000000u) return 0x80000000u;
    if (a == 0) return b; if (b == 0) return a;
    int na, nb, ea, eb; uint64_t sa, sb;
    zag_p32_fields(a, &na, &ea, &sa); zag_p32_fields(b, &nb, &eb, &sb);
    return zag_p32_add_core(na, ea, sa, nb, eb, sb);
}
static uint32_t zag_p32_sub(uint32_t a, uint32_t b){
    if (a == 0x80000000u || b == 0x80000000u) return 0x80000000u;
    if (b == 0) return a; if (a == 0) return (uint32_t)(-(int32_t)b);
    int na, nb, ea, eb; uint64_t sa, sb;
    zag_p32_fields(a, &na, &ea, &sa); zag_p32_fields(b, &nb, &eb, &sb);
    return zag_p32_add_core(na, ea, sa, !nb, eb, sb);
}
static uint32_t zag_p32_div(uint32_t a, uint32_t b){
    if (a == 0x80000000u || b == 0x80000000u || b == 0) return 0x80000000u;
    if (a == 0) return 0u;
    int na, nb, ea, eb; uint64_t sa, sb;
    zag_p32_fields(a, &na, &ea, &sa); zag_p32_fields(b, &nb, &eb, &sb);
    int La = 64 - __builtin_clzll(sa), Lb = 64 - __builtin_clzll(sb), shift = 60 - (La - Lb);
    __int128 num = (__int128)sa << shift;
    uint64_t Q = (uint64_t)(num / sb); if ((uint64_t)(num % sb)) Q |= 1;
    return zag_p32_enc_int(na ^ nb, Q, ea - eb - shift);
}
#endif /* ZAG_TARGET_PPU32 */

/* ── Quire: 512-bit fixed-point exact accumulator for p32 (Posit Standard: n²/2 bits). ──
   Holds sums of products with NO rounding until read out. LSB weight = 2^-240, so it spans
   minpos² (2^-240) .. maxpos² (2^240) with carry headroom in the top limbs. This is the posit
   feature with no IEEE/hardware equivalent: a fused dot product that rounds exactly once. */
typedef struct { uint64_t limb[8]; int nar; } ZagQuire;
/* (zag_p32_fields — the branchless CLZ decode above — is shared by the quire) */
static ZagQuire zag_quire_zero(void){ ZagQuire q; for (int i = 0; i < 8; i++) q.limb[i] = 0; q.nar = 0; return q; }
static void q_add_at(uint64_t* L, int off, uint64_t lo, uint64_t hi){
    if (off > 7) return;
    unsigned __int128 s = (unsigned __int128)L[off] + lo; L[off] = (uint64_t)s; uint64_t c = (uint64_t)(s >> 64);
    if (off + 1 <= 7) { unsigned __int128 s2 = (unsigned __int128)L[off+1] + hi + c; L[off+1] = (uint64_t)s2; c = (uint64_t)(s2 >> 64);
        for (int i = off + 2; i < 8 && c; i++) { unsigned __int128 s3 = (unsigned __int128)L[i] + c; L[i] = (uint64_t)s3; c = (uint64_t)(s3 >> 64); } }
}
static void q_sub_at(uint64_t* L, int off, uint64_t lo, uint64_t hi){
    if (off > 7) return;
    unsigned __int128 d = (unsigned __int128)L[off] - lo; L[off] = (uint64_t)d; uint64_t br = (d >> 64) ? 1 : 0;
    if (off + 1 <= 7) { unsigned __int128 d2 = (unsigned __int128)L[off+1] - hi - br; L[off+1] = (uint64_t)d2; br = (d2 >> 64) ? 1 : 0;
        for (int i = off + 2; i < 8 && br; i++) { unsigned __int128 d3 = (unsigned __int128)L[i] - br; L[i] = (uint64_t)d3; br = (d3 >> 64) ? 1 : 0; } }
}
static ZagQuire zag_quire_fma(ZagQuire q, uint32_t a, uint32_t b){   /* q += a*b, EXACTLY */
    if (a == 0x80000000u || b == 0x80000000u || q.nar) { q.nar = 1; return q; }
    int na, nb, ea, eb; uint64_t sa, sb;
    zag_p32_fields(a, &na, &ea, &sa); zag_p32_fields(b, &nb, &eb, &sb);
    if (sa == 0 || sb == 0) return q;                 /* product is zero */
    uint64_t P = sa * sb;                              /* <= 2^57, exact in uint64 */
    int shift = (ea + eb) + 240;                       /* place at exact bit position */
    if (shift < 0) return q;                           /* below quire LSB (unreachable for valid p32) */
    int off = shift >> 6, bo = shift & 63;
    uint64_t lo, hi;
    if (bo == 0) { lo = P; hi = 0; } else { lo = P << bo; hi = P >> (64 - bo); }
    if (na ^ nb) q_sub_at(q.limb, off, lo, hi); else q_add_at(q.limb, off, lo, hi);
    return q;
}
static int q_bit(const uint64_t* L, int i){ return (int)((L[i >> 6] >> (i & 63)) & 1); }
static uint32_t zag_quire_to_p32(ZagQuire q){           /* round the exact value to p32 (RNE) */
    if (q.nar) return 0x80000000u;
    uint64_t m[8]; int neg = (q.limb[7] >> 63) & 1;
    if (neg) { uint64_t c = 1; for (int i = 0; i < 8; i++) { unsigned __int128 t = (unsigned __int128)(~q.limb[i]) + c; m[i] = (uint64_t)t; c = (uint64_t)(t >> 64); } }
    else for (int i = 0; i < 8; i++) m[i] = q.limb[i];
    int M = -1; for (int i = 511; i >= 0; i--) if (q_bit(m, i)) { M = i; break; }
    if (M < 0) return 0u;
    uint64_t hi53 = 0;
    for (int j = 0; j < 53; j++) { int bi = M - j; hi53 = (hi53 << 1) | (bi >= 0 ? (uint64_t)q_bit(m, bi) : 0); }
    int sticky = 0; for (int i = 0; i <= M - 53; i++) if (q_bit(m, i)) { sticky = 1; break; }
    if (sticky) hi53 |= 1ull;                           /* round-to-odd: makes the final RNE correct */
    double val = ldexp((double)hi53, (M - 52) - 240);
    return zag_f64_to_p32(neg ? -val : val);
}

/* ── p8: n=8, es=0, useed=2; range [2^-6, 2^6] = [1/64, 64]; max 6 fraction bits ── */
static double zag_p8_to_f64(uint8_t bits){
    if (!bits) return 0.0; if (bits == 0x80u) return NAN;
    int neg = (bits >> 7) & 1;
    uint8_t u = neg ? (uint8_t)(0u - (unsigned)bits) : bits;
    uint32_t wu = (uint32_t)u << 25;          /* bit6 of u -> bit31 for __builtin_clz */
    int r = (int)(wu >> 31);
    int run = __builtin_clz(r ? ~wu : wu);    /* regime run length */
    int k = r ? (run - 1) : (-run);           /* regime value; es=0: useed_exp=1, E=k */
    int F = 6 - run;                          /* n-2-run fraction bits (may be 0 or neg) */
    if (F < 0) F = 0;
    double frac = F ? (double)(u & ((1u << F) - 1u)) / (double)(1u << F) : 0.0;
    return (neg ? -1.0 : 1.0) * ldexp(1.0 + frac, k);
}
static uint8_t zag_f64_to_p8(double x){
    if (x == 0.0) return 0u; if (isnan(x) || isinf(x)) return 0x80u;
    int neg = (x < 0.0); double ax = fabs(x);
    int E2; double m = frexp(ax, &E2); m *= 2.0; int E = E2 - 1;
    int k = E;                                /* es=0: k=E, e=0 */
    double fr = m - 1.0; unsigned char bs[24]; int n = 0;
    if(k>=0){for(int i=0;i<=k&&n<24;i++)bs[n++]=1;if(n<24)bs[n++]=0;}
    else    {for(int i=0;i<-k&&n<24;i++)bs[n++]=0;if(n<24)bs[n++]=1;}
    while(n<24){fr*=2.0;int d=(int)fr;bs[n++]=(unsigned char)d;fr-=d;}
    uint8_t mag=0; for(int i=0;i<7;i++) mag=(uint8_t)((mag<<1)|(i<n?bs[i]:0));
    int rb=(7<n)?bs[7]:0,st=0; for(int i=8;i<n;i++) st|=bs[i];
    if(rb&&(st||(mag&1))) mag++;
    uint8_t w = mag & 0x7Fu; if(!w) w=1;
    return neg ? (uint8_t)(0u-(unsigned)w) : w;
}
static uint8_t  zag_p8_from_i64(int64_t v){ return zag_f64_to_p8((double)v); }
static uint64_t zag_p8_bits(uint8_t b){ return (uint64_t)b; }
static uint8_t  zag_p8_add(uint8_t a,uint8_t b){ if(a==0x80u||b==0x80u)return 0x80u; return zag_f64_to_p8(zag_p8_to_f64(a)+zag_p8_to_f64(b)); }
static uint8_t  zag_p8_sub(uint8_t a,uint8_t b){ if(a==0x80u||b==0x80u)return 0x80u; return zag_f64_to_p8(zag_p8_to_f64(a)-zag_p8_to_f64(b)); }
static uint8_t  zag_p8_mul(uint8_t a,uint8_t b){ if(a==0x80u||b==0x80u)return 0x80u;if(!a||!b)return 0u; return zag_f64_to_p8(zag_p8_to_f64(a)*zag_p8_to_f64(b)); }
static uint8_t  zag_p8_div(uint8_t a,uint8_t b){ if(a==0x80u||b==0x80u||!b)return 0x80u;if(!a)return 0u; return zag_f64_to_p8(zag_p8_to_f64(a)/zag_p8_to_f64(b)); }

/* ── p16: n=16, es=1, useed=4; range [2^-28, 2^28]; max 14 fraction bits ── */
static double zag_p16_to_f64(uint16_t bits){
    if (!bits) return 0.0; if (bits == 0x8000u) return NAN;
    int neg = (bits >> 15) & 1;
    uint16_t u = neg ? (uint16_t)(0u - (unsigned)bits) : bits;
    uint32_t wu = (uint32_t)u << 17;          /* bit14 of u -> bit31 */
    int r = (int)(wu >> 31);
    int run = __builtin_clz(r ? ~wu : wu);
    int k = r ? (run - 1) : (-run);
    int rem = 14 - run;                       /* n-2-run */
    int e = 0;
    if (rem >= 1) { e = (u >> (rem - 1)) & 1; rem--; }  /* es=1: one exp bit */
    int F = rem > 0 ? rem : 0;
    double frac = F ? (double)(u & ((1u << F) - 1u)) / (double)(1u << F) : 0.0;
    return (neg ? -1.0 : 1.0) * ldexp(1.0 + frac, 2 * k + e);
}
static uint16_t zag_f64_to_p16(double x){
    if (x == 0.0) return 0u; if (isnan(x) || isinf(x)) return 0x8000u;
    int neg = (x < 0.0); double ax = fabs(x);
    int E2; double m = frexp(ax, &E2); m *= 2.0; int E = E2 - 1;
    int k = (E >= 0) ? (E / 2) : -(((-E) + 1) / 2);    /* floor(E/2) for es=1 */
    int e = E - 2 * k;
    double fr = m - 1.0; unsigned char bs[48]; int n = 0;
    if(k>=0){for(int i=0;i<=k&&n<48;i++)bs[n++]=1;if(n<48)bs[n++]=0;}
    else    {for(int i=0;i<-k&&n<48;i++)bs[n++]=0;if(n<48)bs[n++]=1;}
    if(n<48) bs[n++]=e&1;                    /* es=1: single exp bit */
    while(n<48){fr*=2.0;int d=(int)fr;bs[n++]=(unsigned char)d;fr-=d;}
    uint16_t mag=0; for(int i=0;i<15;i++) mag=(uint16_t)((mag<<1)|(i<n?bs[i]:0));
    int rb=(15<n)?bs[15]:0,st=0; for(int i=16;i<n;i++) st|=bs[i];
    if(rb&&(st||(mag&1))) mag++;
    uint16_t w = mag & 0x7FFFu; if(!w) w=1;
    return neg ? (uint16_t)(0u-(unsigned)w) : w;
}
static uint16_t zag_p16_from_i64(int64_t v){ return zag_f64_to_p16((double)v); }
static uint64_t zag_p16_bits(uint16_t b){ return (uint64_t)b; }
static uint16_t zag_p16_add(uint16_t a,uint16_t b){ if(a==0x8000u||b==0x8000u)return 0x8000u; return zag_f64_to_p16(zag_p16_to_f64(a)+zag_p16_to_f64(b)); }
static uint16_t zag_p16_sub(uint16_t a,uint16_t b){ if(a==0x8000u||b==0x8000u)return 0x8000u; return zag_f64_to_p16(zag_p16_to_f64(a)-zag_p16_to_f64(b)); }
static uint16_t zag_p16_mul(uint16_t a,uint16_t b){ if(a==0x8000u||b==0x8000u)return 0x8000u;if(!a||!b)return 0u; return zag_f64_to_p16(zag_p16_to_f64(a)*zag_p16_to_f64(b)); }
static uint16_t zag_p16_div(uint16_t a,uint16_t b){ if(a==0x8000u||b==0x8000u||!b)return 0x8000u;if(!a)return 0u; return zag_f64_to_p16(zag_p16_to_f64(a)/zag_p16_to_f64(b)); }

/* ── p64: n=64, es=3, useed=256; range [2^-240, 2^240]; max 59 fraction bits.
   Decode/encode via long double (64-bit mantissa ≥ 59 frac bits → exact).
   Arithmetic via long double (exact for +/-; mul has ≤1 ULP — 120-bit product > 64-bit mant). ── */
static long double zag_p64_to_ld(uint64_t bits){
    if (!bits) return 0.0L;
    if (bits == 0x8000000000000000ULL) return (long double)NAN;
    int neg = (int)(bits >> 63);
    uint64_t u = neg ? (uint64_t)(0ULL - bits) : bits;
    uint64_t w = u << 1;                          /* bit62 of u -> bit63, for __builtin_clzll */
    int r = (int)(w >> 63);
    int run = __builtin_clzll(r ? ~w : w);
    int k = r ? (run - 1) : (-run);
    int rem = 62 - run;                            /* n-2-run bits remaining */
    int e = 0;
    if (rem >= 3) { e = (int)((u >> (rem - 3)) & 7ULL); rem -= 3; }   /* es=3: 3 exp bits */
    else if (rem > 0) { e = (int)((u & ((1ULL << rem) - 1ULL)) << (3 - rem)); rem = 0; }
    int F = rem > 0 ? rem : 0;
    long double frac = F ? (long double)(u & ((F >= 64) ? ~0ULL : ((1ULL << F) - 1ULL)))
                         / (long double)(1ULL << F) : 0.0L;
    return (neg ? -1.0L : 1.0L) * ldexpl(1.0L + frac, 8 * k + e);
}
static uint64_t zag_ld_to_p64(long double x){
    if (x == 0.0L) return 0ULL;
    if (isnan(x) || isinf(x)) return 0x8000000000000000ULL;
    int neg = (x < 0.0L); long double ax = fabsl(x);
    int E2; long double m = frexpl(ax, &E2); m *= 2.0L; int E = E2 - 1;
    int k = (E >= 0) ? (E / 8) : -(((-E) + 7) / 8);
    int e = E - 8 * k;
    long double fr = m - 1.0L; unsigned char bs[200]; int n = 0;
    if(k>=0){for(int i=0;i<=k&&n<200;i++)bs[n++]=1;if(n<200)bs[n++]=0;}
    else    {for(int i=0;i<-k&&n<200;i++)bs[n++]=0;if(n<200)bs[n++]=1;}
    if(n<200)bs[n++]=(e>>2)&1; if(n<200)bs[n++]=(e>>1)&1; if(n<200)bs[n++]=e&1;
    while(n<200){fr*=2.0L;int d=(int)fr;bs[n++]=(unsigned char)d;fr-=d;}
    uint64_t mag=0; for(int i=0;i<63;i++) mag=(mag<<1)|(i<n?(uint64_t)bs[i]:0ULL);
    int rb=(63<n)?bs[63]:0,st=0; for(int i=64;i<n;i++) st|=bs[i];
    if(rb&&(st||(int)(mag&1))) mag++;
    uint64_t w2 = mag & 0x7FFFFFFFFFFFFFFFULL; if(!w2) w2=1;
    return neg ? (uint64_t)(0ULL - w2) : w2;
}
static double   zag_p64_to_f64(uint64_t b){ return (double)zag_p64_to_ld(b); }
static uint64_t zag_f64_to_p64(double x){ return zag_ld_to_p64((long double)x); }
static uint64_t zag_p64_from_i64(int64_t v){ return zag_ld_to_p64((long double)v); }
static uint64_t zag_p64_bits(uint64_t b){ return b; }
static uint64_t zag_p64_add(uint64_t a,uint64_t b){
    if(a==0x8000000000000000ULL||b==0x8000000000000000ULL)return 0x8000000000000000ULL;
    return zag_ld_to_p64(zag_p64_to_ld(a)+zag_p64_to_ld(b)); }
static uint64_t zag_p64_sub(uint64_t a,uint64_t b){
    if(a==0x8000000000000000ULL||b==0x8000000000000000ULL)return 0x8000000000000000ULL;
    return zag_ld_to_p64(zag_p64_to_ld(a)-zag_p64_to_ld(b)); }
static uint64_t zag_p64_mul(uint64_t a,uint64_t b){
    if(a==0x8000000000000000ULL||b==0x8000000000000000ULL)return 0x8000000000000000ULL;
    if(!a||!b)return 0ULL;
    return zag_ld_to_p64(zag_p64_to_ld(a)*zag_p64_to_ld(b)); }
static uint64_t zag_p64_div(uint64_t a,uint64_t b){
    if(a==0x8000000000000000ULL||b==0x8000000000000000ULL||!b)return 0x8000000000000000ULL;
    if(!a)return 0ULL;
    return zag_ld_to_p64(zag_p64_to_ld(a)/zag_p64_to_ld(b)); }
typedef enum { optic__phase__PhaseQuant_Neg120, optic__phase__PhaseQuant_Zero, optic__phase__PhaseQuant_Pos120 } optic__phase__PhaseQuant;
typedef struct optic__phase__Phase_s optic__phase__Phase;
typedef struct optic__Wave_s optic__Wave;
typedef struct optic_beam__BeamHop_s optic_beam__BeamHop;
typedef struct optic_beam__Beam_s optic_beam__Beam;
typedef struct optic_beam__BeamPath_s optic_beam__BeamPath;
typedef enum { trit__Trit_Neg, trit__Trit_Zero, trit__Trit_Pos } trit__Trit;
typedef enum { trit____zag_errors_InvalidTrit, trit____zag_errors_InvalidTritIndex } trit____zag_errors;
typedef struct VoxelCoord_s VoxelCoord;
typedef struct BeamId_s BeamId;
typedef struct WaveSample_s WaveSample;
typedef struct HorizontalBeam_s HorizontalBeam;
typedef struct VerticalPipeline_s VerticalPipeline;
typedef struct CollisionResult_s CollisionResult;
typedef enum { trit__Trit_Neg, trit__Trit_Zero, trit__Trit_Pos } trit__Trit;
typedef enum { trit____zag_errors_InvalidTrit, trit____zag_errors_InvalidTritIndex } trit____zag_errors;
enum { ZAG_OK = 0, ZAG_ERR_RawDenied = 1, ZAG_ERR_InvalidPolarity = 2, ZAG_ERR_LaserFault = 3 };
typedef struct RawSession_s RawSession;
struct optic__phase__Phase_s { float radians; optic__phase__PhaseQuant quantized; int8_t trit_wire; };
struct optic__Wave_s { float amplitude; float frequency_hz; float wavelength_m; optic__phase__Phase phase; int32_t polarization; };
struct optic_beam__BeamHop_s { int32_t pcm_row; int32_t pcm_col; phase__Phase phase_delta; int32_t lattice_op; };
struct optic_beam__Beam_s { wave__Wave wave; int32_t origin_row; int32_t origin_col; int32_t hop_count; float total_phase_rad; };
struct optic_beam__BeamPath_s { ZagSlice_optic_beam__BeamHop hops; int32_t len; };
struct VoxelCoord_s { int32_t x; int32_t y; int32_t z; };
struct BeamId_s { int32_t wall; int32_t slot; };
struct WaveSample_s { float amp; float phase; int8_t pol; uint64_t payload; };
struct HorizontalBeam_s { VoxelCoord origin; VoxelCoord target; float power; float phase; int8_t pol; BeamId beam_id; optic_beam__Beam optic; };
struct VerticalPipeline_s { int32_t column; int32_t depth; int32_t head_z; int32_t active; };
struct CollisionResult_s { WaveSample out; int32_t absorbed; int32_t split; };
struct RawSession_s { int32_t active; };
typedef struct { int32_t _has; trit__Trit _val; } ZagOpt_trit__Trit;
typedef struct { int32_t _err; trit__Trit _val; } ZagErr_trit__Trit;
typedef struct { int32_t _err; RawSession _val; } ZagErr_RawSession;
typedef struct { int32_t _err; void _val; } ZagErr_void;
static float optic__phase__pi_third(void);
static float optic__phase__two_pi_third(void);
static optic__phase__Phase optic__phase__from_radians(float r);
static optic__phase__Phase optic__phase__from_trit(int8_t t);
static optic__phase__PhaseQuant optic__phase__quantize(float r);
static optic__phase__Phase optic__phase__shift(optic__phase__Phase base, float delta_rad);
static optic__phase__Phase optic__phase__negate(optic__phase__Phase p);
static float optic__phase__step_rad(void);
static optic__Wave optic__wave_make(float amplitude, float frequency_hz, float phase_rad);
static optic__Wave optic__from_trit(float amplitude, float frequency_hz, int8_t trit_val);
static float optic__intensity(optic__Wave w);
static optic__Wave optic__phase_shift(optic__Wave w, float delta_rad);
static float optic__clamp_unit_amp(float v);
static optic__Wave optic__superpose(optic__Wave a, optic__Wave b);
static optic_beam__Beam optic_beam__launch(wave__Wave w, int32_t row, int32_t col);
static optic_beam__Beam optic_beam__route(optic_beam__Beam b, int32_t row, int32_t col, phase__Phase delta, int32_t op_tag);
static optic_beam__Beam optic_beam__trit_hop(optic_beam__Beam b, int32_t row, int32_t col, int8_t trit_val, int32_t op_tag);
static optic_beam__BeamPath optic_beam__append_hop(optic_beam__BeamPath path, optic_beam__BeamHop hop);
static int8_t optic_beam__terminal_trit(optic_beam__Beam b);
static int8_t trit__trit_neg_val(void);
static int8_t trit__trit_zero_val(void);
static int8_t trit__trit_pos_val(void);
static int32_t trit__trit_count(void);
static trit__Trit trit__neg(void);
static trit__Trit trit__zero(void);
static trit__Trit trit__pos(void);
static ZagErr_trit__Trit trit__from_i8(int8_t v);
static ZagErr_trit__Trit trit__from_index(int32_t idx);
static ZagOpt_trit__Trit trit__try_i8(int8_t v);
static int8_t trit__to_i8(trit__Trit t);
static int32_t trit__to_index(trit__Trit t);
static int8_t trit__index_to_i8(int32_t idx);
static int32_t trit__i8_to_index(int8_t v);
static int32_t trit__is_valid_i8(int8_t v);
static int32_t trit__is_valid_index(int32_t idx);
static int8_t trit__clamp_i8(int8_t v);
static ZagSliceU8 trit__symbol(trit__Trit t);
static ZagSliceU8 trit__symbol_i8(int8_t v);
static trit__Trit trit__Trit_negate(trit__Trit self);
static int32_t trit__Trit_is_negative(trit__Trit self);
static int32_t trit__Trit_is_zero(trit__Trit self);
static int32_t trit__Trit_is_positive(trit__Trit self);
static int8_t POL_OFF(void);
static int8_t POL_VERT(void);
static int8_t POL_HORZ(void);
static int8_t polarity_to_i8(trit__Trit p);
static ZagErr_trit__Trit i8_to_polarity(int8_t v);
static int8_t ternary_add(int8_t a, int8_t b);
static VoxelCoord voxel(int32_t x, int32_t y, int32_t z);
static WaveSample wave_neutral(void);
static WaveSample wave_vertical(float amp, float phase, uint64_t payload);
static WaveSample wave_horizontal(float amp, float phase);
static VerticalPipeline pipeline_new(int32_t column, int32_t depth);
static HorizontalBeam horizontal_beam(VoxelCoord origin, VoxelCoord target, float power, int32_t wall, int32_t slot);
static optic__Wave sample_to_wave(WaveSample s);
static WaveSample wave_to_sample(optic__Wave w, uint64_t payload);
static int8_t trit__trit_neg_val(void);
static int8_t trit__trit_zero_val(void);
static int8_t trit__trit_pos_val(void);
static int32_t trit__trit_count(void);
static trit__Trit trit__neg(void);
static trit__Trit trit__zero(void);
static trit__Trit trit__pos(void);
static ZagErr_trit__Trit trit__from_i8(int8_t v);
static ZagErr_trit__Trit trit__from_index(int32_t idx);
static ZagOpt_trit__Trit trit__try_i8(int8_t v);
static int8_t trit__to_i8(trit__Trit t);
static int32_t trit__to_index(trit__Trit t);
static int8_t trit__index_to_i8(int32_t idx);
static int32_t trit__i8_to_index(int8_t v);
static int32_t trit__is_valid_i8(int8_t v);
static int32_t trit__is_valid_index(int32_t idx);
static int8_t trit__clamp_i8(int8_t v);
static ZagSliceU8 trit__symbol(trit__Trit t);
static ZagSliceU8 trit__symbol_i8(int8_t v);
static trit__Trit trit__Trit_negate(trit__Trit self);
static int32_t trit__Trit_is_negative(trit__Trit self);
static int32_t trit__Trit_is_zero(trit__Trit self);
static int32_t trit__Trit_is_positive(trit__Trit self);
extern int32_t _flash_raw_fire_shield(int32_t x, int32_t y, int32_t z, float mw);
extern int32_t _flash_raw_inject_phase(int32_t x, int32_t y, int32_t z, float phase);
extern int32_t _flash_raw_set_laser_mw(int32_t wall, int32_t slot, float mw);
extern int32_t _flash_raw_target_voxel(int32_t x, int32_t y, int32_t z);
extern int32_t _flash_raw_set_polarity(int32_t x, int32_t y, int32_t z, int32_t pol);
extern int32_t _flash_raw_mode_enter(void);
extern int32_t _flash_raw_mode_exit(void);
static ZagErr_RawSession raw_enter(void);
static void raw_exit(RawSession* sess);
static ZagErr_void fire_shield(VoxelCoord target, float mw);
static ZagErr_void inject_phase(VoxelCoord coord, float phase_rad);
static ZagErr_void set_laser_mw(BeamId beam, float mw);
static ZagErr_void target_voxel(VoxelCoord coord);
static ZagErr_void manual_polarity(VoxelCoord coord, int8_t pol);
static ZagErr_void manual_polarity_trit(VoxelCoord coord, trit__Trit p);
static float optic__phase__pi_third(void) {
    return 1.0471976;
}
static float optic__phase__two_pi_third(void) {
    return 2.0943952;
}
static optic__phase__Phase optic__phase__from_radians(float r) {
    optic__phase__PhaseQuant q = optic__phase__PhaseQuant_Zero;
    int8_t tw = 0;
    if ((r < (0.0 - 0.5235988))) {
    q = optic__phase__PhaseQuant_Neg120;
    tw = (0 - 1);
    } else {
    if ((r > 0.5235988)) {
    q = optic__phase__PhaseQuant_Pos120;
    tw = 1;
    } else {
    q = optic__phase__PhaseQuant_Zero;
    tw = 0;
    }
    }
    return (optic__phase__Phase){ .radians = r, .quantized = q, .trit_wire = tw };
}
static optic__phase__Phase optic__phase__from_trit(int8_t t) {
    float r = 0.0;
    optic__phase__PhaseQuant q = optic__phase__PhaseQuant_Zero;
    if ((t == (0 - 1))) {
    r = (0.0 - optic__phase__pi_third());
    q = optic__phase__PhaseQuant_Neg120;
    } else {
    if ((t == 1)) {
    r = optic__phase__pi_third();
    q = optic__phase__PhaseQuant_Pos120;
    } else {
    r = 0.0;
    q = optic__phase__PhaseQuant_Zero;
    }
    }
    return (optic__phase__Phase){ .radians = r, .quantized = q, .trit_wire = t };
}
static optic__phase__PhaseQuant optic__phase__quantize(float r) {
    if ((r < (0.0 - 0.5235988))) {
    return optic__phase__PhaseQuant_Neg120;
    }
    if ((r > 0.5235988)) {
    return optic__phase__PhaseQuant_Pos120;
    }
    return optic__phase__PhaseQuant_Zero;
}
static optic__phase__Phase optic__phase__shift(optic__phase__Phase base, float delta_rad) {
    return optic__phase__from_radians((base.radians + delta_rad));
}
static optic__phase__Phase optic__phase__negate(optic__phase__Phase p) {
    return optic__phase__from_radians((0.0 - p.radians));
}
static float optic__phase__step_rad(void) {
    return optic__phase__two_pi_third();
}
static optic__Wave optic__wave_make(float amplitude, float frequency_hz, float phase_rad) {
    float c = 299792458.0;
    float wl = (c / frequency_hz);
    optic__phase__Phase ph = phase__from_radians(phase_rad);
    return (optic__Wave){ .amplitude = amplitude, .frequency_hz = frequency_hz, .wavelength_m = wl, .phase = ph, .polarization = 0 };
}
static optic__Wave optic__from_trit(float amplitude, float frequency_hz, int8_t trit_val) {
    optic__phase__Phase ph = phase__from_trit(trit_val);
    float c = 299792458.0;
    float wl = (c / frequency_hz);
    return (optic__Wave){ .amplitude = amplitude, .frequency_hz = frequency_hz, .wavelength_m = wl, .phase = ph, .polarization = 0 };
}
static float optic__intensity(optic__Wave w) {
    return (w.amplitude * w.amplitude);
}
static optic__Wave optic__phase_shift(optic__Wave w, float delta_rad) {
    optic__phase__Phase ph = phase__shift(w.phase, delta_rad);
    return (optic__Wave){ .amplitude = w.amplitude, .frequency_hz = w.frequency_hz, .wavelength_m = w.wavelength_m, .phase = ph, .polarization = w.polarization };
}
static float optic__clamp_unit_amp(float v) {
    if ((v > 1.0)) {
    return 1.0;
    }
    return v;
}
static optic__Wave optic__superpose(optic__Wave a, optic__Wave b) {
    float amp = (a.amplitude + b.amplitude);
    float clamped = optic__clamp_unit_amp(amp);
    optic__phase__Phase ph = phase__from_radians((a.phase.radians + b.phase.radians));
    return (optic__Wave){ .amplitude = clamped, .frequency_hz = a.frequency_hz, .wavelength_m = a.wavelength_m, .phase = ph, .polarization = a.polarization };
}
static optic_beam__Beam optic_beam__launch(wave__Wave w, int32_t row, int32_t col) {
    return (optic_beam__Beam){ .wave = w, .origin_row = row, .origin_col = col, .hop_count = 0, .total_phase_rad = w.phase.radians };
}
static optic_beam__Beam optic_beam__route(optic_beam__Beam b, int32_t row, int32_t col, phase__Phase delta, int32_t op_tag) {
    float new_phase = (b.total_phase_rad + delta.radians);
    wave__Wave w2 = wave__phase_shift(b.wave, delta.radians);
    return (optic_beam__Beam){ .wave = w2, .origin_row = b.origin_row, .origin_col = b.origin_col, .hop_count = (b.hop_count + 1), .total_phase_rad = new_phase };
}
static optic_beam__Beam optic_beam__trit_hop(optic_beam__Beam b, int32_t row, int32_t col, int8_t trit_val, int32_t op_tag) {
    phase__Phase delta = phase__from_trit(trit_val);
    return optic_beam__route(b, row, col, delta, op_tag);
}
static optic_beam__BeamPath optic_beam__append_hop(optic_beam__BeamPath path, optic_beam__BeamHop hop) {
    if ((path.len < path.hops.len)) {
    (path.hops).ptr[path.len] = hop;
    path.len = (path.len + 1);
    }
    return path;
}
static int8_t optic_beam__terminal_trit(optic_beam__Beam b) {
    return b.wave.phase.trit_wire;
}
static int8_t trit__trit_neg_val(void) {
    return (0 - 1);
}
static int8_t trit__trit_zero_val(void) {
    return 0;
}
static int8_t trit__trit_pos_val(void) {
    return 1;
}
static int32_t trit__trit_count(void) {
    return 3;
}
static trit__Trit trit__neg(void) {
    return trit__Trit_Neg;
}
static trit__Trit trit__zero(void) {
    return trit__Trit_Zero;
}
static trit__Trit trit__pos(void) {
    return trit__Trit_Pos;
}
static ZagErr_trit__Trit trit__from_i8(int8_t v) {
    if ((v == trit__trit_neg_val())) {
    return (ZagErr_trit__Trit){ ._err = 0, ._val = trit__Trit_Neg };
    }
    if ((v == trit__trit_zero_val())) {
    return (ZagErr_trit__Trit){ ._err = 0, ._val = trit__Trit_Zero };
    }
    if ((v == trit__trit_pos_val())) {
    return (ZagErr_trit__Trit){ ._err = 0, ._val = trit__Trit_Pos };
    }
    return (ZagErr_trit__Trit){ ._err = ZAG_ERR_InvalidTrit };
}
static ZagErr_trit__Trit trit__from_index(int32_t idx) {
    if ((idx == 0)) {
    return (ZagErr_trit__Trit){ ._err = 0, ._val = trit__Trit_Neg };
    }
    if ((idx == 1)) {
    return (ZagErr_trit__Trit){ ._err = 0, ._val = trit__Trit_Zero };
    }
    if ((idx == 2)) {
    return (ZagErr_trit__Trit){ ._err = 0, ._val = trit__Trit_Pos };
    }
    return (ZagErr_trit__Trit){ ._err = ZAG_ERR_InvalidTritIndex };
}
static ZagOpt_trit__Trit trit__try_i8(int8_t v) {
    if ((v == trit__trit_neg_val())) {
    return (ZagOpt_trit__Trit){1, trit__Trit_Neg};
    }
    if ((v == trit__trit_zero_val())) {
    return (ZagOpt_trit__Trit){1, trit__Trit_Zero};
    }
    if ((v == trit__trit_pos_val())) {
    return (ZagOpt_trit__Trit){1, trit__Trit_Pos};
    }
    return (ZagOpt_trit__Trit){0};
}
static int8_t trit__to_i8(trit__Trit t) {
    return ({ __auto_type __sw = t; (__sw == trit__Trit_Neg) ? (trit_neg_val()) : (__sw == trit__Trit_Zero) ? (trit_zero_val()) : (trit_pos_val()); });
}
static int32_t trit__to_index(trit__Trit t) {
    return ({ __auto_type __sw = t; (__sw == trit__Trit_Neg) ? (0) : (__sw == trit__Trit_Zero) ? (1) : (2); });
}
static int8_t trit__index_to_i8(int32_t idx) {
    return ({ __auto_type __sw = idx; (__sw == 0) ? (trit_neg_val()) : (__sw == 1) ? (trit_zero_val()) : (__sw == 2) ? (trit_pos_val()) : (trit_zero_val()); });
}
static int32_t trit__i8_to_index(int8_t v) {
    if ((v == trit__trit_neg_val())) {
    return 0;
    }
    if ((v == trit__trit_zero_val())) {
    return 1;
    }
    return 2;
}
static int32_t trit__is_valid_i8(int8_t v) {
    return (((v == trit__trit_neg_val()) || (v == trit__trit_zero_val())) || (v == trit__trit_pos_val()));
}
static int32_t trit__is_valid_index(int32_t idx) {
    return ((idx >= 0) && (idx < trit__trit_count()));
}
static int8_t trit__clamp_i8(int8_t v) {
    if ((v < trit__trit_neg_val())) {
    return trit__trit_neg_val();
    }
    if ((v > trit__trit_pos_val())) {
    return trit__trit_pos_val();
    }
    return v;
}
static ZagSliceU8 trit__symbol(trit__Trit t) {
    return ({ __auto_type __sw = t; (__sw == trit__Trit_Neg) ? ((ZagSliceU8){(const uint8_t*)"T", 1}) : (__sw == trit__Trit_Zero) ? ((ZagSliceU8){(const uint8_t*)"0", 1}) : ((ZagSliceU8){(const uint8_t*)"1", 1}); });
}
static ZagSliceU8 trit__symbol_i8(int8_t v) {
    if ((v == trit__trit_neg_val())) {
    return (ZagSliceU8){(const uint8_t*)"T", 1};
    }
    if ((v == trit__trit_pos_val())) {
    return (ZagSliceU8){(const uint8_t*)"1", 1};
    }
    return (ZagSliceU8){(const uint8_t*)"0", 1};
}
static trit__Trit trit__Trit_negate(trit__Trit self) {
    return ({ __auto_type __sw = self; (__sw == trit__Trit_Neg) ? (Trit_Pos) : (__sw == trit__Trit_Zero) ? (Trit_Zero) : (Trit_Neg); });
}
static int32_t trit__Trit_is_negative(trit__Trit self) {
    return (self == trit__Trit_Neg);
}
static int32_t trit__Trit_is_zero(trit__Trit self) {
    return (self == trit__Trit_Zero);
}
static int32_t trit__Trit_is_positive(trit__Trit self) {
    return (self == trit__Trit_Pos);
}
static int8_t POL_OFF(void) {
    return trit__trit_zero_val();
}
static int8_t POL_VERT(void) {
    return trit__trit_pos_val();
}
static int8_t POL_HORZ(void) {
    return trit__trit_neg_val();
}
static int8_t polarity_to_i8(trit__Trit p) {
    return trit__to_i8(p);
}
static ZagErr_trit__Trit i8_to_polarity(int8_t v) {
    return ({ __auto_type __t = trit__from_i8(v); if (__t._err) return (ZagErr_trit__Trit){ ._err = __t._err }; __t._val; });
}
static int8_t ternary_add(int8_t a, int8_t b) {
    int32_t s = (((int32_t)(a)) + ((int32_t)(b)));
    return trit__clamp_i8(((int8_t)(s)));
}
static VoxelCoord voxel(int32_t x, int32_t y, int32_t z) {
    return (VoxelCoord){ .x = x, .y = y, .z = z };
}
static WaveSample wave_neutral(void) {
    return (WaveSample){ .amp = 0.0, .phase = 0.0, .pol = POL_OFF(), .payload = 0 };
}
static WaveSample wave_vertical(float amp, float phase, uint64_t payload) {
    return (WaveSample){ .amp = amp, .phase = phase, .pol = POL_VERT(), .payload = payload };
}
static WaveSample wave_horizontal(float amp, float phase) {
    return (WaveSample){ .amp = amp, .phase = phase, .pol = POL_HORZ(), .payload = 0 };
}
static VerticalPipeline pipeline_new(int32_t column, int32_t depth) {
    return (VerticalPipeline){ .column = column, .depth = depth, .head_z = 0, .active = 0 };
}
static HorizontalBeam horizontal_beam(VoxelCoord origin, VoxelCoord target, float power, int32_t wall, int32_t slot) {
    optic__Wave w = optic__from_trit(1.0, 193414489.0, POL_HORZ());
    optic_beam__Beam b = optic_beam__launch(w, origin.x, origin.y);
    return (HorizontalBeam){ .origin = origin, .target = target, .power = power, .phase = 0.0, .pol = POL_HORZ(), .beam_id = (BeamId){ .wall = wall, .slot = slot }, .optic = b };
}
static optic__Wave sample_to_wave(WaveSample s) {
    return optic__from_trit(s.amp, 193414489.0, s.pol);
}
static WaveSample wave_to_sample(optic__Wave w, uint64_t payload) {
    return (WaveSample){ .amp = w.amplitude, .phase = w.phase.radians, .pol = w.phase.trit_wire, .payload = payload };
}
static int8_t trit__trit_neg_val(void) {
    return (0 - 1);
}
static int8_t trit__trit_zero_val(void) {
    return 0;
}
static int8_t trit__trit_pos_val(void) {
    return 1;
}
static int32_t trit__trit_count(void) {
    return 3;
}
static trit__Trit trit__neg(void) {
    return trit__Trit_Neg;
}
static trit__Trit trit__zero(void) {
    return trit__Trit_Zero;
}
static trit__Trit trit__pos(void) {
    return trit__Trit_Pos;
}
static ZagErr_trit__Trit trit__from_i8(int8_t v) {
    if ((v == trit__trit_neg_val())) {
    return (ZagErr_trit__Trit){ ._err = 0, ._val = trit__Trit_Neg };
    }
    if ((v == trit__trit_zero_val())) {
    return (ZagErr_trit__Trit){ ._err = 0, ._val = trit__Trit_Zero };
    }
    if ((v == trit__trit_pos_val())) {
    return (ZagErr_trit__Trit){ ._err = 0, ._val = trit__Trit_Pos };
    }
    return (ZagErr_trit__Trit){ ._err = ZAG_ERR_InvalidTrit };
}
static ZagErr_trit__Trit trit__from_index(int32_t idx) {
    if ((idx == 0)) {
    return (ZagErr_trit__Trit){ ._err = 0, ._val = trit__Trit_Neg };
    }
    if ((idx == 1)) {
    return (ZagErr_trit__Trit){ ._err = 0, ._val = trit__Trit_Zero };
    }
    if ((idx == 2)) {
    return (ZagErr_trit__Trit){ ._err = 0, ._val = trit__Trit_Pos };
    }
    return (ZagErr_trit__Trit){ ._err = ZAG_ERR_InvalidTritIndex };
}
static ZagOpt_trit__Trit trit__try_i8(int8_t v) {
    if ((v == trit__trit_neg_val())) {
    return (ZagOpt_trit__Trit){1, trit__Trit_Neg};
    }
    if ((v == trit__trit_zero_val())) {
    return (ZagOpt_trit__Trit){1, trit__Trit_Zero};
    }
    if ((v == trit__trit_pos_val())) {
    return (ZagOpt_trit__Trit){1, trit__Trit_Pos};
    }
    return (ZagOpt_trit__Trit){0};
}
static int8_t trit__to_i8(trit__Trit t) {
    return ({ __auto_type __sw = t; (__sw == trit__Trit_Neg) ? (trit_neg_val()) : (__sw == trit__Trit_Zero) ? (trit_zero_val()) : (trit_pos_val()); });
}
static int32_t trit__to_index(trit__Trit t) {
    return ({ __auto_type __sw = t; (__sw == trit__Trit_Neg) ? (0) : (__sw == trit__Trit_Zero) ? (1) : (2); });
}
static int8_t trit__index_to_i8(int32_t idx) {
    return ({ __auto_type __sw = idx; (__sw == 0) ? (trit_neg_val()) : (__sw == 1) ? (trit_zero_val()) : (__sw == 2) ? (trit_pos_val()) : (trit_zero_val()); });
}
static int32_t trit__i8_to_index(int8_t v) {
    if ((v == trit__trit_neg_val())) {
    return 0;
    }
    if ((v == trit__trit_zero_val())) {
    return 1;
    }
    return 2;
}
static int32_t trit__is_valid_i8(int8_t v) {
    return (((v == trit__trit_neg_val()) || (v == trit__trit_zero_val())) || (v == trit__trit_pos_val()));
}
static int32_t trit__is_valid_index(int32_t idx) {
    return ((idx >= 0) && (idx < trit__trit_count()));
}
static int8_t trit__clamp_i8(int8_t v) {
    if ((v < trit__trit_neg_val())) {
    return trit__trit_neg_val();
    }
    if ((v > trit__trit_pos_val())) {
    return trit__trit_pos_val();
    }
    return v;
}
static ZagSliceU8 trit__symbol(trit__Trit t) {
    return ({ __auto_type __sw = t; (__sw == trit__Trit_Neg) ? ((ZagSliceU8){(const uint8_t*)"T", 1}) : (__sw == trit__Trit_Zero) ? ((ZagSliceU8){(const uint8_t*)"0", 1}) : ((ZagSliceU8){(const uint8_t*)"1", 1}); });
}
static ZagSliceU8 trit__symbol_i8(int8_t v) {
    if ((v == trit__trit_neg_val())) {
    return (ZagSliceU8){(const uint8_t*)"T", 1};
    }
    if ((v == trit__trit_pos_val())) {
    return (ZagSliceU8){(const uint8_t*)"1", 1};
    }
    return (ZagSliceU8){(const uint8_t*)"0", 1};
}
static trit__Trit trit__Trit_negate(trit__Trit self) {
    return ({ __auto_type __sw = self; (__sw == trit__Trit_Neg) ? (Trit_Pos) : (__sw == trit__Trit_Zero) ? (Trit_Zero) : (Trit_Neg); });
}
static int32_t trit__Trit_is_negative(trit__Trit self) {
    return (self == trit__Trit_Neg);
}
static int32_t trit__Trit_is_zero(trit__Trit self) {
    return (self == trit__Trit_Zero);
}
static int32_t trit__Trit_is_positive(trit__Trit self) {
    return (self == trit__Trit_Pos);
}
static ZagErr_RawSession raw_enter(void) {
    int32_t ok = _flash_raw_mode_enter();
    if ((ok == 0)) {
    return (ZagErr_RawSession){ ._err = ZAG_ERR_RawDenied };
    }
    return (ZagErr_RawSession){ ._err = 0, ._val = (RawSession){ .active = 1 } };
}
static void raw_exit(RawSession* sess) {
    if (((*sess).active == 1)) {
    _flash_raw_mode_exit();
    (*sess).active = 0;
    }
}
static ZagErr_void fire_shield(VoxelCoord target, float mw) {
    int32_t ok = _flash_raw_fire_shield(target.x, target.y, target.z, mw);
    if ((ok == 0)) {
    return (ZagErr_void){ ._err = ZAG_ERR_LaserFault };
    }
}
static ZagErr_void inject_phase(VoxelCoord coord, float phase_rad) {
    int32_t ok = _flash_raw_inject_phase(coord.x, coord.y, coord.z, phase_rad);
    if ((ok == 0)) {
    return (ZagErr_void){ ._err = ZAG_ERR_LaserFault };
    }
}
static ZagErr_void set_laser_mw(BeamId beam, float mw) {
    int32_t ok = _flash_raw_set_laser_mw(beam.wall, beam.slot, mw);
    if ((ok == 0)) {
    return (ZagErr_void){ ._err = ZAG_ERR_LaserFault };
    }
}
static ZagErr_void target_voxel(VoxelCoord coord) {
    int32_t ok = _flash_raw_target_voxel(coord.x, coord.y, coord.z);
    if ((ok == 0)) {
    return (ZagErr_void){ ._err = ZAG_ERR_LaserFault };
    }
}
static ZagErr_void manual_polarity(VoxelCoord coord, int8_t pol) {
    if ((((pol != POL_OFF()) && (pol != POL_VERT())) && (pol != POL_HORZ()))) {
    return (ZagErr_void){ ._err = ZAG_ERR_InvalidPolarity };
    }
    int32_t tag = 0;
    if ((pol == POL_VERT())) {
    tag = 1;
    }
    if ((pol == POL_HORZ())) {
    tag = 2;
    }
    int32_t ok = _flash_raw_set_polarity(coord.x, coord.y, coord.z, tag);
    if ((ok == 0)) {
    return (ZagErr_void){ ._err = ZAG_ERR_LaserFault };
    }
}
static ZagErr_void manual_polarity_trit(VoxelCoord coord, trit__Trit p) {
    return ({ __auto_type __t = manual_polarity(coord, polarity_to_i8(p)); if (__t._err) return (ZagErr_void){ ._err = __t._err }; __t._val; });
}
