#pragma once
#include <nds.h>
#include <math.h>
// ======================= Musik (PSG-Kanäle der DS-Soundhardware) =======================
// Melodien und Akkorde stammen aus audio.js; das Schlagzeug ist aus Rauschkanälen.
static int gMusicVol = 70, gSfxVol = 80;   // 0..100
static const int PSG_MUL = 8;   // PSG spielt 8 Samples pro Zyklus. Klingt die Musik 3 Oktaven zu tief/hoch: hier 1 testen.
static const int MAJ[7] = {0, 2, 4, 5, 7, 9, 11}, MIN[7] = {0, 2, 3, 5, 7, 8, 10};
struct SongDef { int root; const int* scale; int prog[4][3]; const char* A; const char* B; };
static const SongDef SONGS[5] = {
  {60, MAJ, {{0, 4, 7}, {-5, -1, 2}, {-3, 0, 4}, {-7, -3, 0}},
   "4 . 4 . 5 4 . 2 - . 0 . 2 . 4 - 5 . 5 . 4 . 2 . 4 - - . 2 . 1 .", "7 . 7 . 8 7 . 5 - . 4 . 5 . 7 - 9 . 8 . 7 . 5 . 4 - - . 2 . 4 ."},
  {57, MIN, {{0, 3, 7}, {-4, 0, 3}, {3, 7, 10}, {-2, 2, 5}},
   "0 . 0 2 . 4 . 2 0 . 0 2 . 4 5 4 2 . 2 4 . 5 . 4 2 . 0 . 1 . 2 .", "7 . 6 . 4 . 6 7 - . 4 . 5 . 4 . 7 . 9 . 8 . 7 . 5 - . 4 . 2 . ."},
  {54, MIN, {{0, 3, 7}, {5, 8, 12}, {-4, 0, 3}, {-5, -1, 2}},
   "0 2 4 2 7 4 2 4 0 2 4 2 8 4 2 4 0 2 4 2 7 4 2 4 6 5 4 2 4 . 2 .", "7 . 7 9 . 7 . 4 5 . 5 7 . 5 . 2 4 . 4 5 . 4 . 2 1 . 2 . 4 - - ."},
  {52, MIN, {{0, 3, 7}, {-2, 2, 5}, {-4, 0, 3}, {-5, -2, 2}},
   "7 . 4 . 7 . 9 7 . 4 . 2 4 - 2 . 7 . 4 . 7 . 10 9 - 7 . 4 . 5 4 .", "11 . 9 . 7 . 9 11 - . 9 . 7 . 6 . 7 . 4 . 7 . 9 . 7 - - . 4 . 2 ."},
  {62, MAJ, {{0, 4, 7}, {-3, 0, 4}, {-7, -3, 0}, {-5, -1, 2}},
   "4 . . 2 . . 4 . 5 . 4 . 2 . . . 1 . . 2 . . 4 . 2 . 1 . 0 . . .", "7 . . 5 . . 7 . 8 . 7 . 5 . . . 4 . . 5 . . 4 . 2 . 1 . 2 . . ."}
};
static signed char melDeg[2][32], melLen[2][32];
static void parseMel(const char* s, signed char* deg, signed char* len) {
  for (int i = 0; i < 32; i++) { deg[i] = -100; len[i] = 0; }
  int pos = 0, last = -1;
  const char* p = s;
  while (*p && pos < 32) {
    while (*p == ' ') p++;
    if (!*p) break;
    if (*p == '-') { if (last >= 0) len[last]++; p++; }
    else if (*p == '.') { p++; }
    else { int v = 0; while (*p >= '0' && *p <= '9') v = v * 10 + (*p++ - '0'); deg[pos] = (signed char)v; len[pos] = 1; last = pos; }
    pos++;
  }
}

static bool menuSong = false;
static bool musicOn = false; static const SongDef* song = &SONGS[0];
static int mStep = 0, mBpm = 140; static float mAcc = 0;
struct KillQ { int ch, frames; };
static KillQ kq[10];
static int melCh = -1, melEnd = 0;

static void killLater(int ch, int frames) {
  if (ch < 0) return;
  for (int i = 0; i < 10; i++) if (kq[i].ch < 0) { kq[i].ch = ch; kq[i].frames = frames; return; }
  soundKill(ch);
}
static float mtof(int m) { return 440.0f * powf(2.0f, (m - 69) / 12.0f); }
static int degToNote(const SongDef* S, int deg) { int o = deg / 7, i = ((deg % 7) + 7) % 7; return S->root + 12 * o + S->scale[i]; }
static void psg(DutyCycle d, float f, int vol, int frames, bool sfx = false) {
  int ch = soundPlayPSG(d, (u16)(f * PSG_MUL), (u8)(vol * (sfx ? gSfxVol : gMusicVol) / 100), 64);
  killLater(ch, frames);
}
static void noiseHit(int freq, int vol, int frames, bool sfx = false) { killLater(soundPlayNoise((u16)freq, (u8)(vol * (sfx ? gSfxVol : gMusicVol) / 100), 64), frames); }

