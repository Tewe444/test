# SKY DASH — Sky Games (Nintendo DS / DSi Homebrew)

Sky Dash is an offline, native ARM9 homebrew project written in C for the
Nintendo DS/DSi. It is a rhythm-platformer-inspired cube runner with a 20-level
campaign, original Sky Games branding, and a replaceable song for every level.

> **Build status:** this is an expanded source project. The source has been
> reviewed and configuration-checked, but no `.nds` build or on-hardware test
> could be run here because devkitPro/devkitARM and `mmutil` are not installed
> in this environment.

## What's included

- Exactly **20 normal campaign levels**, each with its own layout, difficulty,
  speed, colour accent and music assignment.
- Dual-screen 16-bit bitmap rendering with an animated neon skyline, parallax
  blocks, stars, glowing obstacles and a rotating Sky Dash cube.
- Cube jump physics, collision detection, spikes, double spikes and blocks.
- S-block visual marker, ready for a future Dash Orb mechanic.
- Title screen, 20-level selection, song preview, pause, crash/retry and clear
  screens.
- Maxmod integration for looping XM music plus four event sound effects
  (level enter, level exit, crash and level clear); pause/resume/stop are wired in.
- All **34 XM modules** and four gameplay SFX WAV files from the
  Geometry Dash Advance ZIP you supplied; their audio data is byte-identical.
- A replaceable music pipeline: drop another `.xm` into `assets/audio/`, rebuild,
  and assign its generated `MOD_...` ID to a level in `source/levels.c`.
- A 32×32 Nintendo DS banner icon (`data/icon.bmp`), a larger PNG (`data/icon.png`), and an editable sprite-sheet source (`assets/graphics/sky_dash_spritesheet.png`).
- GitHub Actions build workflow for a downloadable `.nds` artifact.
- No Wi-Fi, accounts, online services, remote APIs or online level browser.

## Controls

| Input | Action |
|---|---|
| A / B / X / Y / Up | Jump |
| Touch screen while playing | Jump |
| Start | Pause / resume |
| Left / L | Previous level |
| Right / R | Next level |
| A | Play / retry / continue |
| B | Return to level selection |

## The 20 levels

1. First Flight — Stereo Madness
2. Blue Horizon — Back on Track
3. Prism Peak — Polargeist
4. Dust Trail — Dry Out
5. Baseline — Base After Base
6. Night Shift — Can't Let Go
7. Cloud Hop — Jumper
8. Time Loop — Time Machine
9. Cycle Drive — Cycles
10. X-Step — xStep
11. Neon Clutter — Clutterfunk
12. Theory — Theory of Everything
13. Voltage — Electroman Adventures
14. Dark Signal — Clubstep
15. Electro Rush — Electrodynamix
16. Hexa Force — Hexagon Force
17. Blast Off — Blast Processing
18. Second Theory — Theory of Everything 2
19. Faura Sky — Faura Slow
20. Moonlight — Death Moon

The level layouts are original Sky Dash data in `source/levels.c`; the song files
are modules from the Geometry Dash Advance archive you supplied. All 34 XM modules
are bundled; 20 are assigned as the initial campaign songs. The other 14 are
available to assign to any level too: Clown Party, Every End, Final Battle, Ground
to Space, Inferno, Main Menu, Practice, Problematic, Pyrophoric, Rose, RTAY,
Select Payment Type, Stereo Madness 2 and Ultimate Destruction.

## Add your own songs

1. Put a tracker module such as `MY_TRACK.xm` into `assets/audio/`.
2. Keep the filename simple (letters, numbers and underscores; no spaces).
3. Run `make clean`, then `make`. Maxmod's `mmutil` rebuilds the soundbank.
4. Open `source/levels.c` and replace the `MOD_...` value for the chosen level
   with the new generated ID (normally `MOD_MY_TRACK`).
5. Rebuild. The level will now play your song.

More detailed instructions: `docs/ADDING_SONGS.md`.

## Build locally

1. Install devkitPro with devkitARM, libnds, Maxmod, libfat/filesystem and the
   DS build tools (including `mmutil` and `grit`).
2. Open a devkitPro shell in this project folder.
3. Run `make`.
4. The output file is `sky_dash.nds`.
5. Run `make clean` before a clean rebuild if you changed the audio list.

The build follows the devkitPro `ds_rules` template, uses Maxmod for XM music,
and packages the generated soundbank in NitroFS. References:
- [devkitPro DS example Makefile](https://github.com/devkitPro/nds-examples/blob/master/templates/arm9/Makefile)
- [Maxmod formats and soundbank tool](https://github.com/blocksds/mmutil)
- [BlocksDS base-library overview](https://blocksds.skylyrac.net/docs/libs/core_libs/)

## Build on GitHub

1. Create a repository and upload the contents of this folder.
2. Open **Actions**.
3. Run **Build Sky Dash for Nintendo DS / DSi** (or push a commit).
4. Download the `SkyDash-DSi` artifact from the completed workflow run. It contains
   `sky_dash.nds`.

The workflow verifies the song/level configuration and then invokes `make` in
the `devkitpro/devkitarm` container.

## Folder layout

```text
SkyDash-DSi-Source/
├── .github/workflows/build.yml
├── assets/audio/              # 34 XM songs + 4 SFX WAVs
├── assets/graphics/           # original editable sprite-sheet source
├── data/icon.bmp              # 32x32 NDS banner icon source
├── data/icon.png              # larger icon artwork
├── docs/
│   ├── ADDING_SONGS.md
│   ├── CODE_REVIEW.md
│   ├── GDA_AUDIO_CREDITS.md
│   ├── GRAPHICS.md
│   ├── REFERENCE.md
│   └── CONTROLS_AND_GAMEPLAY.md
├── include/
│   ├── audio.h
│   ├── game.h
│   └── render.h
├── nitrofs/                   # build creates soundbank.bin here
├── source/
│   ├── audio.c
│   ├── game.c
│   ├── levels.c
│   ├── main.c
│   └── render.c
├── tools/verify_project.py
└── Makefile
```

## S-Block note

I interpreted the earlier term “S-Morse” as the Geometry Dash **S-Block /
Stop Block**. A visual, non-lethal S-block marker is included in some levels.
The reference archive declares an `S_BLOCK` object type, but the provided code has no implemented sprite or effect for it. Sky Dash currently draws a distinct yellow S-block marker. It is non-lethal until Dash Orb gameplay exists, so it cannot yet stop a Dash Orb effect.

## Audio attribution and publishing

The 34 XM files and four SFX WAV files were copied byte-for-byte from the
Geometry Dash Advance ZIP you supplied. The WAV filenames are uppercased so
Maxmod generates predictable `SFX_...` identifiers; the audio data is unchanged.
The original audio-credit list is kept in `docs/GDA_AUDIO_CREDITS.md`.

Important: the Geometry Dash Advance repository's MIT license covers its code;
its audio credits list composers and cover artists but does not itself grant a
blanket music license. Verify rights and attribution requirements for each
track before publishing the game or sharing a public binary.

## Known limits

- The project is a native DS/DSi prototype, not a one-to-one full Geometry Dash
  port. It currently implements Cube mode, spikes, blocks, and 20 original
  layouts.
- The S-Block is a visual placeholder until Dash Orb mode exists.
- Best percentages and deaths are session-only; persistent save data is not
  implemented yet.
- No compile/hardware validation was possible in this environment. Use melonDS
  first, then test on a homebrew-enabled DSi.
