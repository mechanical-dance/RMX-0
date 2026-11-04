# lxr-0 0.5.7 showcase kits, slots 111-120. Run: python3 gen.py OUTDIR
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from kitlib import *

BASE = BACKUP + '85-ARCAD.SND'	# snare, cymbal and hats come from Arcade

def hz_to_lfo(hz):
	# lfo.c: f = 200*(1/128 + v/127*(1-1/128))^3
	x = (hz/200.)**(1/3.)
	return max(0, min(127, round((x - 0.0078125)/(1 - 0.0078125)*127)))

def drum(p, d, wave=0, coarse=60, fine=63, pwm=0, atk=0, dec=90, slope=64, pdec=0, pamt=0, pslope=0,
		fm=0, fmnote=None, modwave=0, mix=0, tvol=0, twave=0, tfreq=0, cutoff=127, reso=0, ftype=0, fdrive=0,
		vol=100, pan=63, drive=0, out=0, verb=0, veldest=None, velamt=0, velvol=1):
	setp(p, **{f'OSC_WAVE_DRUM{d}': wave, f'COARSE{d}': coarse, f'FINE{d}': fine, f'PWM_{d}': pwm,
		f'VELOA{d}': atk, f'VELOD{d}': dec, f'VOL_SLOPE{d}': slope, f'MOD_EG{d}': pdec, f'MODAMNT{d}': pamt, f'PITCH_SLOPE{d}': pslope,
		f'FMAMNT{d}': fm, f'FM_FREQ{d}': coarse if fmnote is None else fmnote, f'MOD_WAVE_DRUM{d}': modwave, f'MIX_MOD_{d}': mix,
		f'TRANS{d}_VOL': tvol, f'TRANS{d}_WAVE': twave, f'TRANS{d}_FREQ': tfreq,
		f'FILTER_FREQ_{d}': cutoff, f'RESO_{d}': reso, f'FILTER_TYPE_{d}': ftype, f'FILTER_DRIVE_{d}': fdrive,
		f'VOL{d}': vol, f'PAN{d}': pan, f'DRIVE{d}': drive, f'AUDIO_OUT{d}': out, f'LXR0_VERB_SEND{d}': verb,
		f'VOICE_DECIMATION{d}': 127, f'VEL_DEST_{d}': 0 if veldest is None else tgt(d, veldest), f'VELO_MOD_AMT_{d}': velamt,
		f'VOLUME_MOD_ON_OFF{d}': velvol, f'LXR0_TRIM_START{d}': 0, f'LXR0_TRIM_END{d}': 0,
		f'AMOUNT_LFO{d}': 0, f'VOICE_LFO{d}': d, f'TARGET_LFO{d}': 0, f'RETRIGGER_LFO{d}': 0})

def route(p, n, voice, target, depth):
	# a mod matrix slot: LFO n (1-6) to drum `voice`'s `target`, depth 0-100 (bipolar around the knob value).
	# In this firmware the LFO page's own target and amount do nothing; the matrix routes LFOs.
	for i in range(1, 19):
		if p[IDX[f'CV_MOD_AMT_{i}']] == 0:
			setp(p, **{f'CV_SRC_{i}': 4 + n, f'CV_DEST_{i}': tgt(voice, target), f'CV_MOD_AMT_{i}': depth})
			return
	raise SystemExit('mod matrix full')

def lfo(p, n, voice, target, shape, hz=None, amount=0, retrig=0, offset=0, sync=0, rate=None):
	# LFO n (1-6): its shape and rate on the LFO page, routed to drum `voice`'s `target` in the matrix.
	# amount is the depth as a fraction of 127 (63 swings a centred knob end to end).
	setp(p, **{f'WAVE_LFO{n}': LFO[shape], f'FREQ_LFO{n}': rate if rate is not None else hz_to_lfo(hz), f'AMOUNT_LFO{n}': amount,
		f'VOICE_LFO{n}': voice, f'TARGET_LFO{n}': tgt(voice, target), f'RETRIGGER_LFO{n}': retrig, f'OFFSET_LFO{n}': offset, f'SYNC_LFO{n}': sync})
	route(p, n, voice, target, min(100, round(amount/127*100)))

