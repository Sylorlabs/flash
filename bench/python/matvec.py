N,M=64,64
A=[(i*7+3)%97 for i in range(N*N)]
x=[(i*5+1)%89 for i in range(N)]
y=[0]*N; steps=0; chk=0
for m in range(M):
    for i in range(N):
        s=0
        for j in range(N):
            s+=A[i*N+j]*x[j]; steps+=1
        y[i]=s
    chk+=y[0]
print(f"{chk} steps={steps}")
