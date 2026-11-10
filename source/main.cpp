// Sky Dash für den Nintendo DS (Homebrew) - Sky Games / RD-GlitchedGuy
// Physik: 1:1 aus physics.js, Modi und Werte nach Geometry Dash (Advance).
// Oben: Spiel. Unten: Touchscreen (Menüs, Editor, Figuren-Zeichner).
#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include "gfx.h"
#include "audio.h"

// ======================= Ein-/Ausgabe =======================
static u32 kd, kh;
static bool tDown, tHeld; static int tx, ty;
static const u32 JUMPKEYS = KEY_A | KEY_B | KEY_X | KEY_Y | KEY_UP | KEY_TOUCH;
static bool inRect(int x, int y, int w, int h) { return tx >= x && tx < x + w && ty >= y && ty < y + h; }
static bool pill(int x, int y, int w, int h, const char* label, int style = 0, int sc = 2) {
  bool hot = tHeld && inRect(x, y, w, h);
  u16 top = style == 1 ? RGB(168, 244, 255) : style == 2 ? RGB(255, 170, 170) : style == 3 ? RGB(255, 240, 160) : RGB(184, 255, 122);
  u16 bot = style == 1 ? RGB(0, 112, 184) : style == 2 ? RGB(168, 0, 0) : style == 3 ? RGB(196, 106, 0) : RGB(28, 158, 0);
  int d = hot ? 2 : 0;
  while (sc > 1 && textW(label, sc) > w - 8) sc--;
  rbox(x, y + d, x + w, y + h + d, BLACK, BLACK);
  for (int i = 2; i < h - 2; i++) fillRect(x + 2, y + d + i, x + w - 2, y + d + i + 1, mix(top, bot, i * 16 / h));
  text(-(x + w / 2), y + d + (h - 7 * sc) / 2, sc, label, WHITE, WHITE, 1);
  if (tDown && inRect(x, y, w, h)) { sfxClick(); return true; }
  return false;
}
static bool roundBtn(int cx, int cy, int r, const char* label, int style = 0, int sc = 3) {
  bool hot = tHeld && (tx - cx) * (tx - cx) + (ty - cy) * (ty - cy) <= r * r;
  int d = hot ? 2 : 0;
  disc(cx, cy + 3, r + 3, BLACK); disc(cx, cy + d, r + 3, BLACK);
  u16 c = style == 1 ? CYAN : style == 2 ? RGB(255, 90, 214) : style == 3 ? GOLD : GREEN;
  disc(cx, cy + d, r, mix(c, BLACK, 5)); disc(cx, cy + d - 2, r - 2, c); disc(cx - r / 4, cy + d - r / 3, r / 3, mix(c, WHITE, 8));
  text(-cx, cy + d - 3 * sc - 1, sc, label, WHITE, WHITE, 1);
  return tDown && (tx - cx) * (tx - cx) + (ty - cy) * (ty - cy) <= r * r && (sfxClick(), true);
}
static bool slider(int x, int y, int w, int& v) {
  fillRect(x, y + 5, x + w, y + 11, BLACK); fillRect(x + 1, y + 6, x + 1 + (w - 2) * v / 100, y + 10, GREEN);
  int kx = x + w * v / 100; disc(kx, y + 8, 7, BLACK); disc(kx, y + 8, 5, WHITE);
  if (tHeld && tx >= x - 8 && tx <= x + w + 8 && ty >= y - 6 && ty <= y + 22) { v = (tx - x) * 100 / w; if (v < 0) v = 0; if (v > 100) v = 100; return true; }
  return false;
}

// ======================= Speichern (SD-Karte) =======================
static bool sdOk = false;
struct SaveData {
  u32 magic; int musicVol, sfxVol, pct, glow, icon, c1, c2;
  uint8_t best[16], bestP[16], coins[16]; u32 attempts;
};
static SaveData sv;
static bool showPct = true;
static const char* DIR_ROOT = "fat:/skydash";
static const char* DIR_LV = "fat:/skydash/levels";
static void ensureDirs() { if (!sdOk) return; mkdir(DIR_ROOT, 0777); mkdir(DIR_LV, 0777); }
static void saveGame() {
  if (!sdOk) return;
  sv.magic = 0x53444D31; sv.musicVol = gMusicVol; sv.sfxVol = gSfxVol; sv.pct = showPct; sv.glow = glowOn;
  sv.icon = selIcon; sv.c1 = selC1; sv.c2 = selC2;
  FILE* f = fopen("fat:/skydash/save.bin", "wb"); if (!f) return; fwrite(&sv, sizeof sv, 1, f); fclose(f);
}
static void loadGame() {
  memset(&sv, 0, sizeof sv);
  if (!sdOk) return;
  FILE* f = fopen("fat:/skydash/save.bin", "rb");
  if (f) { SaveData t; if (fread(&t, sizeof t, 1, f) == 1 && t.magic == 0x53444D31) { sv = t; gMusicVol = sv.musicVol; gSfxVol = sv.sfxVol; showPct = sv.pct; glowOn = sv.glow; selIcon = sv.icon; selC1 = sv.c1 % 12; selC2 = sv.c2 % 12; } fclose(f); }
  f = fopen("fat:/skydash/char.bin", "rb");
  if (f) { if (fread(sprite, 1, 256, f) == 256) { hasSprite = false; for (int i = 0; i < 256; i++) if (((uint8_t*)sprite)[i]) hasSprite = true; } fclose(f); }
}
static void saveSprite() { if (!sdOk) return; FILE* f = fopen("fat:/skydash/char.bin", "wb"); if (f) { fwrite(sprite, 1, 256, f); fclose(f); } }

// ======================= Level aus Text (SD-Dateien, Editor) =======================
static LObj cObjs[SIM_MAX]; static LColor cCols[16]; static LLevel cust;
static char cName[24] = "Eigenes Level"; static char cDiff[8] = "normal";
static int cmpObj(const void* a, const void* b) {
  const LObj *p = (const LObj*)a, *q = (const LObj*)b;
  if (p->x != q->x) return p->x < q->x ? -1 : 1;
  return p->y < q->y ? -1 : (p->y > q->y ? 1 : 0);
}
static bool num(const char* s, float& o) { char* e; o = strtof(s, &e); return e != s && *e == 0; }
static unsigned hex555(const char* h) {
  unsigned r = strtoul(std::string(h + 1, 2).c_str(), 0, 16) >> 3, g = strtoul(std::string(h + 3, 2).c_str(), 0, 16) >> 3, b = strtoul(std::string(h + 5, 2).c_str(), 0, 16) >> 3;
  return r | (g << 5) | (b << 10) | 0x8000;
}
static struct { const char* n; int t; } CMD[] = {
  {"orb", T_ORB}, {"borb", T_BORB}, {"porb", T_PORB}, {"rorb", T_RORB}, {"pad", T_PAD}, {"ppad", T_PPAD}, {"rpad", T_RPAD}, {"coin", T_COIN},
  {"ship", T_SHIP}, {"cube", T_CUBE}, {"wave", T_WAVE}, {"ball", T_BALL}, {"ufo", T_UFO}, {"mini", T_MINI}, {"big", T_BIG},
  {"gup", T_GUP}, {"gdown", T_GDOWN}, {"sp05", T_SP05}, {"sp1", T_SP1}, {"sp2", T_SP2}, {"sp3", T_SP3}, {"sp4", T_SP4}};
