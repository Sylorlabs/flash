K=1000000
steps=0; chk=0
for i in range(K):
    a=(i%3)-1; b=((i*7)%3)-1
    d=-min(a,b); steps+=1
    if d==chk%3: chk+=d
print(f"{chk} steps={steps}")
