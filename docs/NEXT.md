# Next work on lxr-0 (written 2026-11-01)

State: `main` is at 0.5.8, flashed by the user only. Work serially on `main` with no branches. Local commits only; no pushes.
The user flashes every image. Get a yes before changing behaviour beyond what's written here.

Order:
1. **0.4.13: retrigger clicks** (below). Built; waiting for the user to try it.
2. **0.5.0: voice links** (below). Built, plus 0.5.1 (global MIDI channel) and 0.5.2 (global channel 0 off, group LEDs) 0.5.3 (off on the knob) and 0.5.4 (glide); waiting for the user to try it. What shipped is in `PLAN.md`; the settings went on the drum transient page, not the MIX page.
3. **LFO shapes** (below). Approved in principle; behaviour needs the user's sign-off before any code.

**2026-11-03:** the user said features are complete. If 0.5.6 works on hardware, the next session is docs, polish and publishing. 0.5.7 then added per-drum glide and MIDI legato, at the user's request. User-facing feature docs so far: `FEATURES.md`. LFO shapes (3) are done in 0.5.5. All features are now complete.

These three finish the project; then the user is wrapping lxr-0 up. The sequencer from the original plan is dropped, and so are waveform modulation and trim modulation for samples (not needed).

**Fast memory:** 0.5.5 uses 14,872 of 16,384 bytes of ITCM (1,512 free; 0.5.4 used 14,840, 0.5.0 14,824, 0.4.14 used 14,752, 0.4.13 used 15,896, 0.4.12 14,144). New code goes in normal flash unless it runs per sample. Keep any per-sample additions tiny, and check render CPU for each one, using the reverb load guard's DWT timing.

## 0.4.13: retrigger clicks

### Problem
Retriggering a voice while it still sounds clicks. It's worst on long decays and low notes, and it stops the user making 808-style kicks or basslines: the case they named was a bassline from the `cw_modbass02` sample. Raising attack doesn't help.

### Cause, from reading the code (not yet measured)
- `Drum_trigger` (`DrumVoice.c`) always resets oscillator phase and sample position: its "only reset if the envelope is closed" check is compiled out, because `USE_AMP_FILTER` isn't defined. `Snare.c`, `CymbalVoice.c` and `HiHat.c` reset the phase on trigger too.
- `SVF_reset` also runs on each drum trigger, which may add a step if the filter is ringing. Unconfirmed.
- The old note's envelope is high when the waveform jumps, so the output steps instantly. Attack can't help, because the jump doesn't depend on the envelope.

### Scope
- Only clicks the retrigger adds. **A hit from silence must stay bit-identical to 0.4.12.** The user accepts that a sound that clicks from silence is just clicky. Don't touch attack-0 behaviour or start phases.
- **No added delay on any note.** Timing matters on a drum machine.

### Design
1. **Retrigger declick, all voices.** At a trigger, if the voice is sounding, keep its last final output sample (after filter, envelope, drive and volume) and add it to the new note's output, fading to zero over a few milliseconds. The new note's attack is then exactly what it would be from silence, and the old level fades instead of vanishing. Start around 2-5 ms and pick the shortest length that's clean on the low-note test. Triggers are processed at the start of a block (`voiceControl_processQueue` in `mixer_calcNextSampleBlock`), so this is block-aligned.
   Render points: `calcDrumVoiceSyncBlock`, and the sync blocks in `Snare.c`, `CymbalVoice.c`, `HiHat.c`. Check CPU in the load guard's terms: this should be a few operations per sample while a fade is active.
2. **Sample trim fades** (`lxr0_sampleOsc.c`): short fades at the trimmed start and end of one-shots, and a short crossfade at a trimmed loop point. Only when a trim is non-zero: an untrimmed sample plays exactly as now. Keep the start fade very short so attacks aren't softened. 0.4.11's cuts are hard and can click.

### Tests
Use the real engine (`tools/engine-tests/`, see below).
- Long-decay sine at about 40-60 Hz, retriggered at many points in its cycle, including peaks.
- `MODBASS` as a repeated bassline. Use the user's `cw_modbass02` if it's found; ask rather than hunting for it.
- Measure the largest sample-to-sample jump across the retrigger, before and after. It should be no bigger than the new note's own step from silence.
- Filter ringing case, to settle the `SVF_reset` question.
- First hits from silence on all voices: identical to 0.4.12.
- Trims: starts, ends and loop points without a step, and untrimmed samples identical to 0.4.12.
- Save renders for the user to listen to, at coarse 0.
Avoid long batches of reruns: the user asked for tests that work the first time. Run with a hard time limit and write output to a file.

