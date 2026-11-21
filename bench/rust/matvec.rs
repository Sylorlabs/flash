// Rust baseline: 64x64 matvec, M=64 vectors.
const N: usize = 64;
const M: usize = 64;
fn main() {
    let mut a = vec![0i32; N * N];
    let mut x = vec![0i32; N];
    let mut y = vec![0i32; N];
    let mut steps: u64 = 0;
    let mut checksum: u64 = 0;
    for i in 0..N * N { a[i] = ((i * 7 + 3) % 97) as i32; }
    for i in 0..N { x[i] = ((i * 5 + 1) % 89) as i32; }
    for _m in 0..M {
        for i in 0..N {
            let mut s: i32 = 0;
            for j in 0..N { s += a[i * N + j] * x[j]; steps += 1; }
            y[i] = s;
        }
        checksum += y[0] as u64;
    }
    println!("{} steps={}", checksum, steps);
}
