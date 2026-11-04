import wave,struct,math,sys
def rms(f,ch=0,s=4410):
    w=wave.open(f);n=w.getnframes();X=struct.unpack('<%dh'%(n*2),w.readframes(n))[ch::2];r=[]
    for i in range(0,n-s,s):
        seg=X[i:i+s];m=sum(seg)/s;r.append(math.sqrt(sum((x-m)**2 for x in seg)/s))
    return r
def times(r,step=0.1):
    pk=max(r);i0=r.index(pk)
    t=lambda db:next((round(i*step,1) for i in range(i0,len(r)) if r[i]<pk*10**(db/20)),None)
    return round(i0*step,1),t(-6),t(-20),t(-40)
if __name__=='__main__':
    for f in sys.argv[1:]: print(f.split('/')[-1][:-4],'peak at %s, -6dB %s, -20dB %s, -40dB %s'%times(rms(f)))
