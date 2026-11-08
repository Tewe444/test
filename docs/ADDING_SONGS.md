# Adding or changing a level song

Sky Dash builds a Maxmod soundbank from every supported file in `assets/audio/`.
On Nintendo DS, Maxmod supports XM modules as well as MOD/S3M/IT and WAV files.
This project uses the `.xm` modules from the Geometry Dash Advance archive supplied
in this chat. The build step creates `nitrofs/soundbank.bin` and a generated
`build/soundbank.h` with constants for each module.

## Change the song for one of the 20 levels

Open `source/levels.c`. Each row at the bottom ends with a `MOD_...` identifier.
For example:

```c
{"FIRST FLIGHT", "EASY", "STEREO MADNESS", level_01, ... , MOD_STEREOMA}
```

Change `MOD_STEREOMA` to another song ID, such as `MOD_BACKOT` or `MOD_TOE`.
The ID is based on the filename (uppercase, with punctuation replaced by `_`).

## Add your own song

1. Convert your song to an XM module (`.xm`) using OpenMPT or another tracker.
2. Give the file a simple name with no spaces, for example `MY_TRACK.xm`.
3. Copy it into `assets/audio/`.
4. Run `make clean` and then `make` so mmutil regenerates the soundbank.
5. In `source/levels.c`, assign `MOD_MY_TRACK` to the level you want.

All 34 `.xm` modules from the supplied Geometry Dash Advance archive are included.
The 20 campaign tracks are assigned by default; the remaining modules are also
available to assign to any level: `CLOWNPARTY`, `EVERYEND`, `FINALBATTLE`,
`GROUND`, `INFERNO`, `MENU`, `PRACTICE`, `PROBLEM`, `PYRO`, `ROSE`, `RTAY`,
`SELECTPAYMENT`, `STEREOMA2` and `ULTIMATE`.

Four WAV effects (`EXPLOSION`, `LEVEL_COMPLETE`, `LEVEL_ENTER`, `LEVEL_EXIT`)
are also included and loaded by the event system. Their audio data is byte-identical
to the originals; the basenames are uppercase to produce stable Maxmod `SFX_...`
identifiers. The original audio credit list is preserved in `GDA_AUDIO_CREDITS.md`.

## Important rights note

The supplied Geometry Dash Advance repository's `LICENSE` is an MIT license for
the code; the audio credit document lists the composers and cover artists but
does not grant a separate blanket license for all music. The audio files have
been kept unchanged from the archive you supplied. Before publishing the game
or its binary publicly, verify the rights and attribution requirements for each
track with its respective creators.