def link(p, voices=7, mode=0, detune=0, width=0, sync=0, alloc=0):
	setp(p, LXR0_LINK_VOICES=voices, LXR0_LINK_MODE=mode, LXR0_LINK_DETUNE=detune, LXR0_LINK_WIDTH=width,
		LXR0_LINK_SYNC=sync, LXR0_LINK_ALLOC=alloc)

def glide(p, d, amount=0, typ=0, legato=0):
	g = ['LXR0_LINK_GLIDE', 'LXR0_LINK_GLIDE2', 'LXR0_LINK_GLIDE3'][d-1]
	t = ['LXR0_LINK_GLIDE_TYPE', 'LXR0_LINK_GLIDE_TYPE2', 'LXR0_LINK_GLIDE_TYPE3'][d-1]
	setp(p, **{g: amount, t: typ, f'LXR0_LINK_LEGATO{d}': legato})

def verb(p, amount=100, predelay=10, decay=90, tone=64, refl=40):
	setp(p, LXR0_VERB_AMOUNT=amount, LXR0_VERB_ROUTING=0, LXR0_VERB_PREDELAY=predelay, LXR0_VERB_DECAY=decay,
		LXR0_VERB_TONE=tone, LXR0_VERB_REFLECTION=refl)

def delay(p, wet=30, time=80, rng=1, tone=60, feedback=60, pingpong=1):
	setp(p, FX_TYPE=4, FX_ROUTING=0, FX_DRY_WET=wet, FX_DELAY_TYPE=pingpong, FX_P1=time, FX_P2=rng, FX_P3=tone, FX_P4=feedback)

def perc(p, level=1.0, out=None, verbsend=0):
	# Arcade's snare, cymbal and hats, scaled; out None keeps Arcade's routing
	for v in (4, 5, 6):
		setp(p, **{f'VOL{v}': round(p[IDX[f'VOL{v}']]*level), f'LXR0_VERB_SEND{v}': verbsend})
		if out is not None:
			setp(p, **{f'AUDIO_OUT{v}': out})

def vibrato(p, hz=5.0, amount=8):
	# the snare, cymbal and hat LFOs, each pointed at one drum's fine tune
	for n, d in ((4, 1), (5, 2), (6, 3)):
		lfo(p, n, d, 'fine', 'sin', hz=hz*(1 + 0.03*(d-2)), amount=amount)

def base():
	p = load(BASE)
	glide(p, 1); glide(p, 2); glide(p, 3)
	link(p, voices=0)
	setp(p, FX_TYPE=0, FX_DRY_WET=0)
	for i in range(1, 19):
		setp(p, **{f'CV_SRC_{i}': 0, f'CV_DEST_{i}': 0, f'CV_MOD_AMT_{i}': 0})
	return p

KITS = []
def kit(slot, name, notes):
	def wrap(f):
		KITS.append((slot, name, f, notes)); return f
	return wrap

@kit(111, 'Lush Am', '''Chord mode, lush. An open A minor (A2, E3, C4): three saws, each doubled an octave up in mix mode, slow
attack, about 7 s of decay, wide spread. Each drum's cutoff drifts on its own smooth-random LFO (`nse`), so the chord
never sits still, and the snare, cymbal and hat LFOs add a slow, slightly different vibrato to each voice. Ping-pong
delay and a long plate.
- Play drum 1 (trigger or MIDI): one hit plays the chord. MIDI notes transpose it.
- Patch ideas: retune drums 2-3 (coarse) for other chords; N+5/N+10 gives a sus/7 colour. Turn `wid` down for a mono
  pad. Shorten decay and add delay feedback for dub chords.''')
def lush(p):
	link(p, voices=7, mode=2, width=115)
	for d, c, f in ((1, 45, 61), (2, 52, 66), (3, 60, 63)):
		drum(p, d, wave=2, coarse=c, fine=f, atk=75, dec=108, slope=70, fm=45, fmnote=c+12, modwave=2, mix=1,
			cutoff=58, reso=22, fdrive=10, vol=110, out=6, verb=85)
	lfo(p, 1, 1, 'cutoff', 'nse', hz=0.21, amount=24)
	lfo(p, 2, 2, 'cutoff', 'nse', hz=0.17, amount=22)
	lfo(p, 3, 3, 'cutoff', 'nse', hz=0.13, amount=26)
	vibrato(p, hz=4.6, amount=25)
	delay(p, wet=32, time=95, tone=45, feedback=62)
	verb(p, amount=115, predelay=28, decay=112, tone=52, refl=62)
	perc(p, level=0.7, out=0)

