# Controls and gameplay

## Menu
- A / Start: continue from title or play the selected level.
- Left / L: previous level.
- Right / R: next level.
- B: return to title.
- Touch the bottom screen's left/right/play zones to browse or start.

## Play
- A / B / X / Y / Up: jump.
- Touch screen: jump.
- Start: pause.
- A: retry after a crash or play the next level after a clear.
- B: return to level selection from pause/crash/clear.

## Gameplay
The cube automatically moves horizontally. Jump timing, gravity and obstacle
spacing determine success. The current build has spikes, paired spikes and
blocks. Twenty level definitions are stored in `source/levels.c`.

Each level carries its own Maxmod module ID and display title. Music begins when
a level is selected and plays in a loop. Pause/resume/retry/return-to-menu manage
playback. The bundled WAV cues play on level entry, exit, crash and completion.

The S-Block marker is displayed in selected layouts but is not lethal. Its
intended Stop behaviour requires Dash Orb gameplay, which is not part of this
Cube-only prototype yet.
