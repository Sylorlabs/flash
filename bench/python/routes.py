K=262144
lat=[0]*K; steps=0; chk=0
for r in range(64):
    for i in range(K):
        lat[i]=1; chk+=lat[i]&1; steps+=1
print(f"{chk} steps={steps}")
