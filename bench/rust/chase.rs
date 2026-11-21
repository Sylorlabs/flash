// Rust baseline: 1M serial dependent pointer-chase hops.
const K: usize = 65536;
fn main() {
    let mut nxt = vec![0u32; K];
    for i in 0..K { nxt[i] = (((i as u32) * 48271 + 1) % (K as u32)); }
    let mut p: u32 = 0;
    let mut steps: u64 = 0;
    for _ in 0..1000000 { p = nxt[p as usize]; steps += 1; }
    println!("{} steps={}", p, steps);
}