@kit(112, 'Swap Am', '''Your swap chord. A minor (A3, C4, E4) in chord mode, soft triangles through a gentle low-pass. Drum 1, the root,
sits in the middle; drums 2 and 3 start hard left and hard right on every chord, from sine LFOs on their pans half a
cycle apart (offsets 0 and 50 %, retriggered by drum 1), and swap sides over about 6 s, back over the next 6. The snare, cymbal and hat LFOs give each voice its own
subtle vibrato.
- Needs 0.5.8 (pan follows its LFO while a note sounds). On older firmware the outer voices stay where each chord
  starts.
- Patch ideas: set the pan LFOs' retrigger to off to have each new chord land wherever the LFOs are (a per-hit swap);
  raise the vibrato amounts (the `lf4`-`lf6` mod matrix slots) for a chorus feel.''')
def swap(p):
	link(p, voices=7, mode=2, width=0)
	for d, c in ((1, 57), (2, 60), (3, 64)):
		drum(p, d, wave=1, coarse=c, atk=55, dec=106, slope=70, fm=30, fmnote=c+12, modwave=1, mix=1,
			cutoff=80, reso=10, vol=110, out=0, verb=70)
	lfo(p, 2, 2, 'pan', 'sin', hz=0.083, amount=64, retrig=1, offset=0)
	lfo(p, 3, 3, 'pan', 'sin', hz=0.083, amount=64, retrig=1, offset=64)
	lfo(p, 1, 1, 'cutoff', 'nse', hz=0.15, amount=14)
	vibrato(p, hz=5.2, amount=28)
	verb(p, amount=105, predelay=20, decay=108, tone=58, refl=55)
	perc(p, level=0.7)

@kit(113, 'Glass', '''Poly MIDI, ethereal. FM glass bells: a sine carrier with a 3.5:1 sine modulator, velocity on level. Each note lands at a new random place in the stereo field (`rn1` on pan, restarted by each
drum's own notes), and a slow smooth-random drift on each drum's fine tune keeps it organic. Long plate, ping-pong.
- Set drum 1's MIDI note to `any`. Coarse is 60, so MIDI notes play at their own pitch. `syn` is on, so knob edits keep
  all three drums the same.
- Patch ideas: change drum 1's FM freq for other bell ratios (+12 is a clean octave, +19 a fifth above that); raise
  decay for drones; `alc hi` to keep a bass note while arpeggiating above it.''')
def glass(p):
	link(p, voices=7, mode=3, sync=1, alloc=0)
	for d in (1, 2, 3):
		drum(p, d, wave=0, coarse=60, atk=6, dec=100, slope=82, fm=38, fmnote=82, modwave=0, cutoff=118,
			vol=105, out=6, verb=100)
		lfo(p, d, d, 'pan', 'rn1', hz=3.0, amount=55, retrig=d)
		lfo(p, d + 3, d, 'fine', 'nse', hz=0.3, amount=14)
	delay(p, wet=28, time=70, tone=75, feedback=55)
	verb(p, amount=120, predelay=40, decay=118, tone=76, refl=70)
	perc(p, level=0.6, out=0, verbsend=40)

@kit(114, 'Halo', '''Poly MIDI pad, pretty. A triangle with a saw an octave up mixed in, slow attack, long tail. Each note's filter
breathes: an exponential triangle (`xtr`) on cutoff, restarted by each note, opens over about a second and closes again,
so every note blooms on its own. Gentle vibrato from the snare, cymbal and hat LFOs; big reverb.
- Drum 1's MIDI note `any`; coarse 60 plays MIDI pitch. `syn` on.
- Patch ideas: lengthen the LFO rate for slower blooms; set the LFO shape to `tr1` for a one-time swell per note that
  then holds; `alc old` vs `hi` changes which note a 4th voice steals.''')