// Übersetzt Level-Text (dasselbe Format wie die .txt-Dateien des PC-Tools, ohne def/use/loop). Verändert txt.
static bool compileText(char* txt) {
  int n = 0, nc = 0; float stars = 1, speed = 10.386f, length = 0; int bpm = 140, song = 0;
  strcpy(cName, "Eigenes Level"); strcpy(cDiff, "normal");
  char* save = 0;
  for (char* line = strtok_r(txt, "\n", &save); line; line = strtok_r(0, "\n", &save)) {
    char* tk[8]; int nt = 0; char* sv2 = 0;
    for (char* w = strtok_r(line, " \t\r", &sv2); w && nt < 8; w = strtok_r(0, " \t\r", &sv2)) {
      if (w[0] == '#' && !(nt > 0 && strcmp(tk[0], "color") == 0 && strlen(w) == 7)) break;
      tk[nt++] = w;
    }
    if (!nt) continue;
    float a[5] = {0, 0, 1, 1, 0}; for (int i = 1; i < nt && i < 6; i++) num(tk[i], a[i - 1]);
    const char* c = tk[0];
    auto add = [&](int t, float x, float y, float w, float h) { if (n < SIM_MAX) { cObjs[n].t = (uint8_t)t; cObjs[n].x = x; cObjs[n].y = y; cObjs[n].w = w; cObjs[n].h = h; n++; } };
    if (!strcmp(c, "name")) { cName[0] = 0; for (int i = 1; i < nt; i++) { if (i > 1) strncat(cName, " ", 22); strncat(cName, tk[i], 22 - strlen(cName)); } }
    else if (!strcmp(c, "diff") && nt > 1) { strncpy(cDiff, tk[1], 7); cDiff[7] = 0; }
    else if (!strcmp(c, "stars")) stars = a[0];
    else if (!strcmp(c, "speed")) speed = a[0] > 0 ? a[0] : 10.386f;
    else if (!strcmp(c, "bpm")) bpm = (int)a[0];
    else if (!strcmp(c, "song")) song = (int)a[0];
    else if (!strcmp(c, "length")) length = a[0];
    else if (!strcmp(c, "color") && nt >= 5 && nc < 16) { cCols[nc].x = a[0]; cCols[nc].bg = hex555(tk[2]); cCols[nc].ground = hex555(tk[3]); cCols[nc].accent = hex555(tk[4]); nc++; }
    else if (!strcmp(c, "block")) add(T_BLOCK, a[0], a[1], nt > 3 ? a[2] : 1, nt > 4 ? a[3] : 1);
    else if (!strcmp(c, "slab")) add(T_SLAB, a[0], a[1], nt > 3 ? a[2] : 1, 0.5f);
    else if (!strcmp(c, "deco")) add(T_DECO, a[0], a[1], nt > 3 ? a[2] : 1, nt > 4 ? a[3] : 1);
    else if (!strcmp(c, "spike") || !strcmp(c, "cspike")) { int cnt = nt > 3 ? (int)a[2] : 1; for (int i = 0; i < cnt; i++) add(c[0] == 'c' ? T_CSPIKE : T_SPIKE, a[0] + i, a[1], 1, 1); }
    else if (!strcmp(c, "saw")) { float sz = nt > 3 ? a[2] : 1; add(T_SAW, a[0], a[1], sz, sz); }
    else for (unsigned k = 0; k < sizeof CMD / sizeof CMD[0]; k++) if (!strcmp(c, CMD[k].n)) { add(CMD[k].t, a[0], a[1], 1, 1); break; }
  }
  qsort(cObjs, n, sizeof(LObj), cmpObj);
  float maxX = 0, maxW = 1; for (int i = 0; i < n; i++) { if (cObjs[i].x + cObjs[i].w > maxX) maxX = cObjs[i].x + cObjs[i].w; if (cObjs[i].w > maxW) maxW = cObjs[i].w; }
  if (length <= 0) length = maxX + 20;
  if (nc == 0 || cCols[0].x > 0) { for (int i = nc < 15 ? nc : 15; i > 0; i--) cCols[i] = cCols[i - 1]; cCols[0].x = 0; cCols[0].bg = hex555("#1f62ff"); cCols[0].ground = hex555("#0a49d6"); cCols[0].accent = hex555("#6fe3ff"); if (nc < 16) nc++; }
  cust.name = cName; cust.diff = cDiff; cust.stars = (int)stars; cust.speed = speed; cust.bpm = bpm; cust.song = song; cust.length = length; cust.maxW = maxW;
  cust.nobjs = n; cust.objs = cObjs; cust.ncolors = nc; cust.colors = cCols;
  return true;
}
static char fbuf[40000];
static bool readLevelFile(const char* path) {
  FILE* f = fopen(path, "rb"); if (!f) return false;
  size_t n = fread(fbuf, 1, sizeof fbuf - 1, f); fclose(f); fbuf[n] = 0;
  return compileText(fbuf);
}
static char cFiles[12][32]; static int nCust = 0;
static void scanLevels() {
  nCust = 0; if (!sdOk) return;
  DIR* d = opendir(DIR_LV); if (!d) return;
  struct dirent* e;
  while ((e = readdir(d)) && nCust < 12) { size_t l = strlen(e->d_name); if (l > 4 && l < 31 && !strcasecmp(e->d_name + l - 4, ".txt")) strcpy(cFiles[nCust++], e->d_name); }
  closedir(d);
}
static int totalLevels() { return NLEVELS + nCust; }

// ======================= Spielzustand =======================
enum { ST_SPLASH, ST_MENU, ST_LEVELS, ST_PLAY, ST_PAUSE, ST_COMPLETE, ST_CHARS, ST_CHARED, ST_EDITOR, ST_SETTINGS, ST_CREDITS };
static int state = ST_SPLASH, sel = 0, frameNo = 0;
static Lvl lv; static Sim S; static Sim cps[16]; static float cpRot[16]; static int ncp = 0;
static bool practice = false, testFromEditor = false;
static int attempt = 1, deadT = 0, doneT = 0; static float visRot = 0, camx = 0; static float startX = 0;
static float trailX[24], trailY[24]; static int trailN = 0;
struct Part { float x, y, vx, vy; };
static Part parts[16]; static int partT = 0;
static const LLevel* curLevel = 0;
static int pauseConfirm = 0;
static float stepAcc = 0, fpsShown = 60; static int nsteps = 4;

