f=open('/tmp/num.log','r')
lines = f.readlines()
sum=0
cnt=0
for l in lines:
    try:
        if(int(l,16) > 0):
            sum = sum + int(l,16)
            cnt += 1
    except Exception:
        continue

print sum/float(cnt)
f.close()
