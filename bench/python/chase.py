K=65536
nxt=[(i*48271+1)%K for i in range(K)]
p=0; steps=0
for _ in range(1000000):
    p=nxt[p]; steps+=1
print(f"{p} steps={steps}")