### Test programs
`retrig_test.c` is the 0.4.13 test: it renders block by block on one thread, drops replayed blocks and reports stalls with stacks; `make_tones.py` builds its card folder.
`tools/engine-tests/` holds this session's programs, which load `engine-lxr0.dylib` from `~/code/synth/phx-d01/build` (`make -j8 build/engine-lxr0.dylib`). Build with `cc -O1`. Run them as `LXR0_DIR=<temp state dir> ./prog <dylib> <kit.SND> <folder with samples/ and loops/>`. Don't run with a shell `alarm` wrapper: one run ignored it and hung. Use a background kill loop.
Lessons: the Arcade kit's FX and click layer mask the sample unless they're zeroed. Zeroing `PAR_VOICE_DECIMATION*` silences a voice (leave it at 127). `PAR_OSC_WAVE_SNARE` is byte 4, `PAR_WAVE1_CYM` 6, `PAR_WAVE1_HH` 7; byte 5 is not a waveform. Edits made through the panel aren't known to `lxr_fw_edit`'s change tracking, so read the kit back to check them.

### Open from earlier
- One test run hung for over 7 minutes in 0.4.11 testing, and another run's kit read-back failed after writing wrong bytes (5 and 7). The cause of both is unknown. In 0.4.13 testing, one run of both 0.4.12 and 0.4.13 gave identical readings whatever the settings for its last tests, as if blocks were being replayed. It did not come back in the later full runs, which have a stall detector. A kit's noise sources make renders differ between runs of the same build, so compare with noise removed.
- Remove LXR-1 from PHX-D01 only on the user's clear go.

## 0.5.0: voice links

### Context
The original lxr-0 plan listed poly as a later goal. You've now specified it as "voice links": drums 1-3 can be grouped and played as one instrument. The sequencer is dropped from the plan. All of this is firmware; a kit with no link behaves exactly as it does today.

### Decisions from you
- Three modes, drum voices only. In edit mode, hold DRUM and select more than one drum.
- **Unison:** one trigger and CV play all linked voices. `det` detunes, `wid` pans.
- **Chord:** one trigger and CV play all linked voices. Intervals come from each voice's own tuning. `wid` still applies.
- **Poly:** you patch **one trigger and one pitch CV**. The firmware assigns each note to a voice, and that voice holds its pitch. When all voices are busy it replaces the oldest (fifo) or the highest note.
- **Sync (poly):** the whole sound follows voice 1: oscillator, envelope, modulation, FM, click, filter and drive. Pan and level stay per voice.
- **Group knobs:** a pot moves every linked voice by the same amount, so differences and chord intervals survive. Values stop at their limits.
- Double-tap a drum while holding DRUM to edit that voice alone. It flashes once per second and the others stay lit.

### Decisions I've made, for you to overrule
- **One group:** any 2 or 3 of drums 1-3.
- **Selecting:** the first press selects a voice immediately, as now, so normal selection gets no added delay. A second drum pressed while the first is held forms the group. Single-pressing a linked voice returns to the group view.
- **Unlinking:** the mode setting has a 4th value, `off`. Selecting a voice outside the group leaves the link running.
- **Settings screen:** `mod det wid syn` plus `alc` (fifo or highest), on an extra screen of the MIX page that appears only while a group is linked. It opens when a group forms.
- **Jacks:** the lowest linked drum's trigger, accent and pitch CV control the group. The other linked drums' trigger jacks are ignored while linked. Pitch CV is patched through the mod matrix as now.
- **MIDI:** the same allocation applies to notes on the lowest linked drum's channel.
- **Level:** unison and chord scale each voice by 1/sqrt(voices), so linking doesn't jump about 9.5 dB. Poly isn't scaled, because each note is one voice.
- **Turning sync on** copies voice 1 into the others straight away. This changes the kit in memory only, until you save it.

