// Rust baseline: serial cell-marking sweeps, 64 x 256K cells.
const K: usize = 262144;
fn main() {
    let mut lat = vec![0u8; K];
    let mut steps: u64 = 0;
    let mut checksum: u64 = 0;
    for _r in 0..64 {
        for i in 0..K { lat[i] = 1; checksum += (lat[i] & 1) as u64; steps += 1; }
    }
    println!("{} steps={}", checksum, steps);
}
