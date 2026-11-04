import os
HERE=os.path.dirname(os.path.abspath(__file__))
IDX={l.split()[1][4:]:int(l.split()[0]) for l in open(os.path.join(HERE,'params.txt'))}
N=340
BACKUP=os.path.expanduser('~/Documents/Gear/lxr-02/card-2026-10-25/PROJ00/')
def load(path):
    d=open(path,'rb').read()
    p=bytearray(N); p[:len(d)-8]=d[8:8+N]
    return p
def setp(p,**kw):
    for k,v in kw.items():
        assert k in IDX, k
        assert 0<=v<=255, (k,v)
        p[IDX[k]]=v
def save(path,name,p):
    assert len(p)==N
    open(path,'wb').write(name.encode()[:8].ljust(8)+bytes(p))
# modTargets index for a drum (1-3): name -> offset within the voice's block
TGT=dict(cv=0,coarse=1,fine=2,wave=3,pwm=4,atk=5,dec=6,slope=7,pdec=8,pslope=9,pamt=10,veldest=11,velamt=12,velvol=13,
 fmamt=14,fmfreq=15,modwave=16,mode=17,twave=18,tvol=19,tfreq=20,cutoff=21,reso=22,ftype=23,fdrive=24,
 lfofreq=25,lfosync=26,lfoamt=27,lfowave=28,lforetrig=29,lfooffset=30,vol=31,pan=32,decim=33,drive=34)
def tgt(drum,name): return 2+40*(drum-1)+TGT[name]
LFO={n:i for i,n in enumerate('sin tri sup sdn sqr rnd xup xdn nse xtr si1 tr1 su1 sd1 sq1 xu1 xd1 xt1 rn1'.split())}