### Implementation
1. **Kit (`Inc/Parameters.h`, `menu.c`, `MenuText.h`, `menu.h`):** new `PAR_LXR0_LINK_*` bytes after the trims, for voices, mode, detune, width, sync and allocation. All zero means unlinked, so older kits load unchanged. Kit files grow from 333 to 339 bytes.
2. **Triggers (`MidiVoiceControl.c`, `voiceControl_queueTrigger`):** one routing point for jack and MIDI notes. Unison and chord fan out. Poly calls the allocator.
3. **Allocator (new `lxr0_voiceLink.c`):** picks a free voice first, judged by its amp envelope since these voices have no gate. If none is free, it applies fifo or highest. It latches pitch into the chosen voice.
4. **Pitch CV (`MidiParser.c`, where `Drum_setCv1VOct` is called):** unison and chord copy the lead voice's CV to the others, keeping each voice's tuning. Poly latches it per voice at the trigger.
5. **Detune and width:** a per-voice detune factor in the oscillator's frequency calculation (`Oscillator.c`, `lxr0_sampleOsc.c`, so samples follow). A pan offset in `Drum_updatePan`.
6. **Group edits and sync (`menu_parseKnobValue`, `menu.c`):** find the matching parameter on other voices through the same page, sub-page and slot in `menuPages`, then send it through the normal path. A check confirms the three drum pages line up, with an exclusion list for pan and level.
7. **Buttons and LEDs (`buttonHandler.c`, `ledHandler.c`):** group selection, double-tap window, and group LEDs while DRUM is held, reusing the existing blink code at a once-per-second rate. When DRUM is released, the buttons go back to showing pages, as they do now.
8. **Voice steal:** reuse the 0.4.13 retrigger declick, and only send a stolen note the same way as any retrigger. No delay.
9. **PHX-D01 (local `lxr0-engine` only):** grow the kit layout as with trim, so link bytes load, save and reach the firmware. No group UI in the app's own panels; the optional firmware panel gives the real controls.
10. **Release:** 0.5.0 image, `RELEASES.md`, `PLAN.md`, hub decisions line. Local commits only.

### Known limits
- LFO, velocity and mod matrix routes stay per voice, apart from pitch CV. A route to drum 1's filter won't move drum 2, even under sync. Mirroring routes can be a later change.
- Unison at zero detune is phase-locked, so it sounds like one louder voice until `det` is raised.

### Main risk
**CV timing in poly:** the trigger and a new pitch can arrive at nearly the same moment, and the CV reading may lag, which would give a note the previous pitch. Triggers already wait about 1 ms before firing. I'll measure this in the engine; on hardware it may need a slightly longer delay, in which case I'd add a small fixed one.

### Verification
One test program, run once per stage against the native engine, using the real panel calls (`lxr_fw_button`, `lxr_fw_pot`, trigger, CV):
- **Controls:** form a group, double-tap, unlink. Check the LCD, LEDs and voice selection.
- **Unison and chord:** one trigger sounds every linked voice, at the expected pitches for detune and intervals. Width changes left and right levels.
- **Poly:** with one trigger and a changing CV, notes land on successive voices and hold their pitch. Fifo and highest replace the right voice. MIDI does the same.
- **Group knobs and sync:** values move by the same amount and stop at limits. Sync covers the agreed parameters, and pan and level stay separate.
- **Clicks:** a stolen voice adds no step beyond what the new note has from silence.
- **Kits:** a linked kit saves and reloads exactly. 333-byte and older kits load unlinked.
- **No regression:** with no link, renders match 0.4.12 exactly.

You flash and test on hardware. The CV timing and the LED behaviour are the two things to check there first.

## LFO shapes (after voice links)
From the original plan: three new shapes, written from LXR02Open's `ENHANCED_FEATURES.md` description. That fork's code isn't in our trees, so this is new code.
- **One-shot:** runs one cycle from the trigger, then stops and holds, so it works as an extra envelope on any LFO target (filter sweep, pitch drop, pan, drive, FM).
- **Exponential-triangle:** a triangle with curved, snappier ramps.
- **Noise:** smooth random movement, unlike stepped random.

Steps:
1. **First:** read the existing LFO code and its shape list, and check whether `ENHANCED_FEATURES.md` is available. Then write exact behaviour for each shape: what it holds at the end, what retrigger and sync do, the curve, how smooth the noise is and its rate range. **Get the user's sign-off on that before writing code.**
2. **Kit:** the shapes should be new values of the existing LFO waveform parameter, so no new kit bytes. Older kits keep their shapes.
3. **Code:** shape maths runs per LFO update, not per sample, so it can live in flash. Check CPU anyway.
4. **Tests:** in the engine, plot each shape against the agreed behaviour, and confirm existing shapes are unchanged.
