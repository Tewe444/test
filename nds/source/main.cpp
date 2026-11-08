// Sky Dash für den Nintendo DS (Homebrew)  –  Sky Games
// Oberer Bildschirm: Spiel (Software-Rendering in den 16-Bit-Framebuffer)
// Unterer Bildschirm: Text (Menü, Fortschritt)
#include <nds.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "game.h"

// ======================= Zeichnen =======================
static u16 back[256 * 192];
#define RGB(r, g, b) ((u16)(((r) >> 3) | (((g) >> 3) << 5) | (((b) >> 3) << 10) | 0x8000))
static const u16 BLACK = RGB(0, 0, 0), WHITE = RGB(255, 255, 255);

static inline int cR(u16 c) { return c & 31; }
static inline int cG(u16 c) { return (c >> 5) & 31; }
static inline int cB(u16 c) { return (c >> 10) & 31; }
static u16 mix(u16 a, u16 b, int t) {   // t = 0..16
  int r = (cR(a) * (16 - t) + cR(b) * t) >> 4, g = (cG(a) * (16 - t) + cG(b) * t) >> 4, bl = (cB(a) * (16 - t) + cB(b) * t) >> 4;
  return (u16)(r | (g << 5) | (bl << 10) | 0x8000);
}

static void fillRect(int x0, int y0, int x1, int y1, u16 c) {
  if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0; if (x1 > 256) x1 = 256; if (y1 > 192) y1 = 192;
  if (x0 >= x1 || y0 >= y1) return;
  for (int y = y0; y < y1; y++) { u16* p = back + y * 256 + x0; for (int x = x0; x < x1; x++) *p++ = c; }
}
static void frameRect(int x0, int y0, int x1, int y1, u16 c) {
  fillRect(x0, y0, x1, y0 + 1, c); fillRect(x0, y1 - 1, x1, y1, c);
  fillRect(x0, y0, x0 + 1, y1, c); fillRect(x1 - 1, y0, x1, y1, c);
}
static void disc(int cx, int cy, int r, u16 c) {
  for (int dy = -r; dy <= r; dy++) {
    int w = (int)(sqrtf((float)(r * r - dy * dy)) + 0.5f);
    fillRect(cx - w, cy + dy, cx + w + 1, cy + dy + 1, c);
  }
}
static void line(int x0, int y0, int x1, int y1, u16 c) {
  int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y0 - y1 : y1 - y0;
  int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx + dy;
  for (;;) {
    if ((unsigned)x0 < 256 && (unsigned)y0 < 192) back[y0 * 256 + x0] = c;
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}
static int SN[360], CS[360];
static void initTrig() { for (int i = 0; i < 360; i++) { SN[i] = (int)(sinf(i * 3.14159265f / 180.f) * 256); CS[i] = (int)(cosf(i * 3.14159265f / 180.f) * 256); } }
static void rotRect(int cx, int cy, int hu, int hv, int deg, u16 fill, u16 border, int bw) {
  deg %= 360; if (deg < 0) deg += 360;
  int c = CS[deg], s = SN[deg], R = hu + hv;
  for (int dy = -R; dy <= R; dy++) {
    int y = cy + dy; if ((unsigned)y >= 192) continue;
    for (int dx = -R; dx <= R; dx++) {
      int x = cx + dx; if ((unsigned)x >= 256) continue;
      int u = (dx * c + dy * s) >> 8, v = (-dx * s + dy * c) >> 8;
      int au = u < 0 ? -u : u, av = v < 0 ? -v : v;
      if (au <= hu && av <= hv) back[y * 256 + x] = (au > hu - bw || av > hv - bw) ? border : fill;
    }
  }
}
// Stachel: Zelle mit Unterkante yb (Pixel), 16 Pixel hoch
static void drawSpike(int x, int yb, bool up) {
  int cx = x + 8;
  for (int r = 0; r < 16; r++) {
    int hw = 1 + (r * 7) / 15, y = up ? (yb - 16 + r) : (yb - 1 - r);
    fillRect(cx - hw, y, cx + hw, y + 1, WHITE);
    if (r >= 2 && r < 15) fillRect(cx - hw + 2, y, cx + hw - 2, y + 1, BLACK);
  }
}

// ======================= Level darstellen =======================
static const u16 P1 = RGB(0, 234, 255), P2 = RGB(255, 225, 79);
static int worldFrame = 0;

static void levelColors(const LLevel* L, float x, u16& bg, u16& gr, u16& ac) {
  int i = 0;
  while (i + 1 < L->ncolors && L->colors[i + 1].x <= x) i++;
  const LColor& c = L->colors[i];
  bg = c.bg; gr = c.ground; ac = c.accent;
  if (i > 0) {
    float t = (x - c.x) / 8.0f;
    if (t < 1.0f) {
      int tt = (int)(t * 16);
      const LColor& p = L->colors[i - 1];
      bg = mix(p.bg, c.bg, tt); gr = mix(p.ground, c.ground, tt); ac = mix(p.accent, c.accent, tt);
    }
  }
}

static void drawBackground(u16 bg, u16 gr, u16 ac) {
  u16 dark = mix(bg, BLACK, 9);
  for (int y = 0; y < 192; y++) {
    u16 c = mix(bg, dark, y * 16 / 191);
    u16* p = back + y * 256;
    for (int x = 0; x < 256; x++) p[x] = c;
  }
  fillRect(0, 176, 256, 192, mix(gr, BLACK, 5)); fillRect(0, 176, 256, 178, ac);
  fillRect(0, 0, 256, 16, mix(gr, BLACK, 5));    fillRect(0, 14, 256, 16, ac);
}

#define SX(wx) ((int)(((wx) - camx + 64.f) * 16.f) - 1024)
#define SY(wy) (176 - (int)(((wy) + 64.f) * 16.f) + 1024)

static void drawLevel(const Sim& S, float camx, u16 bg, u16 gr, u16 ac) {
  const LLevel* L = S.L;
  int j0 = S.firstAt(camx - L->maxW - 1.0f);
  u16 blockFill = mix(gr, BLACK, 7), decoCol = mix(bg, ac, 3);

  // 1) Deko im Hintergrund
  for (int j = j0; j < L->nobjs; j++) {
    const LObj& o = L->objs[j];
    if (o.x > camx + 17.0f) break;
    if (o.t != T_DECO) continue;
    int x = SX(o.x), yBot = SY(o.y);
    fillRect(x, yBot - (int)(o.h * 16), x + (int)(o.w * 16), yBot, decoCol);
  }
  // 2) Rest
  for (int j = j0; j < L->nobjs; j++) {
    const LObj& o = L->objs[j];
    if (o.x > camx + 17.0f) break;
    int t = o.t;
    if (t == T_DECO) continue;
    int x = SX(o.x), yBot = SY(o.y);
    switch (t) {
      case T_BLOCK: case T_SLAB: {
        int w = (int)(o.w * 16), h = (int)(o.h * 16), yTop = yBot - h;
        fillRect(x, yTop, x + w, yBot, blockFill); frameRect(x, yTop, x + w, yBot, ac);
        if (w > 4 && h > 4) frameRect(x + 2, yTop + 2, x + w - 2, yBot - 2, mix(blockFill, ac, 4));
        break; }
      case T_SPIKE: drawSpike(x, yBot, true); break;
      case T_CSPIKE: drawSpike(x, yBot, false); break;
      case T_ORB: case T_BORB: {
        u16 c = t == T_ORB ? RGB(255, 225, 40) : RGB(60, 160, 255);
        disc(x + 8, yBot - 8, 7, BLACK); disc(x + 8, yBot - 8, 5, S.used[j] ? mix(c, BLACK, 8) : c); disc(x + 8, yBot - 8, 2, WHITE);
        break; }
      case T_PAD:
        fillRect(x + 1, yBot - 5, x + 15, yBot, BLACK);
        fillRect(x + 2, yBot - 4, x + 14, yBot - 1, S.used[j] ? RGB(160, 120, 0) : RGB(255, 225, 40));
        break;
      case T_COIN:
        if (!(S.coins & (1 << S.cid[j]))) { disc(x + 8, yBot - 8, 7, BLACK); disc(x + 8, yBot - 8, 5, RGB(255, 210, 30)); disc(x + 8, yBot - 8, 2, RGB(255, 250, 190)); }
        break;
      case T_SHIP: case T_CUBE: case T_WAVE: case T_GUP: case T_GDOWN: {
        u16 c = t == T_SHIP ? RGB(255, 80, 214) : t == T_CUBE ? RGB(109, 255, 58) : t == T_WAVE ? RGB(0, 234, 255) : t == T_GUP ? RGB(255, 225, 79) : RGB(80, 140, 255);
        frameRect(x + 3, yBot - 48, x + 13, yBot, c); frameRect(x + 4, yBot - 47, x + 12, yBot - 1, mix(c, BLACK, 6));
        break; }
      case T_SAW: {
        int r = (int)(o.w * 8), cx = x + r, cy = yBot - r;
        disc(cx, cy, r, BLACK); disc(cx, cy, r - 2, RGB(200, 200, 210)); disc(cx, cy, r / 3 + 1, RGB(60, 60, 70));
        for (int k = 0; k < 4; k++) { int a = (worldFrame * 9 + k * 90) % 360; line(cx, cy, cx + (CS[a] * (r - 1)) / 256, cy + (SN[a] * (r - 1)) / 256, BLACK); }
        break; }
      default: break;
    }
  }
}

// Spieler + Wave-Spur
static float trailX[24], trailY[24]; static int trailN = 0;
static void drawPlayer(const Sim& S, float camx) {
  int cx = (int)((S.x + S.pw() * 0.5f - camx) * 16.f + 0.5f), cy = 176 - (int)((S.y + S.ph() * 0.5f) * 16.f + 0.5f);
  int rot = (int)S.rot;
  if (S.mode == M_CUBE) {
    rotRect(cx, cy, 7, 7, rot, P1, BLACK, 2);
    rotRect(cx, cy, 3, 3, rot, P2, BLACK, 1);
  } else if (S.mode == M_SHIP) {
    rotRect(cx, cy, 8, 4, -rot, P1, BLACK, 2);
    disc(cx, cy - 4, 4, BLACK); disc(cx, cy - 4, 3, P2);
  } else {
    for (int i = 1; i < trailN; i++) {
      int ax = (int)((trailX[i - 1] - camx) * 16.f), ay = 176 - (int)(trailY[i - 1] * 16.f);
      int bx = (int)((trailX[i] - camx) * 16.f), by = 176 - (int)(trailY[i] * 16.f);
      line(ax, ay, bx, by, P1); line(ax, ay + 1, bx, by + 1, P1);
    }
    rotRect(cx, cy, 4, 4, 45, P1, BLACK, 1);
  }
}

// Partikel beim Tod
struct Part { float x, y, vx, vy; };
static Part parts[16]; static int partT = 0;
static void spawnParts(const Sim& S) {
  for (int i = 0; i < 16; i++) {
    float a = i * 0.3927f, sp = 3.0f + (i % 3) * 2.0f;
    parts[i].x = S.x + 0.45f; parts[i].y = S.y + 0.45f; parts[i].vx = cosf(a) * sp; parts[i].vy = sinf(a) * sp;
  }
  partT = 40;
}

// ======================= Musik (PSG-Kanäle der DS-Soundhardware) =======================
// Melodien und Akkorde stammen aus audio.js; das Schlagzeug ist aus Rauschkanälen.
static const int PSG_MUL = 8;   // PSG spielt 8 Samples pro Zyklus. Klingt die Musik 3 Oktaven zu tief/hoch: hier 1 testen.
static const int MAJ[7] = {0, 2, 4, 5, 7, 9, 11}, MIN[7] = {0, 2, 3, 5, 7, 8, 10};
struct SongDef { int root; const int* scale; int prog[4][3]; const char* A; const char* B; };
static const SongDef SONGS[4] = {
  {60, MAJ, {{0, 4, 7}, {-5, -1, 2}, {-3, 0, 4}, {-7, -3, 0}},
   "4 . 4 . 5 4 . 2 - . 0 . 2 . 4 - 5 . 5 . 4 . 2 . 4 - - . 2 . 1 .", "7 . 7 . 8 7 . 5 - . 4 . 5 . 7 - 9 . 8 . 7 . 5 . 4 - - . 2 . 4 ."},
  {57, MIN, {{0, 3, 7}, {-4, 0, 3}, {3, 7, 10}, {-2, 2, 5}},
   "0 . 0 2 . 4 . 2 0 . 0 2 . 4 5 4 2 . 2 4 . 5 . 4 2 . 0 . 1 . 2 .", "7 . 6 . 4 . 6 7 - . 4 . 5 . 4 . 7 . 9 . 8 . 7 . 5 - . 4 . 2 . ."},
  {54, MIN, {{0, 3, 7}, {5, 8, 12}, {-4, 0, 3}, {-5, -1, 2}},
   "0 2 4 2 7 4 2 4 0 2 4 2 8 4 2 4 0 2 4 2 7 4 2 4 6 5 4 2 4 . 2 .", "7 . 7 9 . 7 . 4 5 . 5 7 . 5 . 2 4 . 4 5 . 4 . 2 1 . 2 . 4 - - ."},
  {52, MIN, {{0, 3, 7}, {-2, 2, 5}, {-4, 0, 3}, {-5, -2, 2}},
   "7 . 4 . 7 . 9 7 . 4 . 2 4 - 2 . 7 . 4 . 7 . 10 9 - 7 . 4 . 5 4 .", "11 . 9 . 7 . 9 11 - . 9 . 7 . 6 . 7 . 4 . 7 . 9 . 7 - - . 4 . 2 ."}
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
static void psg(DutyCycle d, float f, int vol, int frames) {
  int ch = soundPlayPSG(d, (u16)(f * PSG_MUL), (u8)vol, 64);
  killLater(ch, frames);
}
static void noiseHit(int freq, int vol, int frames) { killLater(soundPlayNoise((u16)freq, (u8)vol, 64), frames); }

static void musicStart(int idx, int bpm) {
  if (idx < 0 || idx > 3) idx = 0;
  song = &SONGS[idx]; mBpm = bpm > 0 ? bpm : 140; mStep = 0; mAcc = 0; musicOn = true;
  parseMel(song->A, melDeg[0], melLen[0]); parseMel(song->B, melDeg[1], melLen[1]);
  melCh = -1;
}
static void musicStop() {
  musicOn = false;
  for (int i = 0; i < 10; i++) if (kq[i].ch >= 0) { soundKill(kq[i].ch); kq[i].ch = -1; }
  if (melCh >= 0) { soundKill(melCh); melCh = -1; }
}
static int sectionOf(int bar) {   // 0 intro, 1 drop, 2 break, 3 build, 4 drop2
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
      melCh = soundPlayPSG(drop ? DutyCycle_25 : DutyCycle_50, (u16)(mtof(n) * PSG_MUL), drop ? 40 : 28, 64);
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
static void sfxDie() { noiseHit(3500, 110, 18); psg(DutyCycle_50, 90, 100, 8); }
static void sfxCoin() { psg(DutyCycle_50, 1318, 70, 5); }
static void sfxWin() { psg(DutyCycle_50, 523, 80, 30); psg(DutyCycle_50, 659, 80, 30); psg(DutyCycle_50, 784, 80, 30); }

// ======================= Spielablauf =======================
enum { ST_MENU, ST_PLAY, ST_DEAD, ST_WIN };
static int state = ST_MENU, sel = 0, attempt = 1, deadT = 0, textT = 0;
static int bestPct[16]; static uint8_t coinSave[16];
static Sim S;

static const u32 JUMPKEYS = KEY_A | KEY_B | KEY_X | KEY_Y | KEY_UP | KEY_TOUCH;

static int pctOf(const Sim& s) { int p = (int)(s.x / s.L->length * 100.0f); return p < 0 ? 0 : (p > 100 ? 100 : p); }
static int bits(int v) { int c = 0; while (v) { c += v & 1; v >>= 1; } return c; }

static void showMenu() {
  iprintf("\x1b[2J");
  iprintf("\x1b[1;9HS K Y   D A S H\n");
  iprintf("\x1b[2;8Hein Spiel von Sky Games\n");
  for (int i = 0; i < NLEVELS; i++)
    iprintf("\x1b[%d;1H%c %d %-13s %-6s %3d%%  %d/3", 5 + i * 2, i == sel ? '>' : ' ', i + 1, LEVELS[i].name, LEVELS[i].diff, bestPct[i], bits(coinSave[i]));
  iprintf("\x1b[18;1HHoch/Runter: Level   A: Start");
  iprintf("\x1b[20;1HIm Spiel: A/B/X/Y/Hoch/Touch = Sprung");
  iprintf("\x1b[21;1H(gedrueckt halten = immer wieder)");
  iprintf("\x1b[22;1HSTART: Menue   SELECT: Neustart");
}
static void showPlayText() {
  iprintf("\x1b[2J");
  iprintf("\x1b[2;2H%s", S.L->name);
  iprintf("\x1b[4;2HVersuch %d", attempt);
  iprintf("\x1b[6;2HFortschritt %3d%%   ", pctOf(S));
  iprintf("\x1b[8;2HBestwert    %3d%%", bestPct[sel]);
  iprintf("\x1b[10;2HMuenzen %d/3", bits(S.coins));
  iprintf("\x1b[22;1HSTART: Menue  SELECT: Neustart");
}
static void startAttempt() {
  S.reset(&LEVELS[sel]); trailN = 0; partT = 0; worldFrame = 0;
  musicStop(); musicStart(LEVELS[sel].song, LEVELS[sel].bpm);
  state = ST_PLAY; showPlayText();
}

static void renderTop(float camx) {
  u16 bg, gr, ac; levelColors(S.L, S.x, bg, gr, ac);
  drawBackground(bg, gr, ac);
  drawLevel(S, camx, bg, gr, ac);
  if (state != ST_DEAD) drawPlayer(S, camx);
  if (partT > 0) for (int i = 0; i < 16; i++) {
    int px = (int)((parts[i].x - camx) * 16.f), py = 176 - (int)(parts[i].y * 16.f);
    fillRect(px - 2, py - 2, px + 2, py + 2, (i & 1) ? P1 : P2);
  }
  // Fortschrittsbalken im oberen Rand
  int p = pctOf(S);
  fillRect(63, 4, 193, 11, BLACK); fillRect(65, 6, 191, 9, RGB(40, 40, 40)); fillRect(65, 6, 65 + p * 126 / 100, 9, RGB(100, 255, 60));
}
static void renderAttract(int t) {
  const LLevel* L = &LEVELS[sel];
  u16 bg = L->colors[0].bg, gr = L->colors[0].ground, ac = L->colors[0].accent;
  drawBackground(bg, gr, ac);
  float camx = t * 0.17f;
  u16 blockFill = mix(gr, BLACK, 7);
  for (int i = (int)camx - 1; i < (int)camx + 18; i++) {
    int x = (int)((i - camx) * 16.f);
    if (i % 7 == 0) drawSpike(x, 176, true);
    if (i % 11 == 5) { fillRect(x, 144, x + 32, 176, blockFill); frameRect(x, 144, x + 32, 176, ac); }
  }
  int ph = t % 60;
  float h = (ph < 40) ? (ph / 40.f) : 0.f; float yy = 4.f * h * (1.f - h) * 2.2f;
  rotRect(72 + 8, 176 - 8 - (int)(yy * 16), 7, 7, (ph < 40) ? ph * 90 / 40 : 0, P1, BLACK, 2);
  rotRect(72 + 8, 176 - 8 - (int)(yy * 16), 3, 3, (ph < 40) ? ph * 90 / 40 : 0, P2, BLACK, 1);
}

int main() {
  videoSetMode(MODE_FB0); vramSetBankA(VRAM_A_LCD);
  consoleDemoInit();
  soundEnable();
  initTrig();
  for (int i = 0; i < 10; i++) kq[i].ch = -1;
  memset(back, 0, sizeof back);
  showMenu();
  int frame = 0;

  while (true) {
    scanKeys();
    u32 down = keysDown(), held = keysHeld();
    bool jh = (held & JUMPKEYS) != 0, jp = (down & JUMPKEYS) != 0;
    worldFrame++; frame++;

    if (state == ST_MENU) {
      if (down & (KEY_UP | KEY_LEFT)) { sel = (sel + NLEVELS - 1) % NLEVELS; showMenu(); }
      if (down & (KEY_DOWN | KEY_RIGHT)) { sel = (sel + 1) % NLEVELS; showMenu(); }
      if (down & KEY_A) { attempt = 1; startAttempt(); }
      renderAttract(frame);
    } else {
      if (down & KEY_START) { musicStop(); state = ST_MENU; showMenu(); }
      else if (down & KEY_SELECT) { attempt++; startAttempt(); }
      else if (state == ST_PLAY) {
        S.step(jh, jp);
        if (S.mode == M_WAVE) {
          if (trailN < 24) trailN++; else for (int i = 1; i < 24; i++) { trailX[i - 1] = trailX[i]; trailY[i - 1] = trailY[i]; }
          trailX[trailN - 1] = S.x + 0.17f; trailY[trailN - 1] = S.y + 0.17f;
        } else trailN = 0;
        static uint8_t lastCoins = 0;
        if (S.frame <= 1) lastCoins = 0;
        if (S.coins != lastCoins) { sfxCoin(); lastCoins = S.coins; }
        if (S.dead) {
          int p = pctOf(S); if (p > bestPct[sel]) bestPct[sel] = p;
          sfxDie(); musicStop(); spawnParts(S); state = ST_DEAD; deadT = 40;
        } else if (S.won) {
          bestPct[sel] = 100; coinSave[sel] |= S.coins;
          musicStop(); sfxWin(); state = ST_WIN;
          iprintf("\x1b[2J\x1b[8;6HLEVEL GESCHAFFT!\n\x1b[11;8HMuenzen: %d/3\x1b[13;8HVersuche: %d\x1b[20;3HA: Menue   SELECT: nochmal", bits(S.coins), attempt);
        }
        if (state == ST_PLAY && (frame % 8) == 0) iprintf("\x1b[6;2HFortschritt %3d%%   \x1b[10;2HMuenzen %d/3", pctOf(S), bits(S.coins));
      } else if (state == ST_DEAD) {
        for (int i = 0; i < 16; i++) { parts[i].x += parts[i].vx * G::DT; parts[i].y += parts[i].vy * G::DT; parts[i].vy -= 20.f * G::DT; }
        if (partT > 0) partT--;
        if (--deadT <= 0) { attempt++; startAttempt(); }
      } else if (state == ST_WIN) {
        if (down & KEY_A) { state = ST_MENU; showMenu(); }
      }
      if (state != ST_MENU) renderTop(S.x - 4.5f);
      else renderAttract(frame);
    }

    musicUpdate();
    swiWaitForVBlank();
    DC_FlushRange(back, sizeof back);
    dmaCopy(back, VRAM_A, sizeof back);
  }
  return 0;
}