static const LLevel* levelAt(int i) {
  if (i < NLEVELS) return &LEVELS[i];
  if (readLevelFile((std::string("fat:/skydash/levels/") + cFiles[i - NLEVELS]).c_str())) return &cust;
  return 0;
}
static int pctOf() { int p = (int)(S.x / lv.length * 100.0f); return p < 0 ? 0 : (p > 100 ? 100 : p); }
static int bits(int v) { int c = 0; while (v) { c += v & 1; v >>= 1; } return c; }
static int beatPulse() {
  if (!musicOn) return 0;
  float spf = 900.0f / mBpm, ph = ((mStep % 4) + mAcc / spf) / 4.0f;
  return ph < 0.35f ? (int)(16 * (1 - ph / 0.35f)) : 0;
}
static void resetAttempt() {
  Sim* cp = (practice && ncp) ? &cps[ncp - 1] : 0;
  if (cp) { S = *cp; visRot = cpRot[ncp - 1]; } else { S.reset(lv, startX); visRot = 0; }
  trailN = 0; partT = 0; deadT = 0; doneT = 0;
  camx = S.x - 4.8f;
}
static void startLevel(const LLevel* L, bool prac, float sx = 0) {
  curLevel = L; lv.prepare(L); practice = prac; ncp = 0; attempt = 1; startX = sx;
  resetAttempt(); musicStop(); musicStart(L->song, L->bpm); state = ST_PLAY; pauseConfirm = 0;
}
static void spawnParts() {
  for (int i = 0; i < 16; i++) { float a = i * 0.3927f, sp = 3.0f + (i % 3) * 2.2f; parts[i].x = S.x + 0.5f; parts[i].y = S.y + 0.5f; parts[i].vx = cosf(a) * sp; parts[i].vy = sinf(a) * sp; }
  partT = 50;
}
static void updatePlay(bool hold) {
  if (!S.dead && !S.done) {
    for (int k = 0; k < nsteps; k++) { S.step(lv, hold, C::DT); if (S.dead || S.done) break; }
    if (S.ev & EV_COIN) sfxCoin();
    if (S.ev & EV_JUMP) sfxJump();
    if (S.ev & EV_DIE) {
      sfxDie(); musicStop(); spawnParts();
      int p = pctOf(); if (!practice && p > sv.best[sel < 16 ? sel : 0] && sel < NLEVELS && !testFromEditor) sv.best[sel] = (uint8_t)p;
      sv.attempts++;
    }
    if (S.ev & EV_DONE) {
      musicStop(); sfxWin();
      if (!testFromEditor && sel < NLEVELS) { if (practice) sv.bestP[sel] = 100; else { sv.best[sel] = 100; sv.coins[sel] |= S.coins; } saveGame(); }
    }
    S.ev = 0;
  } else if (S.dead) { if (++deadT > 57) { attempt++; resetAttempt(); if (!practice || !ncp) { musicStop(); musicStart(curLevel->song, curLevel->bpm); } } }
  else if (S.done) { S.x += S.speed * (frameScale / 60.0f); if (++doneT > 70) state = ST_COMPLETE; }
  // Drehung (wie game.js)
  if (S.mode == M_CUBE || S.mode == M_BALL) {
    if (S.grounded) { float tg = floorf(visRot / 90.0f + 0.5f) * 90.0f; visRot += (tg - visRot) * 0.31f; } else visRot += 428.6f / 60.0f * S.g * (S.mode == M_BALL ? 1.5f : 1.f);
  } else {
    float want = -atan2f(S.vy, S.speed) * 57.2958f * (S.mode == M_WAVE ? 1.0f : 0.9f);
    visRot += (want - visRot) * (S.mode == M_WAVE ? 1.0f : 0.21f);
  }
  float endCam = lv.length + 2.5f - 16.0f, want = S.x - 4.8f; if (want > endCam) want = endCam; camx = want;
  if (partT > 0) { for (int i = 0; i < 16; i++) { parts[i].x += parts[i].vx / 60.f; parts[i].y += parts[i].vy / 60.f; parts[i].vy -= 20.f / 60.f; } partT--; }
  if (!S.dead && (S.mode == M_WAVE || (frameNo & 1) == 0)) {
    int maxT = S.mode == M_WAVE ? 24 : 10;
    if (trailN < maxT) trailN++; else for (int i = 1; i < maxT; i++) { trailX[i - 1] = trailX[i]; trailY[i - 1] = trailY[i]; }
    trailX[trailN - 1] = S.x + S.sz() * 0.5f; trailY[trailN - 1] = S.y + S.sz() * 0.5f;
  }
}
static void drawPlayTop(bool dim) {
  cur = topBuf;
  u16 bg, gr, ac; levelColors(curLevel, S.x, bg, gr, ac);
  int pulse = beatPulse();
  drawBackground(bg, gr, ac, camx, (curLevel->song) % 4, pulse);
  drawLevel(lv, S, camx, bg, gr, ac);
  { int gx = SX(lv.length); if (gx < 256) { fillRect(gx, 16, gx + 6, 176, mix(bg, WHITE, 10)); fillRect(gx + 6, 16, 256, 176, mix(bg, BLACK, 6)); } }
  if (!practice || ncp == 0) { char b[24]; snprintf(b, sizeof b, "VERSUCH %d", attempt); text(SX(7.0f), 56, 2, b, WHITE, WHITE, 1); }
  for (int i = 0; i < ncp; i++) { int px = SX(cps[i].x + 0.5f), py = SY(cps[i].y + 0.5f); rotRect(px, py, 5, 5, 45, GREEN, BLACK, 1); }
  for (int i = 1; i < trailN; i++) {
    int ax = (int)((trailX[i - 1] - camx) * 16.f), ay = 176 - (int)(trailY[i - 1] * 16.f), bx = (int)((trailX[i] - camx) * 16.f), by = 176 - (int)(trailY[i] * 16.f);
    u16 c = S.mode == M_WAVE ? PAL[selC1] : mix(bg, PAL[selC2], 8);
    line(ax, ay, bx, by, c); if (S.mode == M_WAVE) { line(ax, ay + 1, bx, by + 1, c); line(ax, ay - 1, bx, by - 1, c); }
  }
  if (!S.dead) {
    int half = S.mini ? 5 : 8;
    int cx = (int)((S.x + S.sz() * 0.5f - camx) * 16.f + 0.5f), cy = 176 - (int)((S.y + S.sz() * 0.5f) * 16.f + 0.5f);
    drawPlayer(S.mode, cx, cy, half, (int)visRot, S.g < 0);
  }
  if (partT > 0) for (int i = 0; i < 16; i++) { int px = SX(parts[i].x), py = SY(parts[i].y); fillRect(px - 2, py - 2, px + 2, py + 2, (i & 1) ? PAL[selC1] : PAL[selC2]); }
  // Fortschrittsbalken
  int p = pctOf();
  fillRect(63, 3, 193, 12, BLACK); fillRect(65, 5, 191, 10, RGB(40, 40, 40));
  fillRect(65, 5, 65 + p * 126 / 100, 10, practice ? CYAN : RGB(100, 255, 60));
  if (showPct) { char b[8]; snprintf(b, sizeof b, "%d%%", p); text(198, 4, 1, b, WHITE, WHITE, 1); }
  if (dim) for (int i = 0; i < 256 * 192; i++) cur[i] = (u16)(((cur[i] >> 1) & 0x3DEF) | 0x8000);
}

// ======================= Figuren-Zeichner =======================
static int brush = 7; static bool mirror = true;
static void sendSprite() {}
static void screenCharEd() {
  cur = topBuf; gradBg(RGB(20, 30, 90), RGB(5, 8, 30));
  gtext(-128, 10, 3, "EIGENE FIGUR");
  int t = frameNo; int ph = t % 90; float jh = ph < 40 ? 4.f * (ph / 40.f) * (1.f - ph / 40.f) : 0.f;
  fillRect(0, 150, 256, 192, RGB(10, 10, 40)); fillRect(0, 150, 256, 152, CYAN);
  int sz[3] = {8, 14, 24};
  for (int k = 0; k < 3; k++) { int cx = 50 + k * 78, cy = 150 - sz[k] - (int)(jh * 10 * (k + 1) / 2); drawSprite(cx, cy, sz[k], ph < 40 ? ph * 90 / 40 : 0); }
  text(-128, 160, 1, "ZEICHNE DEINE FIGUR MIT DEM STIFT", WHITE, WHITE, 1);
  text(-128, 174, 1, "SELECT = NAECHSTE FARBE", GOLD, GOLD, 1);
  cur = botBuf; gradBg(RGB(25, 60, 150), RGB(8, 20, 70));
  for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
    int px = 8 + x * 8, py = 30 + y * 8; uint8_t c = sprite[y][x];
    fillRect(px, py, px + 8, py + 8, c ? SPAL[c] : (((x + y) & 1) ? RGB(70, 80, 110) : RGB(50, 60, 90)));
  }
  frameRect(7, 29, 137, 159, BLACK);
  for (int i = 0; i < 16; i++) { int px = 150 + (i % 4) * 25, py = 30 + (i / 4) * 25; if (i == 0) { fillRect(px, py, px + 22, py + 22, RGB(70, 80, 110)); line(px, py, px + 21, py + 21, RGB(255, 60, 60)); } else fillRect(px, py, px + 22, py + 22, SPAL[i]);
    frameRect(px, py, px + 22, py + 22, i == brush ? WHITE : BLACK); if (i == brush) frameRect(px + 1, py + 1, px + 21, py + 21, BLACK);
    if (tDown && inRect(px, py, 22, 22)) { brush = i; sfxClick(); } }
  if (tHeld && inRect(8, 30, 128, 128)) { int x = (tx - 8) / 8, y = (ty - 30) / 8; if (x >= 0 && x < 16 && y >= 0 && y < 16) { sprite[y][x] = (uint8_t)brush; if (mirror) sprite[y][15 - x] = (uint8_t)brush; } }
  text(8, 8, 2, "FIGUR ZEICHNEN", WHITE, WHITE, 1);
  if (pill(148, 134, 48, 22, mirror ? "SPIEGEL" : "FREI", 1, 1)) mirror = !mirror;
  if (pill(200, 134, 48, 22, "LEEREN", 2, 1)) memset(sprite, 0, sizeof sprite);
  if (pill(148, 164, 100, 22, "FERTIG", 0, 2) || (kd & KEY_START)) {
    hasSprite = false; for (int i = 0; i < 256; i++) if (((uint8_t*)sprite)[i]) hasSprite = true;
    if (hasSprite) selIcon = NICONS; saveSprite(); saveGame(); state = ST_CHARS;
  }
  if (pill(8, 164, 60, 22, "ZURUECK", 3, 1) || (kd & KEY_B)) state = ST_CHARS;
  if (kd & KEY_SELECT) brush = (brush + 1) % 16;
}