static void musicStart(int idx, int bpm) {
  if (idx < 0 || idx > 4) idx = 0;
  menuSong = idx == 4; song = &SONGS[idx]; mBpm = bpm > 0 ? bpm : 140; mStep = 0; mAcc = 0; musicOn = true;
  parseMel(song->A, melDeg[0], melLen[0]); parseMel(song->B, melDeg[1], melLen[1]);
  melCh = -1;
}
static void musicStop() {
  musicOn = false;
  for (int i = 0; i < 10; i++) if (kq[i].ch >= 0) { soundKill(kq[i].ch); kq[i].ch = -1; }
  if (melCh >= 0) { soundKill(melCh); melCh = -1; }
}
static int sectionOf(int bar) {
  if (menuSong) return bar % 16 < 8 ? 2 : 1;   // 0 intro, 1 drop, 2 break, 3 build, 4 drop2
  if (bar < 4) return bar == 3 ? 3 : 0;
  int b = 4 + (bar - 4) % 20;
  if (b < 12) return 1;
  if (b < 16) return b == 15 ? 3 : 2;
  return 4;
}
static void musicStepPlay() {
  int bar = mStep / 16, s16 = mStep % 16, sec = sectionOf(bar);
  bool drop = sec == 1 || sec == 4, brk = sec == 2, build = sec == 3;
  const int* chord = song->prog[bar % 4];
  if (!brk && s16 % 4 == 0) noiseHit(1800, 70, 3);                       // Kick
  if (drop && (s16 == 4 || s16 == 12)) noiseHit(9000, 45, 4);            // Klatschen
  if (!brk && s16 % 4 == 2) noiseHit(24000, 14, 1);                      // Hi-Hat
  if (build && s16 % 2 == 0) noiseHit(7000 + s16 * 400, 20 + s16 * 2, 2); // Wirbel
  if (!brk && s16 % 4 == 2) psg(DutyCycle_25, mtof(song->root - 24 + chord[0]), 55, 3);   // Bass
  if (sec != 1) {                                                         // Arpeggio
    static const int ord[4] = {0, 1, 2, 1};
    int c = chord[ord[s16 % 4]] + (s16 >= 8 ? 12 : 0);
    if (!brk || s16 % 2 == 0) psg(DutyCycle_50, mtof(song->root + 12 + c), 14, 3);
  }
  if (drop || brk) {                                                      // Melodie
    int h = (bar % 4 < 2) ? 0 : 1, idx = (bar % 2) * 16 + s16;
    if (melCh >= 0 && mStep >= melEnd) { soundKill(melCh); melCh = -1; }
    if (melDeg[h][idx] > -100) {
      if (melCh >= 0) soundKill(melCh);
      int n = degToNote(song, melDeg[h][idx]) + (sec == 4 ? 12 : 0);
      melCh = soundPlayPSG(drop ? DutyCycle_25 : DutyCycle_50, (u16)(mtof(n) * PSG_MUL), (u8)((drop ? 40 : 28) * gMusicVol / 100), 64);
      melEnd = mStep + melLen[h][idx];
    }
  }
  mStep++;
}
static void musicUpdate() {
  for (int i = 0; i < 10; i++) if (kq[i].ch >= 0 && --kq[i].frames <= 0) { soundKill(kq[i].ch); kq[i].ch = -1; }
  if (!musicOn) return;
  mAcc += 1.0f;
  float spf = 900.0f / mBpm;                 // Frames pro 16tel-Note (60 fps)
  while (mAcc >= spf) { mAcc -= spf; musicStepPlay(); }
}
static void sfxDie() { noiseHit(3500, 110, 18, true); psg(DutyCycle_50, 90, 100, 8, true); }
static void sfxCoin() { psg(DutyCycle_50, 1318, 70, 5, true); }
static void sfxClick() { psg(DutyCycle_50, 1100, 40, 2, true); }
static void sfxJump() { noiseHit(26000, 12, 1, true); }
static void sfxWin() { psg(DutyCycle_50, 523, 80, 30, true); psg(DutyCycle_50, 659, 80, 30, true); psg(DutyCycle_50, 784, 80, 30, true); }

