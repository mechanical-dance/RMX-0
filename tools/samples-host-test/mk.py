import struct, math, os
def wav(path, rate, ch, bits, frames, ext=False, junk=False, f=lambda i,c: 0.5*math.sin(i*0.05)):
    d=bytearray()
    for i in range(frames):
        for c in range(ch):
            v=int(f(i,c)*32767)
            d+= struct.pack('<h',v) if bits==16 else b'\x55'+struct.pack('<h',v)
    fmt=struct.pack('<HHIIHH',0xFFFE if ext else 1,ch,rate,rate*ch*bits//8,ch*bits//8,bits)
    if ext: fmt+=struct.pack('<HIH',22,bits,3)+struct.pack('<H',1)+b'\0'*14
    body=b'WAVE'
    if junk: body+=b'JUNK'+struct.pack('<I',5)+b'xxxxx\0'
    body+=b'fmt '+struct.pack('<I',len(fmt))+fmt+b'data'+struct.pack('<I',len(d))+bytes(d)
    open(path,'wb').write(b'RIFF'+struct.pack('<I',len(body))+body)
S='card/samples/'; L='card/loops/'
os.makedirs(S, exist_ok=True); os.makedirs(L, exist_ok=True)
for p in os.listdir(S)+os.listdir(L):
    for x in (S,L):
        if os.path.exists(x+p): os.remove(x+p)
wav(S+'ZKICK.WAV',44100,1,16,1000,f=lambda i,c: i/2000.)        # ramp, sorts last
wav(S+'ALONG.WAV',44100,1,16,100000,f=lambda i,c: ((i%20000)/20000.-0.5))  # past the old 32768 limit
wav(S+'BSTER.WAV',22050,2,16,1001,f=lambda i,c: 0.5 if c==0 else -0.25)     # odd frame count, L/R differ
wav(S+'C24.WAV',48000,2,24,500,ext=True,junk=True,f=lambda i,c: 0.25)
open(S+'NOTWAV.WAV','wb').write(b'not a riff file at all')
wav(S+'MONO8.WAV',44100,1,16,10); data=bytearray(open(S+'MONO8.WAV','rb').read()); data[34]=8; open(S+'MONO8.WAV','wb').write(data) # 8 bit: rejected
wav(L+'LOOP.WAV',44100,1,16,100,f=lambda i,c: i/200.)
for d in ('empty/samples','empty/loops','big/samples','big/loops'):
    os.makedirs(d, exist_ok=True)
for n in 'AB':
    d=b'\x10\x00'*600000; fmt=struct.pack('<HHIIHH',1,1,44100,88200,2,16)
    b=b'WAVEfmt '+struct.pack('<I',16)+fmt+b'data'+struct.pack('<I',len(d))+d
    open(f'big/samples/{n}.WAV','wb').write(b'RIFF'+struct.pack('<I',len(b))+b)