def halo(p):
	link(p, voices=7, mode=3, sync=1)
	for d in (1, 2, 3):
		drum(p, d, wave=1, coarse=60, atk=82, dec=110, slope=70, fm=32, fmnote=72, modwave=2, mix=1,
			cutoff=48, reso=30, fdrive=8, vol=112, out=0, verb=95)
		lfo(p, d, d, 'cutoff', 'xtr', hz=0.45, amount=38, retrig=d)
	vibrato(p, hz=4.8, amount=22)
	verb(p, amount=118, predelay=35, decay=116, tone=60, refl=66)
	perc(p, level=0.6, out=0)

@kit(115, 'OneShots', '''The one-shot LFO shapes as extra envelopes.
- Drum 1: saw bass with a resonant low-pass. `xd1` on cutoff is a second, snappier filter envelope on every hit.
- Drum 2: sine tom whose pitch rises after the hit (`su1` on coarse): a reverse tom.
- Drum 3: FM perc with `sq1` on its volume, a gate that chops the tail at half the LFO cycle; the LFO rate sets the gate
  length. The snare's LFO adds random pitch per hit to drum 3 (`rn1` on its coarse).
- Arcade's snare, cymbal and hats.
- Patch ideas: offset on a one-shot is a delay before it fires; try `tr1` on drum 1's cutoff for a wah per note, or
  `xu1` on drum 2's coarse for a riser.''')
def oneshots(p):
	drum(p, 1, wave=2, coarse=36, atk=0, dec=78, slope=60, cutoff=22, reso=70, fdrive=25, vol=110, out=0, verb=10)
	lfo(p, 1, 1, 'cutoff', 'xd1', hz=2.2, amount=60, retrig=1)
	drum(p, 2, wave=0, coarse=45, atk=0, dec=72, slope=55, vol=105, pan=50, out=0, verb=30)
	lfo(p, 2, 2, 'coarse', 'su1', hz=1.8, amount=10, retrig=2)
	drum(p, 3, wave=0, coarse=72, atk=0, dec=95, slope=40, fm=70, fmnote=91, modwave=3, cutoff=110, vol=95, pan=78, out=0, verb=25)
	lfo(p, 3, 3, 'vol', 'sq1', hz=4.0, amount=64, retrig=3)
	lfo(p, 4, 3, 'coarse', 'rn1', hz=12.0, amount=7, retrig=3)
	verb(p, amount=90, predelay=8, decay=85, tone=64, refl=40)

@kit(116, 'SuprSaw', '''Unison lead. Three saws in unison mode, detune 70, wide, with an exponential glide and legato on drum 1.
Over MIDI, overlapping notes slide without retriggering; detached notes re-attack. Each drum's cutoff wanders on a
smooth-random LFO at a slightly different rate, so the stack moves like separate oscillators. Ping-pong delay.
- Drum 1's MIDI note `any`; coarse 60 plays MIDI pitch.
- Patch ideas: glide type `lct` for acid-style equal-time slides; `det` 20 for a tighter stack; switch the link to chord
  (`chd`) and set drums 2-3 coarse to 67/72 for power-chord leads; legato off for retriggered arps.''')
def suprsaw(p):
	link(p, voices=7, mode=1, detune=70, width=110)
	for d in (1, 2, 3):
		drum(p, d, wave=2, coarse=60, atk=6, dec=112, slope=64, cutoff=72, reso=32, fdrive=12, vol=112, out=6, verb=45)
		lfo(p, d, d, 'cutoff', 'nse', hz=0.35 + 0.08*d, amount=16)
	glide(p, 1, amount=55, typ=2, legato=1)
	delay(p, wet=26, time=60, tone=55, feedback=48)
	verb(p, amount=95, predelay=15, decay=96, tone=60, refl=45)
	perc(p, level=0.8, out=0)

@kit(117, '808Slide', '''Pitched 808 programming with per-drum glide and legato.
- Drum 1: 808 bass (sine, short pitch punch, light drive, about 3 s), exponential glide, legato on. Over MIDI,
  overlapping notes slide without a new hit: hold a note, play the next one, release the first.
- Drum 2: tom with its own linear glide (`lcr`), so tom fills slide too; no legato, every hit re-attacks.
- Drum 3: FM zap, no glide.
- Arcade's snare, cymbal and hats.
- Patch ideas: glide type `lct` for equal-time slides whatever the interval; legato on drum 2 for sliding tom rolls;
  raise drum 1's decay toward 110 for long subs.''')
