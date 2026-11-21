// Rust baseline: 512K serial record scan+sum.
const K: usize = 524288;
fn main() {
    let mut rec = vec![0u32; K];
    for i in 0..K { rec[i] = (((i as u32) * 31 + 7) % 251); }
    let mut sum: u64 = 0;
    let mut steps: u64 = 0;
    for i in 0..K { sum += rec[i] as u64; steps += 1; }
    println!("{} steps={}", sum, steps);
}
