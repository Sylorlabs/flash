K=524288
rec=[(i*31+7)%251 for i in range(K)]
s=0; steps=0
for i in range(K):
    s+=rec[i]; steps+=1
print(f"{s} steps={steps}")