// ======================= Icons / Figuren =======================
static void screenChars() {
  cur = topBuf; gradBg(RGB(20, 30, 90), RGB(5, 8, 30));
  gtext(-128, 10, 4, "FIGUREN");
  int ph = frameNo % 90; float jh = ph < 40 ? 4.f * (ph / 40.f) * (1.f - ph / 40.f) : 0.f;
  fillRect(0, 150, 256, 192, RGB(10, 10, 40)); fillRect(0, 150, 256, 152, PAL[selC2]);
  int modes[5] = {M_CUBE, M_SHIP, M_BALL, M_UFO, M_WAVE};
  for (int k = 0; k < 5; k++) drawPlayer(modes[k], 28 + k * 50, 126 - (k == 0 ? (int)(jh * 14) : 0), 12, k == 0 ? (ph < 40 ? ph * 90 / 40 : 0) : (k == 4 ? -20 : 0), false);
  text(-128, 160, 1, selIcon >= NICONS ? "EIGENE FIGUR" : "DESIGN", WHITE, WHITE, 1);
  cur = botBuf; gradBg(RGB(25, 60, 150), RGB(8, 20, 70));
  text(8, 6, 2, "ICON-GARAGE", WHITE, WHITE, 1);
  for (int i = 0; i <= NICONS; i++) {
    int px = 8 + (i % 5) * 34, py = 26 + (i / 5) * 34;
    fillRect(px, py, px + 30, py + 30, RGB(0, 0, 30)); frameRect(px, py, px + 30, py + 30, i == selIcon ? WHITE : RGB(60, 80, 160));
    if (i < NICONS) drawIcon(px + 15, py + 15, 10, i, PAL[selC1], PAL[selC2], 0);
    else if (hasSprite) drawSprite(px + 15, py + 15, 11, 0); else text(px + 9, py + 10, 1, "NEU", GOLD, GOLD, 1);
    if (tDown && inRect(px, py, 30, 30)) { sfxClick(); if (i == NICONS && !hasSprite) { state = ST_CHARED; } else selIcon = i; saveGame(); }
  }
  for (int row = 0; row < 2; row++) for (int i = 0; i < 12; i++) {
    int px = 8 + (i % 6) * 22, py = 98 + row * 28 + (i / 6) * 0; (void)py; int yy = 98 + row * 26;
    int xx = 8 + i * 15; fillRect(xx, yy, xx + 13, yy + 20, PAL[i]); bool on = (row == 0 ? selC1 : selC2) == i;
    frameRect(xx, yy, xx + 13, yy + 20, on ? WHITE : BLACK); (void)px;
    if (tDown && inRect(xx, yy, 13, 20)) { if (row == 0) selC1 = i; else selC2 = i; sfxClick(); saveGame(); }
  }
  text(8, 148, 1, "FARBE 1 / FARBE 2", WHITE, WHITE, 1);
  if (pill(180, 26, 68, 28, "ZEICHNEN", 1, 1)) state = ST_CHARED;
  if (pill(180, 62, 68, 28, "ZURUECK", 3, 1) || (kd & KEY_B)) state = ST_MENU;
}

// ======================= Level-Editor =======================
struct Tool { int t; float w, h; const char* name; };
static const Tool TOOLS[] = {
  {T_BLOCK,1,1,"BLOCK"},{T_SLAB,1,0.5f,"HALBBLOCK"},{T_SPIKE,1,1,"STACHEL"},{T_CSPIKE,1,1,"STACHEL OBEN"},{T_SAW,2,2,"SAEGE"},{T_DECO,1,1,"DEKO"},
  {T_ORB,1,1,"GELBER RING"},{T_PORB,1,1,"ROSA RING"},{T_RORB,1,1,"ROTER RING"},{T_BORB,1,1,"BLAUER RING"},
  {T_PAD,1,1,"GELBES PAD"},{T_PPAD,1,1,"ROSA PAD"},{T_RPAD,1,1,"ROTES PAD"},{T_COIN,1,1,"MUENZE"},
  {T_CUBE,1,3,"WUERFEL"},{T_SHIP,1,3,"SCHIFF"},{T_BALL,1,3,"BALL"},{T_UFO,1,3,"UFO"},{T_WAVE,1,3,"WAVE"},{T_MINI,1,3,"MINI"},{T_BIG,1,3,"GROSS"},
  {T_GUP,1,3,"SCHWERK. OBEN"},{T_GDOWN,1,3,"SCHWERK. UNTEN"},{T_SP05,1,3,"TEMPO 0.5X"},{T_SP1,1,3,"TEMPO 1X"},{T_SP2,1,3,"TEMPO 2X"},{T_SP3,1,3,"TEMPO 3X"},{T_SP4,1,3,"TEMPO 4X"}};
