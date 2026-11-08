#!/usr/bin/env python3
"""Static checks for Sky Dash levels, audio assets and offline build setup."""
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
LEVELS = ROOT / "source" / "levels.c"
AUDIO = ROOT / "assets" / "audio"
GAME_H = ROOT / "include" / "game.h"
GAME_C = ROOT / "source" / "game.c"
AUDIO_C = ROOT / "source" / "audio.c"
MAKEFILE = ROOT / "Makefile"


def fail(message: str) -> None:
    print(f"ERROR: {message}")
    sys.exit(1)


src = LEVELS.read_text(encoding="utf-8")
header = GAME_H.read_text(encoding="utf-8")
game_c = GAME_C.read_text(encoding="utf-8")
audio_c = AUDIO_C.read_text(encoding="utf-8")
makefile = MAKEFILE.read_text(encoding="utf-8")

if "#define SKY_LEVEL_COUNT 20" not in header:
    fail("SKY_LEVEL_COUNT must be 20")

rows = re.findall(
    r'\{"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*level_(\d{2}),'
    r'\s*sizeof\(level_\d{2}\)/sizeof\(level_\d{2}\[0\]\),'
    r'\s*([0-9.]+)f,\s*([0-9.]+)f,.*?,\s*MOD_([A-Z0-9_]+)\}', src
)
if len(rows) != 20:
    fail(f"expected 20 complete level rows, found {len(rows)}")
if [row[3] for row in rows] != [f"{i:02d}" for i in range(1, 21)]:
    fail("level table must map level_01 through level_20 in order")

arrays = re.findall(r"static const LevelObject (level_\d{2})\[\]", src)
if len(arrays) != 20 or len(set(arrays)) != 20:
    fail(f"expected 20 unique level object arrays, found {len(arrays)}")

array_data = {}
for match in re.finditer(
    r"static const LevelObject (level_\d{2})\[\]\s*=\s*\{(.*?)\};", src, re.S
):
    coords = [(int(x), kind) for x, kind in re.findall(r"\{(\d+),\s*(OBJ_[A-Z_]+)\}", match.group(2))]
    if not coords:
        fail(f"{match.group(1)} has no obstacles")
    allowed = {"OBJ_SPIKE", "OBJ_DOUBLE_SPIKE", "OBJ_BLOCK", "OBJ_STOP_BLOCK"}
    if any(kind not in allowed for _, kind in coords):
        fail(f"{match.group(1)} contains an unknown object type")
    if any(coords[i][0] >= coords[i + 1][0] for i in range(len(coords) - 1)):
        fail(f"{match.group(1)} obstacle coordinates must be strictly increasing")
    array_data[match.group(1)] = coords

module_files = {p.stem.upper() for p in AUDIO.glob("*.xm")}
used_modules = set()
for row in rows:
    level_name, difficulty, song, array_id, length_text, speed_text, module = row
    used_modules.add(module)
    array_id = "level_" + array_id
    if array_id not in array_data:
        fail(f"missing object array {array_id}")
    length = float(length_text)
    speed = float(speed_text)
    if length <= 0 or speed <= 0:
        fail(f"{array_id} has invalid level length or speed")
    if array_data[array_id][-1][0] >= length:
        fail(f"{array_id} has an obstacle beyond the level finish")
    if module not in module_files:
        fail(f"{array_id} references missing XM module {module}")

xm_files = list(AUDIO.glob("*.xm"))
if len(xm_files) != 34:
    fail(f"expected 34 supplied XM modules, found {len(xm_files)}")
for p in xm_files:
    if p.read_bytes()[:17] != b"Extended Module: ":
        fail(f"{p.name} is not a valid XM module header")

wav_files = list(AUDIO.glob("*.wav")) + list(AUDIO.glob("*.WAV"))
if len(wav_files) != 4:
    fail(f"expected 4 supplied SFX WAV files, found {len(wav_files)}")
for p in wav_files:
    data = p.read_bytes()
    if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        fail(f"{p.name} is not a valid RIFF/WAVE file")
    if f"SFX_{p.stem.upper()}" not in audio_c:
        fail(f"audio.c does not use the sound effect {p.stem}")

if "AUDIO := assets/audio" not in makefile or "mmutil" not in makefile:
    fail("Maxmod audio build configuration is missing")
if "NITRO := nitrofs" not in makefile or "-lmm9" not in makefile:
    fail("NitroFS / Maxmod library configuration is missing")
if "dswifi" in makefile.lower():
    fail("Wi-Fi library must not be linked")
for source_path in list((ROOT / "source").glob("*.c")) + list((ROOT / "include").glob("*.h")):
    code = source_path.read_text(encoding="utf-8").lower()
    if "dswifi" in code or "socket(" in code or "http://" in code or "https://" in code:
        fail(f"network-related code found in {source_path.relative_to(ROOT)}")
if "OBJ_STOP_BLOCK" not in src:
    fail("S-block marker is not present in the level data")
if "mmLoadEffect" not in audio_c or "mmEffect(" not in audio_c:
    fail("WAV sound effects are not loaded and triggered")
if "KEY_LEFT" not in game_c or "KEY_RIGHT" not in game_c:
    fail("20-level selection controls are missing")

bmp = ROOT / "data" / "icon.bmp"
data = bmp.read_bytes()
if data[:2] != b"BM" or len(data) < 26:
    fail("data/icon.bmp is not a valid BMP file")
width, height = struct.unpack_from("<ii", data, 18)
if (width, abs(height)) != (32, 32):
    fail(f"DS banner icon must be 32x32, found {width}x{abs(height)}")

sheet_header = (ROOT / "assets" / "graphics" / "sky_dash_spritesheet.png").read_bytes()[:24]
if sheet_header[:8] != b"\x89PNG\r\n\x1a\n":
    fail("sprite sheet is not a PNG")
sheet_w, sheet_h = struct.unpack(">II", sheet_header[16:24])
if (sheet_w, sheet_h) != (128, 64):
    fail(f"sprite sheet must be 128x64, found {sheet_w}x{sheet_h}")

print(
    f"OK: 20 levels / {sum(len(v) for v in array_data.values())} ordered obstacles, "
    f"{len(used_modules)} assigned + {len(xm_files)} bundled XM modules, "
    f"{len(wav_files)} valid SFX WAVs, Maxmod/NitroFS/offline config, 32x32 icon and 128x64 sprite sheet."
)
