# X86MIDI

**[한국어](README.md)** | English

A **MIDI (.mid) file player** for MSX-DOS2 / Nextor, written in Z80 assembly.
It runs on X86MSX (an MSX emulator for PCs) and on MSX2-or-later machines, including the turbo R.

> Status: under development. The current build is a test version (V1.4.26T); it will be finalized after real-hardware verification.

## Features

- **Accurate timing**: playback beat accuracy is the top design priority.
- Five screens on **SCREEN 7 (512x212)**

  | Key | Screen | Contents |
  |-----|--------|----------|
  | F1 | Player | Playback status, progress, controls |
  | F2 | Roll | Piano roll |
  | F3 | Lyrics | Lyrics |
  | F4 | Files | File / folder list (shows song titles) |
  | F5 | Setup | Settings |

- Press **H** on the player screen for a shortcut-help popup.
- **Remembered song titles**: song info is saved per folder in `X86MIDI.DIR`, so the next run shows it immediately without re-reading the files (up to 128 songs per folder).
- The code is split into shared per-function `.inc` files (engine, screens, files, VDP, key handling, ...).

## Requirements

- MSX2 or later (MSX2+, turbo R included), or X86MSX
- MSX-DOS2 or Nextor
- A MIDI output device (or X86MSX's MIDI output)

## Run

```
X86MIDI.COM
```

Start it from the MSX-DOS2 prompt, then pick a `.mid` file on the F4 screen to play it.

## Build

[pasmo](https://pasmo.speccy.org/) is required.

```
pasmo -I . --bin X86MIDI.ASM x86midi.com x86midi.sym
```

`x86midi.com` is the executable and `x86midi.sym` is the symbol file.

## Source layout

| File | Role |
|------|------|
| `X86MIDI.ASM` | Main file; includes all modules |
| `engine.inc`, `tick.inc`, `isr.inc` | Playback engine, timing |
| `smf.inc`, `midi_out.inc` | MIDI file parsing, MIDI output |
| `scr_*.inc`, `screens.inc` | Per-screen drawing (Player, Roll, Lyrics, Files, Setup, ...) |
| `vdp7.inc`, `text7.inc`, `font.inc`, `pal.inc` | VDP, text output, font, palette |
| `info_file.inc` | Saving / restoring song info in `X86MIDI.DIR` |
| `dos.inc`, `bss.inc` | DOS call definitions, memory layout |

## Tested machines

DPC-200, CPC-400, FS-A1WX and MSX turbo R GT.

## Known limitations

- Song info is remembered for up to 128 songs per folder.
- `X86MIDI.DIR` is not saved while a song is playing.
- On write-protected disks, saving the info is silently skipped.

## Credits

- Author: KimTaeWoo
- Helper: Lee.J.H

The same credit lines are at the top of every source file (`X86MIDI.ASM`, `*.inc`).

## License

(Please choose a license and state it here.)
