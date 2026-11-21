// Rust baseline: direct O(N^2) DFT, N=1024, R=64.
const N: usize = 1024;
const R: usize = 64;
fn main() {
    let mut xr = [0f64; N];
    let xi = [0f64; N];
    let mut yr = [0f64; N];
    let mut yi = [0f64; N];
    let mut steps: u64 = 0;
    let mut checksum: u64 = 0;
    for i in 0..N { xr[i] = (i % 97) as f64 / 97.0; }
    let pi = 3.14159265358979f64;
    for _r in 0..R {
        for k in 0..N {
            let mut sr = 0f64; let mut si = 0f64;
            for n in 0..N {
                let ang = -2.0 * pi * (k * n) as f64 / N as f64;
                sr += xr[n] * ang.cos() - xi[n] * ang.sin();
                si += xr[n] * ang.sin() + xi[n] * ang.cos();
                steps += 2;
            }
            yr[k] = sr; yi[k] = si;
        }
        checksum += (yr[0] * 1000.0) as u64;
    }
    println!("{} steps={}", checksum, steps);
}
