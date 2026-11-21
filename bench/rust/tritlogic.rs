// Rust baseline: 1M digit-parallel trit ops emulated on binary.
const K: usize = 1000000;
fn main() {
    let mut a = vec![0i8; K]; let mut b = vec![0i8; K]; let mut d = vec![0i8; K];
    let mut steps: u64 = 0;
    let mut checksum: u64 = 0;
    for i in 0..K { a[i] = ((i % 3) as i8) - 1; b[i] = (((i * 7) % 3) as i8) - 1; }
    for i in 0..K {
        let mn = if a[i] < b[i] { a[i] } else { b[i] };
        d[i] = -mn; steps += 1;
        if d[i] == (checksum % 3) as i8 { checksum += d[i] as u64; }
    }
    println!("{} steps={}", checksum, steps);
}