static const int NTOOLS = sizeof TOOLS / sizeof TOOLS[0];
static LObj ed[SIM_MAX]; static int edN = 0, edX = 0, tool = 0; static bool eraseMode = false; static int edMsg = 0; static char edMsgT[40];
static void msg(const char* s) { strncpy(edMsgT, s, 39); edMsgT[39] = 0; edMsg = 90; }
static bool edCovers(const LObj& o, int bx, int by) {
  float h = o.h; if (o.t == T_SAW) h = o.w; if (o.t >= T_SHIP && o.t != T_SAW && o.t != T_COIN && o.t != T_SLAB && o.t != T_DECO && o.t != T_BLOCK && o.t != T_SPIKE && o.t != T_CSPIKE && o.t != T_ORB && o.t != T_BORB && o.t != T_PAD && o.t != T_PORB && o.t != T_RORB && o.t != T_PPAD && o.t != T_RPAD) h = 3;
  if (o.t == T_SHIP || o.t == T_CUBE || o.t == T_WAVE || o.t == T_GUP || o.t == T_GDOWN) h = 3;
  return bx >= (int)o.x && bx < (int)(o.x + (o.w > 1 ? o.w : 1) - 0.001f) + (o.w > 1 ? 0 : 0) && by >= (int)o.y && by < (int)(o.y + (h > 1 ? h : 1) - 0.001f) + 1 - (h > 1 ? 1 : 0);
}
static void edPlace(int bx, int by) {
  if (bx < 0 || by < 0 || by > 9) return;
  if (eraseMode) { for (int i = 0; i < edN; i++) if (edCovers(ed[i], bx, by)) { ed[i] = ed[--edN]; i = -1; if (true) break; } return; }
  const Tool& t = TOOLS[tool];
  if (t.h >= 3 && by > 7) by = 7;
  if (t.t == T_SAW && by > 8) by = 8;
  for (int i = 0; i < edN; i++) if (ed[i].t == t.t && (int)ed[i].x == bx && (int)ed[i].y == by) return;
  if (edN >= SIM_MAX - 1) { msg("LEVEL VOLL"); return; }
  ed[edN].t = (uint8_t)t.t; ed[edN].x = (float)bx; ed[edN].y = (float)by; ed[edN].w = t.t == T_SAW ? 2.f : t.w; ed[edN].h = t.t == T_SAW ? 2.f : (t.h >= 3 ? 1.f : t.h); edN++;
}
static void buildFromEd() {
  int n = 0; for (int i = 0; i < edN; i++) cObjs[n++] = ed[i];
  qsort(cObjs, n, sizeof(LObj), cmpObj);
  float maxX = 60, maxW = 1; for (int i = 0; i < n; i++) { if (cObjs[i].x + cObjs[i].w > maxX) maxX = cObjs[i].x + cObjs[i].w; if (cObjs[i].w > maxW) maxW = cObjs[i].w; }
  strcpy(cName, "Mein Level"); strcpy(cDiff, "normal");
  cCols[0].x = 0; cCols[0].bg = hex555("#1f62ff"); cCols[0].ground = hex555("#0a49d6"); cCols[0].accent = hex555("#6fe3ff");
  cust.name = cName; cust.diff = cDiff; cust.stars = 0; cust.speed = 10.386f; cust.bpm = 140; cust.song = 1; cust.length = maxX + 16; cust.maxW = maxW;
  cust.nobjs = n; cust.objs = cObjs; cust.ncolors = 1; cust.colors = cCols;
}
static const char* cmdOf(int t) {
  for (unsigned k = 0; k < sizeof CMD / sizeof CMD[0]; k++) if (CMD[k].t == t) return CMD[k].n;
  return t == T_BLOCK ? "block" : t == T_SLAB ? "slab" : t == T_SPIKE ? "spike" : t == T_CSPIKE ? "cspike" : t == T_DECO ? "deco" : "saw";
}
static void edSave() {
  if (!sdOk) { msg("KEINE SD-KARTE"); return; }
  static char out[40000]; int p = 0;
  p += snprintf(out + p, 200, "# Sky Dash DS - Level aus dem Editor\nname Mein Level\ndiff normal\nstars 0\nspeed 10.386\nbpm 140\nsong 1\ncolor 0 #1f62ff #0a49d6 #6fe3ff\n");
  for (int i = 0; i < edN && p < (int)sizeof out - 80; i++) {
    const LObj& o = ed[i]; const char* c = cmdOf(o.t);
    if (o.t == T_BLOCK || o.t == T_DECO) p += snprintf(out + p, 80, "%s %d %d %d %d\n", c, (int)o.x, (int)o.y, (int)o.w, (int)o.h);
    else if (o.t == T_SLAB) p += snprintf(out + p, 80, "slab %d %d %d\n", (int)o.x, (int)o.y, (int)o.w);
    else if (o.t == T_SAW) p += snprintf(out + p, 80, "saw %d %d %d\n", (int)o.x, (int)o.y, (int)o.w);
    else p += snprintf(out + p, 80, "%s %d %d\n", c, (int)o.x, (int)o.y);
  }
  ensureDirs();
  FILE* f = fopen("fat:/skydash/levels/mylevel.txt", "wb"); if (!f) { msg("SPEICHERN FEHLGESCHLAGEN"); return; }
  fwrite(out, 1, p, f); fclose(f); msg("GESPEICHERT: MYLEVEL.TXT"); scanLevels();
}
static void edLoad() {
  edN = 0;
  if (!readLevelFile("fat:/skydash/levels/mylevel.txt")) return;
  for (int i = 0; i < cust.nobjs && edN < SIM_MAX - 1; i++) ed[edN++] = cObjs[i];
}
static void edTest(bool here) {
  buildFromEd(); testFromEditor = true; sel = NLEVELS + 99; startLevel(&cust, false, here ? (float)edX : 0.f);
}
static void screenEditor() {
  // ---- Eingaben ----
  static int rep = 0;
  if (kh & (KEY_LEFT | KEY_RIGHT)) { rep++; if (rep == 1 || rep > 12) edX += (kh & KEY_RIGHT) ? 1 : -1; } else rep = 0;
  if (kd & KEY_L) edX -= 8; if (kd & KEY_R) edX += 8;
  if (edX < 0) edX = 0; if (edX > 1990) edX = 1990;
  if (kd & KEY_UP) tool = (tool + NTOOLS - 1) % NTOOLS; if (kd & KEY_DOWN) tool = (tool + 1) % NTOOLS;
  if (kd & KEY_A) { eraseMode = !eraseMode; sfxClick(); }
  if ((kd & KEY_B) && edN > 0) edN--;
  if (kd & KEY_X) edTest(false);
  if (kd & KEY_Y) edTest(true);
  if (kd & KEY_START) edSave();
  if (kd & KEY_SELECT) { edSave(); state = ST_MENU; musicStop(); musicStart(4, 120); return; }
  if (state != ST_EDITOR) return;
  static int lastBx = -99, lastBy = -99;
  if (tHeld && ty >= 16 && ty < 176) { int bx = edX + tx / 16, by = (176 - ty) / 16; if (bx != lastBx || by != lastBy || tDown) { edPlace(bx, by); lastBx = bx; lastBy = by; } } else { lastBx = lastBy = -99; }
  // ---- oben: Werkzeuge + Überblick ----
  cur = topBuf; gradBg(RGB(14, 20, 70), RGB(4, 6, 28));
  text(6, 4, 2, "EDITOR", GOLD, GOLD, 1);
  char b[48]; snprintf(b, sizeof b, "X:%d  OBJEKTE:%d/%d", edX, edN, SIM_MAX - 1); text(92, 8, 1, b, WHITE, WHITE, 1);
  for (int i = 0; i < NTOOLS; i++) {
    int cx = 4 + (i % 7) * 36, cy = 22 + (i / 7) * 30;
    fillRect(cx, cy, cx + 34, cy + 28, RGB(0, 0, 25)); frameRect(cx, cy, cx + 34, cy + 28, i == tool ? (eraseMode ? RGB(255, 60, 60) : WHITE) : RGB(50, 60, 120));
    int t = TOOLS[i].t; u16 bg = RGB(20, 40, 160), gr = RGB(10, 70, 220), ac = RGB(111, 227, 255);
    if (TOOLS[i].h >= 3) {
      u16 pc = portalColor(t); frameRect(cx + 9, cy + 3, cx + 25, cy + 25, pc); frameRect(cx + 10, cy + 4, cx + 24, cy + 24, mix(pc, BLACK, 6));
      const char* lb = t == T_CUBE ? "W" : t == T_SHIP ? "S" : t == T_BALL ? "B" : t == T_UFO ? "U" : t == T_WAVE ? "WV" : t == T_MINI ? "M" : t == T_BIG ? "G" : t == T_GUP ? "UP" : t == T_GDOWN ? "DN" :
                       t == T_SP05 ? ".5" : t == T_SP1 ? "1" : t == T_SP2 ? "2" : t == T_SP3 ? "3" : "4";
      text(cx + 17 - textW(lb, 1) / 2, cy + 10, 1, lb, WHITE, WHITE, 0);
    } else if (t == T_SAW) drawObj(t, cx + 9, cy + 24, 1.f, 1.f, bg, gr, ac, false, false);
    else drawObj(t, cx + 9, cy + 22, 1.f, TOOLS[i].h, bg, gr, ac, false, false);
  }
  fillRect(0, 150, 256, 192, RGB(0, 0, 25));
  snprintf(b, sizeof b, "%s%s", eraseMode ? "RADIERER  " : "", TOOLS[tool].name); text(6, 148, 2, b, eraseMode ? RGB(255, 100, 100) : WHITE, WHITE, 1);
  { u16 hc = RGB(160, 190, 255);
    text(6, 166, 1, "PFEILE: WERKZEUG / SCROLLEN   L/R: SCHNELL", hc, hc, 0);
    text(6, 175, 1, "A:RADIERER B:RUECK X:TEST Y:TEST AB HIER", hc, hc, 0);
    text(6, 184, 1, "START:SPEICHERN SELECT:MENUE L+R+B:LEEREN", hc, hc, 0); }
  // Überblickskarte
  fillRect(4, 136, 252, 148, RGB(10, 14, 50));
  float maxX = 100; for (int i = 0; i < edN; i++) if (ed[i].x > maxX) maxX = ed[i].x + 10; maxX += 20;
  for (int i = 0; i < edN; i++) fillRect(4 + (int)(ed[i].x * 248 / maxX), 136 + 11 - (int)(ed[i].y * 1.1f), 4 + (int)(ed[i].x * 248 / maxX) + 1, 148 - (int)(ed[i].y * 1.1f), ed[i].t == T_BLOCK ? RGB(80, 120, 255) : RGB(255, 200, 60));
  { int vx = 4 + (int)(edX * 248 / maxX); frameRect(vx, 135, vx + (int)(16 * 248 / maxX) + 2, 149, WHITE); }
  if (kh & KEY_L && kh & KEY_R && kh & KEY_B) { edN = 0; msg("LEVEL GELEERT"); }
  // ---- unten: Zeichenfläche ----
  cur = botBuf; drawBackground(RGB(20, 40, 160), RGB(10, 70, 220), RGB(111, 227, 255), (float)edX, -1, 0);
  for (int x = 0; x < 16; x++) { int wx = edX + x; fillRect(x * 16, 16, x * 16 + 1, 176, wx % 5 == 0 ? RGB(60, 100, 220) : RGB(30, 60, 170)); if (wx % 5 == 0) { char n[8]; snprintf(n, sizeof n, "%d", wx); text(x * 16 + 2, 4, 1, n, WHITE, WHITE, 0); } }
  for (int y = 0; y <= 10; y++) fillRect(0, 176 - y * 16, 256, 177 - y * 16, RGB(30, 60, 170));
  float cx0 = (float)edX; camx = cx0;
  for (int pass = 0; pass < 2; pass++) for (int i = 0; i < edN; i++) {
    const LObj& o = ed[i]; if (o.x + o.w < edX - 1 || o.x > edX + 17) continue; if ((o.t == T_DECO) != (pass == 0)) continue;
    drawObj(o.t, SX(o.x), SY(o.y), o.w, o.h, RGB(20, 40, 160), RGB(10, 70, 220), RGB(111, 227, 255), false, false);
  }
  if (tHeld && ty >= 16 && ty < 176) { int bx = tx / 16, by = (176 - ty) / 16; frameRect(bx * 16, 176 - by * 16 - 16, bx * 16 + 16, 176 - by * 16, eraseMode ? RGB(255, 60, 60) : WHITE); }
  if (edMsg > 0) { edMsg--; text(-128, 100, 2, edMsgT, GOLD, GOLD, 1); }
  fillRect(0, 0, 256, 16, RGB(10, 20, 60)); { char n[24]; snprintf(n, sizeof n, "X %d", edX); text(200, 4, 1, n, WHITE, WHITE, 1); }
}

