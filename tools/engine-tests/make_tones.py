# Builds the card folder retrig_test needs: 100 Hz test tones, plus samples/MODBASS.WAV and loops/AMENX4.WAV copied from
# your own card (any mono 16-bit WAVs under those names). Args: <card> <out>
import math, os, shutil, struct, sys, wave
src, out = sys.argv[1:3]
for d in ('samples', 'loops'):
    os.makedirs(os.path.join(out, d), exist_ok=True)
shutil.copy(os.path.join(src, 'samples/MODBASS.WAV'), os.path.join(out, 'samples/MODBASS.WAV'))
shutil.copy(os.path.join(src, 'loops/AMENX4.WAV'), os.path.join(out, 'loops/AMENX4.WAV'))
tone = b''.join(struct.pack('<h', int(32000*math.sin(2*math.pi*100*i/44100))) for i in range(44100))
for name in ('samples/SIN100.WAV', 'loops/SINLP.WAV'):
    with wave.open(os.path.join(out, name), 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(44100); w.writeframes(tone)
