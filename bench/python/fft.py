import math
N, R = 1024, 64
xr = [(i % 97) / 97.0 for i in range(N)]
xi = [0.0] * N
steps = 0
chk = 0
for r in range(R):
    for k in range(N):
        sr = si = 0.0
        for n in range(N):
            ang = -2.0 * math.pi * k * n / N
            sr += xr[n] * math.cos(ang) - xi[n] * math.sin(ang)
            si += xr[n] * math.sin(ang) + xi[n] * math.cos(ang)
            steps += 2
    chk += int(sr * 1000)
print(f"{chk} steps={steps}")