// ======================= Menüs =======================
static void attract(int t, const LLevel* L) {
  cur = topBuf;
  u16 bg = L->colors[0].bg, gr = L->colors[0].ground, ac = L->colors[0].accent;
  float cx = t * 0.17f; camx = cx;
  drawBackground(bg, gr, ac, cx, L->song % 4, musicOn ? beatPulse() : 0);
  for (int i = (int)cx - 1; i < (int)cx + 18; i++) {
    int x = SX((float)i); if (i % 7 == 0) drawObj(T_SPIKE, x, 176, 1, 1, bg, gr, ac, false, false);
    if (i % 11 == 5) { drawObj(T_BLOCK, x, 176, 2, 2, bg, gr, ac, false, false); }
    if (i % 13 == 3) drawObj(T_ORB, x, 130, 1, 1, bg, gr, ac, false, false);
  }
  int ph = t % 66; float jh = ph < 36 ? 4.4f * (ph / 36.f) * (1.f - ph / 36.f) * 2.f : 0.f;
  drawHero(88, 168 - (int)(jh * 16), 8, ph < 36 ? ph * 90 / 36 : 0);
}
static void screenMenu() {
  if (sel < 0 || sel >= totalLevels()) sel = 0;
  const LLevel* L = &LEVELS[sel < NLEVELS ? sel : 0];
  attract(frameNo, L); cur = topBuf;
  gtext(-128, 22, 5, "SKY DASH");
  text(-128, 70, 1, "EIN SPIEL VON RD-GLITCHEDGUY - SKY GAMES", WHITE, WHITE, 1);
  int stars = 0, coins = 0; for (int i = 0; i < NLEVELS; i++) { if (sv.best[i] >= 100) stars += LEVELS[i].stars; coins += bits(sv.coins[i]); }
  char b[64]; snprintf(b, sizeof b, "STERNE %d   MUENZEN %d   VERSUCHE %u", stars, coins, (unsigned)sv.attempts); text(-128, 84, 1, b, GOLD, GOLD, 1);
  cur = botBuf; gradBg(RGB(20, 40, 130), RGB(6, 10, 50));
  for (int i = 0; i < 3; i++) { int px = ((i * 97 + frameNo / 2) % 300) - 20; rotRect(px, 20 + i * 50, 8, 8, frameNo + i * 40, mix(RGB(20, 40, 130), CYAN, 3), mix(RGB(20, 40, 130), CYAN, 5), 1); }
  if (roundBtn(128, 82, 40, ">", 0, 5) || (kd & KEY_A)) { state = ST_LEVELS; }
  if (roundBtn(44, 96, 26, "ICON", 1, 1)) state = ST_CHARS;
  if (roundBtn(212, 96, 26, "OPT", 1, 1)) state = ST_SETTINGS;
  if (pill(10, 150, 74, 30, "EDITOR", 2, 2)) { edLoad(); state = ST_EDITOR; musicStop(); }
  if (pill(90, 150, 76, 30, "CREDITS", 3, 2)) state = ST_CREDITS;
  if (pill(172, 150, 74, 30, "FIGUR", 1, 2)) state = ST_CHARED;
  text(-128, 8, 1, sdOk ? "SD-KARTE ERKANNT - FORTSCHRITT WIRD GESPEICHERT" : "KEINE SD-KARTE - NICHTS WIRD GESPEICHERT", sdOk ? WHITE : RGB(255, 120, 120), sdOk ? WHITE : RGB(255, 120, 120), 1);
}
static const u16 DIFFC[6] = {RGB(110, 255, 90), RGB(80, 200, 255), RGB(255, 200, 50), RGB(255, 130, 40), RGB(255, 60, 60), RGB(190, 80, 255)};
static int diffIdx(const char* d) { return !strcmp(d, "easy") ? 0 : !strcmp(d, "normal") ? 1 : !strcmp(d, "hard") ? 2 : !strcmp(d, "harder") ? 3 : 4; }
static void screenLevels() {
  if (sel < 0 || sel >= totalLevels()) sel = 0;
  const LLevel* L = sel < NLEVELS ? &LEVELS[sel] : 0; int ci = sel - NLEVELS;
  const char* nm = L ? L->name : cFiles[ci];
  cur = topBuf;
  u16 bg = L ? L->colors[0].bg : RGB(20, 40, 160), gr = L ? L->colors[0].ground : RGB(10, 70, 220), ac = L ? L->colors[0].accent : RGB(111, 227, 255);
  drawBackground(bg, gr, ac, frameNo * 0.12f, L ? L->song % 4 : 0, 0);
  rbox(14, 20, 242, 148, RGB(0, 0, 0), BLACK); fillRect(16, 22, 240, 146, mix(bg, BLACK, 9));
  int di = L ? diffIdx(L->diff) : 1; u16 dc = DIFFC[di];
  disc(48, 62, 22, BLACK); disc(48, 62, 19, dc); fillRect(40, 56, 44, 62, BLACK); fillRect(52, 56, 56, 62, BLACK);
  if (di == 0) { for (int i = -8; i <= 8; i++) fillRect(48 + i, 68 + (i * i) / 20, 49 + i, 71 + (i * i) / 20, BLACK); } else if (di == 4) { fillRect(40, 72, 56, 75, BLACK); } else fillRect(40, 70, 56, 73, BLACK);
  { int ts = textW(nm, 3) <= 150 ? 3 : (textW(nm, 2) <= 150 ? 2 : 1); text(80, ts == 3 ? 38 : 42, ts, nm, WHITE, WHITE, ts == 1 ? 1 : 2); }
  char b[64];
  if (L) { snprintf(b, sizeof b, "%s   %d STERNE", L->diff, L->stars); text(80, 70, 1, b, dc, dc, 1); }
  else text(80, 70, 1, "EIGENES LEVEL (SD-KARTE)", GOLD, GOLD, 1);
  if (L) {
    snprintf(b, sizeof b, "NORMAL %d%%", sv.best[sel]); text(24, 100, 1, b, WHITE, WHITE, 1);
    fillRect(22, 110, 232, 120, BLACK); fillRect(24, 112, 24 + 206 * sv.best[sel] / 100, 118, RGB(100, 255, 60));
    snprintf(b, sizeof b, "UEBUNG %d%%", sv.bestP[sel]); text(24, 124, 1, b, WHITE, WHITE, 1);
    fillRect(22, 132, 232, 142, BLACK); fillRect(24, 134, 24 + 206 * sv.bestP[sel] / 100, 140, CYAN);
    for (int i = 0; i < 3; i++) { disc(200 + i * 14, 80, 6, BLACK); disc(200 + i * 14, 80, 4, (sv.coins[sel] >> i) & 1 ? GOLD : RGB(60, 60, 60)); }
  }
  snprintf(b, sizeof b, "LEVEL %d / %d", sel + 1, totalLevels()); text(-128, 156, 1, b, WHITE, WHITE, 1);
  for (int i = 0; i < totalLevels(); i++) disc(128 - totalLevels() * 6 + i * 12 + 6, 172, i == sel ? 4 : 3, i == sel ? WHITE : RGB(60, 70, 120));
  cur = botBuf; gradBg(RGB(20, 40, 130), RGB(6, 10, 50));
  if (roundBtn(36, 96, 28, "<", 0, 4) || (kd & KEY_LEFT)) { sel = (sel + totalLevels() - 1) % totalLevels(); }
  if (roundBtn(220, 96, 28, ">", 0, 4) || (kd & KEY_RIGHT)) { sel = (sel + 1) % totalLevels(); }
  if (roundBtn(128, 96, 40, ">", 0, 5) || (kd & KEY_A)) { const LLevel* lv2 = levelAt(sel); if (lv2) { testFromEditor = false; startLevel(lv2, false); } }
  if (pill(60, 150, 136, 30, "UEBUNGSMODUS", 1, 2)) { const LLevel* lv2 = levelAt(sel); if (lv2) { testFromEditor = false; startLevel(lv2, true); } }
  if (pill(8, 8, 70, 24, "ZURUECK", 3, 1) || (kd & KEY_B)) state = ST_MENU;
  text(-128, 40, 1, "A = START    L/R = LEVEL", WHITE, WHITE, 1);
}
static void screenSettings() {
  cur = topBuf; gradBg(RGB(20, 40, 130), RGB(6, 10, 50)); gtext(-128, 40, 5, "OPTIONEN");
  cur = botBuf; gradBg(RGB(20, 60, 150), RGB(6, 20, 70));
  text(10, 14, 2, "MUSIK", WHITE, WHITE, 1); slider(100, 14, 140, gMusicVol);
  text(10, 44, 2, "EFFEKTE", WHITE, WHITE, 1); slider(100, 44, 140, gSfxVol);
  text(10, 76, 2, "PROZENT", WHITE, WHITE, 1); if (pill(150, 70, 90, 24, showPct ? "AN" : "AUS", 0, 2)) showPct = !showPct;
  text(10, 106, 2, "LEUCHTEN", WHITE, WHITE, 1); if (pill(150, 100, 90, 24, glowOn ? "AN" : "AUS", 0, 2)) glowOn = !glowOn;
  if (pill(10, 140, 130, 28, pauseConfirm ? "SICHER?" : "FORTSCHR. LOESCHEN", 2, 1)) { if (pauseConfirm) { memset(&sv, 0, sizeof sv); pauseConfirm = 0; } else pauseConfirm = 1; }
  if (pill(150, 140, 96, 28, "ZURUECK", 3, 2) || (kd & KEY_B)) { saveGame(); state = ST_MENU; pauseConfirm = 0; }
  text(10, 176, 1, sdOk ? "AUTOMATISCH AUF DER SD-KARTE GESPEICHERT" : "KEINE SD-KARTE GEFUNDEN", WHITE, WHITE, 1);
}
static void screenCredits() {
  cur = topBuf; gradBg(RGB(20, 40, 130), RGB(6, 10, 50)); gtext(-128, 24, 5, "CREDITS"); gtext(-128, 80, 3, "SKY GAMES");
  text(-128, 116, 1, "EIN SPIEL VON RD-GLITCHEDGUY", WHITE, WHITE, 1);
  text(-128, 134, 1, "SPIELIDEE, LEVELDESIGN, WAVE-MODUS", GOLD, GOLD, 1);
  text(-128, 146, 1, "& LOGO-ANIMATION: RD-GLITCHEDGUY", GOLD, GOLD, 1);
  cur = botBuf; gradBg(RGB(20, 60, 150), RGB(6, 20, 70));
  const char* ln[] = {"DS-VERSION & CODE: CLAUDE (ANTHROPIC)", "PHYSIK: PHYSICS.JS VON SKY GAMES", "MODI-WERTE: GEOMETRY DASH ADVANCE (MIT)", "GRAFIK & MUSIK: IM CODE ERZEUGT", "", "INSPIRIERT VON GEOMETRY DASH VON ROBTOP", "GAMES. SKY DASH IST EIN EIGENSTAENDIGES", "FANPROJEKT OHNE VERBINDUNG ZU ROBTOP", "UND OHNE GRAFIKEN/SOUNDS AUS GD."};
  for (int i = 0; i < 9; i++) text(-128, 10 + i * 13, 1, ln[i], i < 4 ? WHITE : RGB(190, 210, 255), i < 4 ? WHITE : RGB(190, 210, 255), 1);
  if (pill(78, 150, 100, 28, "ZURUECK", 3, 2) || (kd & KEY_B)) state = ST_MENU;
}
static void screenPause() {
  cur = botBuf; gradBg(RGB(20, 60, 150), RGB(6, 20, 70));
  text(-128, 10, 3, "PAUSE", WHITE, WHITE, 2);
  char b[64]; snprintf(b, sizeof b, "VERSUCH %d   NORMAL %d%%", attempt, sel < NLEVELS ? sv.best[sel] : 0); text(-128, 40, 1, b, GOLD, GOLD, 1);
  if (roundBtn(44, 96, 30, ">", 0, 4) || (kd & KEY_START)) { state = ST_PLAY; }
  if (roundBtn(112, 96, 30, "R", 1, 3)) { ncp = 0; attempt++; resetAttempt(); musicStop(); musicStart(curLevel->song, curLevel->bpm); state = ST_PLAY; }
  if (roundBtn(180, 96, 30, "P", 2, 3)) { practice = !practice; ncp = 0; attempt++; resetAttempt(); }
  if (pill(10, 150, 110, 30, testFromEditor ? "EDITOR" : "MENUE", 3, 2) || (kd & KEY_SELECT)) { musicStop(); if (testFromEditor) { state = ST_EDITOR; testFromEditor = false; } else { state = ST_LEVELS; musicStart(4, 120); } saveGame(); }
  text(130, 156, 1, practice ? "UEBUNGSMODUS: AN" : "UEBUNGSMODUS: AUS", practice ? CYAN : WHITE, practice ? CYAN : WHITE, 1);
  text(130, 168, 1, "P = UEBUNG AN/AUS", WHITE, WHITE, 1);
}
static void screenComplete() {
  cur = topBuf; drawPlayTop(true);
  gtext(-128, 40, 4, practice ? "UEBUNG GESCHAFFT!" : "LEVEL GESCHAFFT!");
  char b[64]; snprintf(b, sizeof b, "VERSUCHE %d", attempt); text(-128, 100, 2, b, WHITE, WHITE, 1);
  if (!practice && !testFromEditor) { snprintf(b, sizeof b, "%d STERNE", curLevel->stars); text(-128, 124, 2, b, GOLD, GOLD, 1); }
  for (int i = 0; i < 3; i++) { disc(98 + i * 30, 152, 9, BLACK); disc(98 + i * 30, 152, 7, (S.coins >> i) & 1 ? GOLD : RGB(60, 60, 60)); }
  cur = botBuf; gradBg(RGB(20, 60, 150), RGB(6, 20, 70));
  if (roundBtn(80, 96, 34, "R", 1, 4)) { attempt = 1; ncp = 0; resetAttempt(); musicStart(curLevel->song, curLevel->bpm); state = ST_PLAY; }
  if (roundBtn(176, 96, 34, testFromEditor ? "E" : "M", 3, 4) || (kd & KEY_A)) { musicStop(); if (testFromEditor) { state = ST_EDITOR; testFromEditor = false; } else { state = ST_LEVELS; musicStart(4, 120); } }
}
static void screenPlayBottom() {
  cur = botBuf; gradBg(RGB(14, 24, 80), RGB(4, 6, 30));
  text(-128, 12, 2, curLevel->name, WHITE, WHITE, 1);
  char b[48]; snprintf(b, sizeof b, "%d%%", pctOf()); text(-128, 52, 6, b, GOLD, RGB(255, 138, 0), 3);
  snprintf(b, sizeof b, "VERSUCH %d", attempt); text(-128, 112, 2, b, WHITE, WHITE, 1);
  text(-128, 140, 1, practice ? "UEBUNG:  L = CHECKPOINT   R = LOESCHEN" : "TIPPEN / A / B / X / Y / OBEN = SPRINGEN", practice ? CYAN : RGB(190, 210, 255), practice ? CYAN : RGB(190, 210, 255), 1);
  text(-128, 156, 1, "START = PAUSE", RGB(190, 210, 255), RGB(190, 210, 255), 1);
  if (practice) { snprintf(b, sizeof b, "CHECKPOINTS: %d", ncp); text(-128, 170, 1, b, GREEN, GREEN, 1); }
  snprintf(b, sizeof b, "%d FPS", (int)(fpsShown + 0.5f)); text(4, 182, 1, b, RGB(120, 140, 200), RGB(120, 140, 200), 0);
}

