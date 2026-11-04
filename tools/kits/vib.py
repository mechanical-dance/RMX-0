import wave,struct,math,sys
for f in sys.argv[1:]:
    w=wave.open(f);n=w.getnframes();X=struct.unpack('<%dh'%(n*2),w.readframes(n))[0::2]
    z=[i-1+X[i-1]/(X[i-1]-X[i]) for i in range(1,n) if X[i-1]<0<=X[i]]
    hz=[44100/(z[i+1]-z[i]) for i in range(len(z)-1) if 44100<z[i]<3*44100]
    hz.sort();lo,hi=hz[len(hz)//50],hz[-len(hz)//50]
    print(f,'centre %.2f Hz, vibrato ±%.1f cents'%((lo+hi)/2,600*math.log2(hi/lo)))