def slide808(p):
	drum(p, 1, wave=0, coarse=36, atk=0, dec=96, slope=55, pdec=40, pamt=58, pslope=8, tvol=35, twave=5, tfreq=40,
		drive=28, vol=115, out=0, verb=0)
	glide(p, 1, amount=50, typ=2, legato=1)
	drum(p, 2, wave=0, coarse=48, atk=0, dec=74, slope=50, pdec=45, pamt=50, pslope=6, tvol=25, twave=12, tfreq=60,
		vol=100, pan=48, out=0, verb=25)
	glide(p, 2, amount=40, typ=0, legato=0)
	drum(p, 3, wave=0, coarse=69, atk=0, dec=55, slope=40, pdec=30, pamt=80, pslope=10, fm=60, fmnote=88, modwave=0,
		vol=95, pan=80, out=0, verb=20)
	verb(p, amount=80, predelay=6, decay=80, tone=64, refl=35)

@kit(118, 'LiveKit', '''A drum kit that never plays the same hit twice, from the new LFO shapes running free.
- Drum 1: kick with smooth random (`nse`) on its decay, so each kick is a little longer or shorter.
- Drum 2: FM tom with an exponential triangle (`xtr`) slowly sweeping its FM amount, from soft to metallic and back
  over about 8 s.
- Drum 3: blip with random per hit (`rn1`) on coarse: a new pitch on each hit, gliding briefly into it.
- Arcade's snare, cymbal and hats.
- Patch ideas: set any LFO's sync to a note value to lock the movement to the clock; swap `nse` for `rnd` for a
  stepped, sample-and-hold feel.''')
def livekit(p):
	drum(p, 1, wave=0, coarse=33, atk=0, dec=64, slope=50, pdec=38, pamt=70, pslope=7, tvol=40, twave=5, tfreq=45,
		drive=20, vol=115, out=0)
	lfo(p, 1, 1, 'dec', 'nse', hz=0.7, amount=14)
	drum(p, 2, wave=0, coarse=50, atk=0, dec=70, slope=50, pdec=40, pamt=45, pslope=6, fm=30, fmnote=69, modwave=0,
		vol=100, pan=45, out=0, verb=25)
	lfo(p, 2, 2, 'fmamt', 'xtr', hz=0.125, amount=30)
	drum(p, 3, wave=1, coarse=76, atk=0, dec=45, slope=40, pdec=20, pamt=20, pslope=5, vol=90, pan=82, out=0, verb=35)
	lfo(p, 3, 3, 'coarse', 'rn1', hz=25.0, amount=12, retrig=3)
	verb(p, amount=85, predelay=8, decay=82, tone=64, refl=40)

@kit(119, 'DubStab', '''Chord mode, dub techno. C minor 7 shell (C3, Eb3, Bb3), pulse waves into a resonant low-pass with drive,
about a second long. Each drum's cutoff drifts on its own smooth-random LFO, so every stab is voiced a little
differently and the delay tails keep moving. Dark ping-pong delay with high feedback, plus plate.
- Play drum 1; MIDI notes transpose the chord.
- Patch ideas: ride the cutoff (group edit moves all three); feedback toward 100 for runaway dub; retune drum 3 to N+7
  for a plain minor triad, or N+5/N+10 for a sus4 7.''')
def dubstab(p):
	link(p, voices=7, mode=2, width=90)
	for d, c in ((1, 48, ), (2, 51), (3, 58)):
		drum(p, d, wave=3, coarse=c, pwm=40, atk=0, dec=74, slope=55, cutoff=46, reso=55, fdrive=30, vol=110,
			out=6, verb=55)
		lfo(p, d, d, 'cutoff', 'nse', hz=0.25 + 0.06*d, amount=20)
	delay(p, wet=45, time=88, tone=30, feedback=84)
	verb(p, amount=95, predelay=12, decay=104, tone=45, refl=50)
	perc(p, level=0.8, out=0)