// ======================= Hauptschleife =======================
int main() {
  lcdMainOnTop();
  videoSetMode(MODE_FB0); vramSetBankA(VRAM_A_LCD);               // oben: 16-Bit-Framebuffer (direkt in VRAM A)
  videoSetModeSub(MODE_5_2D); vramSetBankC(VRAM_C_SUB_BG);        // unten: 16-Bit-Bitmap-Hintergrund
  int bgSub = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
  u16* subGfx = bgGetGfxPtr(bgSub);
  timerStart(0, ClockDivider_1024, 0, NULL);                       // Hardware-Uhr fuer gleichmaessiges Tempo
  soundEnable(); initTrig();
  for (int i = 0; i < 10; i++) kq[i].ch = -1;
  sdOk = fatInitDefault(); ensureDirs(); loadGame(); scanLevels();
  memset(topBuf, 0, sizeof topBuf); memset(botBuf, 0, sizeof botBuf);

  while (true) {
    scanKeys(); kd = keysDown(); kh = keysHeld();
    touchPosition tp; tDown = (kd & KEY_TOUCH) != 0; tHeld = (kh & KEY_TOUCH) != 0;
    if (tHeld) { touchRead(&tp); tx = tp.px; ty = tp.py; } else { tx = ty = -1; tDown = false; }
    bool hold = (kh & JUMPKEYS) != 0;
    frameNo++; worldFrame++;
    { u32 tk = timerElapsed(0); if (tk > 4000) tk = 4000; frameScale = tk / 545.48f; if (frameScale < 0.25f) frameScale = 0.25f; if (frameScale > 4.f) frameScale = 4.f;
      stepAcc += tk / 136.37f; nsteps = (int)stepAcc; stepAcc -= nsteps; if (nsteps > 12) nsteps = 12; fpsShown = fpsShown * 0.9f + (60.0f / frameScale) * 0.1f; }

    switch (state) {
      case ST_SPLASH: {
        cur = topBuf; fillRect(0, 0, 256, 192, BLACK); rbox(88, 36, 168, 116, WHITE, WHITE); rbox(92, 40, 164, 112, BLACK, BLACK);
        int px = 256 - frameNo * 3; if (px < 108) px = 108; disc(128, 76, 10, GOLD); rotRect(px, 76 + (px - 108) / 2, 14, 4, -30, WHITE, RGB(138, 138, 138), 1);
        text(-128, 126, 2, "SKY GAMES", WHITE, WHITE, 0);
        cur = botBuf; fillRect(0, 0, 256, 192, BLACK); text(-128, 90, 1, "LADEN...", RGB(180, 110, 30), RGB(180, 110, 30), 0);
        if (frameNo > 110 || (kd & (KEY_A | KEY_TOUCH | KEY_START))) { state = ST_MENU; musicStart(4, 120); }
        break; }
      case ST_MENU: screenMenu(); break;
      case ST_LEVELS: screenLevels(); break;
      case ST_SETTINGS: screenSettings(); break;
      case ST_CREDITS: screenCredits(); break;
      case ST_CHARS: screenChars(); break;
      case ST_CHARED: screenCharEd(); break;
      case ST_EDITOR: screenEditor(); break;
      case ST_PLAY:
        if (kd & KEY_START) { state = ST_PAUSE; break; }
        if (practice) { if ((kd & KEY_L) && !S.dead && !S.done && ncp < 16) { cps[ncp] = S; cpRot[ncp] = visRot; ncp++; sfxClick(); } if ((kd & KEY_R) && ncp) { ncp--; sfxClick(); } }
        updatePlay(hold); drawPlayTop(false); screenPlayBottom(); break;
      case ST_PAUSE: drawPlayTop(true); screenPause(); break;
      case ST_COMPLETE: screenComplete(); break;
    }
    musicUpdate();
    swiWaitForVBlank();
    DC_FlushRange(topBuf, sizeof topBuf); DC_FlushRange(botBuf, sizeof botBuf);
    dmaCopy(topBuf, VRAM_A, sizeof topBuf); dmaCopy(botBuf, subGfx, sizeof botBuf);
  }
  return 0;
}