@kit(120, 'PolyPluk', '''Poly MIDI plucks for arpeggios, pretty. A triangle with an octave FM shimmer; a one-shot exponential fall
(`xd1`) on each drum's cutoff is a filter envelope per note; velocity sets the level. The snare, cymbal
and hat LFOs throw each note to a new random spot in the stereo field (`rn1` on pan). Ping-pong delay and plate.
- Drum 1's MIDI note `any`; coarse 60 plays MIDI pitch. `syn` on. Fast arps keep up to three notes ringing.
- Patch ideas: longer decay for harp-like overlaps; `alc hi` keeps the lowest notes of a chord when arpeggiating
  above them; set the pan LFOs to `nse` for a smooth sweep instead of random spots.''')
def polypluk(p):
	link(p, voices=7, mode=3, sync=1)
	for d in (1, 2, 3):
		drum(p, d, wave=1, coarse=60, atk=0, dec=76, slope=60, fm=24, fmnote=72, modwave=0, cutoff=62, reso=38,
			fdrive=20, drive=25, vol=127, out=6, verb=70)
		lfo(p, d, d, 'cutoff', 'xd1', hz=2.6, amount=55, retrig=d)
		lfo(p, d + 3, d, 'pan', 'rn1', hz=6.0, amount=50, retrig=d)
	delay(p, wet=30, time=66, tone=65, feedback=52)
	verb(p, amount=110, predelay=25, decay=108, tone=66, refl=60)
	perc(p, level=0.7, out=0)

TNAME = {v: k for k, v in TGT.items()}
TLONG = dict(coarse='coarse', fine='fine tune', pan='pan', cutoff='filter cutoff', vol='volume', dec='decay', fmamt='FM amount')
def routes(p):
	out = []
	for i in range(1, 19):
		amt = p[IDX[f'CV_MOD_AMT_{i}']]
		if not amt: continue
		src = p[IDX[f'CV_SRC_{i}']]; dst = p[IDX[f'CV_DEST_{i}']] - 2
		n = src - 4; shape = [k for k, v in LFO.items() if v == p[IDX[f'WAVE_LFO{n}']]][0]
		t = TNAME[dst % 40]
		out.append(f'{i}: `lf{n}` ({shape}) to drum {dst//40 + 1} {TLONG.get(t, t)}, {amt}')
	return out

HEADER = '''# lxr-0 kits 111-120

Ten kits for lxr-0 0.5.8 that show chord and poly links, per-drum glide and legato, the new LFO shapes and pan that
follows its LFO while a note sounds. They are in `PROJ00` as `111-` to `120-`; load them from the kit menu.

- **Firmware:** made for 0.5.8. On 0.5.7 they load and play, but pan moves only on new hits (`Swap Am` needs 0.5.8),
  and linked groups play about 5 dB quieter.
- **Levels:** kept hot on purpose; trim on the device.
- **LFOs:** in this firmware an LFO reaches a sound only through the mod matrix (source `lf1`-`lf6`). Each kit lists
  its matrix slots. The LFO page sets each LFO's shape, rate, retrigger and offset.
- **Snare, cymbal and hats** come from Arcade (85). Some kits borrow their LFOs for vibrato or pan, so those voices'
  own LFOs are taken.
- **MIDI:** poly and lead kits are tuned so MIDI notes play at their own pitch (coarse 60). Set drum 1's MIDI note to
  `any`. Glide and legato are per drum, on the pitch mod page, screen 2.
'''

if __name__ == '__main__':
	out = sys.argv[1]
	os.makedirs(out, exist_ok=True)
	md = [HEADER]
	for slot, name, f, notes in KITS:
		p = base(); f(p)
		fn = f'{slot}-{name[:4].upper().replace(" ", "_")}.SND'
		save(os.path.join(out, fn), name, p)
		print(fn, name)
		md.append(f'## {slot} {name} (`{fn}`)\n\n{notes}\n')
		r = routes(p)
		if r:
			md.append('Mod matrix:\n' + '\n'.join('- ' + x for x in r) + '\n')
	open(os.path.join(out, 'KITS.md'), 'w').write('\n'.join(md))
