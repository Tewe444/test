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
// Browser-Renderer: U = Bildschirmhoehe / 12.2, Boden bei H - 1.25*U.
static const float UNIT = 192.0f / 12.2f;
static const float GROUND_Y = 192.0f - 1.25f * UNIT;
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
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > 256) x1 = 256;
  if (y1 > 192) y1 = 192;
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
static void triangleFill(int x1, int y1, int x2, int y2, int x3, int y3, u16 c) {
  int ymin = y1 < y2 ? y1 : y2; if (y3 < ymin) ymin = y3;
  int ymax = y1 > y2 ? y1 : y2; if (y3 > ymax) ymax = y3;
  if (ymin < 0) ymin = 0;
  if (ymax > 191) ymax = 191;
  for (int y = ymin; y <= ymax; ++y) {
    int xs[3], n = 0;
    const int px[3] = {x1, x2, x3}, py[3] = {y1, y2, y3};
    for (int i = 0; i < 3; ++i) {
      int j = (i + 1) % 3;
      if ((y >= py[i] && y < py[j]) || (y >= py[j] && y < py[i])) {
        int den = py[j] - py[i];
        if (den) xs[n++] = px[i] + (px[j] - px[i]) * (y - py[i]) / den;
      }
    }
    if (n >= 2) {
      int a = xs[0] < xs[1] ? xs[0] : xs[1], b = xs[0] > xs[1] ? xs[0] : xs[1];
      fillRect(a, y, b + 1, y + 1, c);
    }
  }
}

// Stachel: dieselbe schmale, weiss umrandete Neon-Spitze wie im Browser.
static void drawSpike(int x, int yb, bool up, u16 bg, u16 accent) {
  const int cx = x + (int)(UNIT * 0.5f + 0.5f);
  const int hw = (int)(UNIT * 0.47f + 0.5f), h = (int)(UNIT * 0.94f + 0.5f);
  const int tipY = up ? yb - h : yb;
  const int baseY = up ? yb : yb - h;
  triangleFill(cx - hw, baseY, cx + hw, baseY, cx, tipY, mix(accent, BLACK, 11));
  triangleFill(cx - (hw * 2) / 3, baseY - (up ? 1 : -1), cx + (hw * 2) / 3, baseY - (up ? 1 : -1), cx, tipY + (up ? 3 : -3), mix(bg, BLACK, 8));
  line(cx - hw, baseY, cx, tipY, WHITE); line(cx, tipY, cx + hw, baseY, WHITE);
  line(cx - hw, baseY, cx + hw, baseY, mix(WHITE, accent, 5));
}

// ======================= Level darstellen =======================
static const u16 P1 = RGB(182, 255, 59), P2 = RGB(0, 234, 255);
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

static unsigned tileHash(int x, int y) {
  unsigned h = (unsigned)(x * 374761393u + y * 668265263u);
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}

static void drawGroundBand(u16 gr, u16 ac, float camx, int top, int bottom) {
  fillRect(0, top, 256, bottom, mix(gr, BLACK, 4));
  const int tile = (int)(UNIT * 2.6f + 0.5f);
  int off = ((int)(camx * UNIT * 0.1f) % tile + tile) % tile;
  for (int x = -off; x < 256 + tile; x += tile) {
    const int h = (int)(tileHash(x / tile, top) & 7u);
    u16 c0 = mix(gr, WHITE, 2 + h / 2), c1 = mix(gr, BLACK, 5 + h / 2);
    fillRect(x + 2, top + 2, x + tile - 2, bottom - 2, mix(c0, c1, 7));
    frameRect(x + 2, top + 2, x + tile - 2, bottom - 2, mix(BLACK, ac, 3));
    line(x + 4, top + 4, x + tile - 5, top + 4, mix(c0, WHITE, 3));
  }
  fillRect(0, top - 2, 256, top + 1, mix(ac, WHITE, 6));
  fillRect(0, top, 256, top + 1, WHITE);
}

static void drawBackground(u16 bg, u16 gr, u16 ac, float camx, float ceil) {
  const u16 dark = mix(bg, BLACK, 9);
  for (int y = 0; y < 192; ++y) {
    const u16 c = mix(bg, dark, y * 16 / 191);
    u16* p = back + y * 256;
    for (int x = 0; x < 256; ++x) p[x] = c;
  }
  // Grosse, langsam vorbeiziehende Hintergrundkacheln (Parallax 10%).
  const int tile = (int)(UNIT * 4.0f + 0.5f);
  const int floorY = (int)(GROUND_Y + 0.5f);
  const int off = ((int)(camx * UNIT * 0.1f) % tile + tile) % tile;
  for (int x = -off - tile; x < 256 + tile; x += tile) {
    for (int y = 0; y < floorY; y += tile) {
      unsigned h = tileHash((x + off) / tile, y / tile);
      u16 hi = mix(WHITE, bg, 12 + (h & 3u));
      u16 lo = mix(BLACK, bg, 10 + ((h >> 3) & 3u));
      fillRect(x + 3, y + 3, x + tile - 3, y + tile - 3, mix(hi, lo, 7));
      frameRect(x + 3, y + 3, x + tile - 3, y + tile - 3, mix(BLACK, bg, 5));
      if (h & 8u) frameRect(x + 14, y + 14, x + tile - 14, y + tile - 14, mix(WHITE, bg, 12));
    }
  }
  drawGroundBand(gr, ac, camx, floorY, 192);
  if (ceil > 0.03f) {
    // Die Decke blendet bei Schiff/Wave und umgedrehter Schwerkraft ein.
    u16 topGr = mix(gr, bg, (int)((1.0f - ceil) * 16.0f));
    drawGroundBand(topGr, ac, camx, 0, (int)(UNIT + 0.5f));
  }
}

#define SX(wx) ((int)(((wx) - camx) * UNIT + 0.5f))
#define SY(wy) ((int)(GROUND_Y - (wy) * UNIT + 0.5f))

static void drawLevel(const Sim& S, float camx, u16 bg, u16 gr, u16 ac) {
  const LLevel* L = S.L;
  int j0 = S.firstAt(camx - L->maxW - 1.0f);
  (void)gr;

  // 1) Deko im Hintergrund
  for (int j = j0; j < L->nobjs; j++) {
    const LObj& o = L->objs[j];
    if (o.x > camx + 17.0f) break;
    if (o.t != T_DECO) continue;
    int x = SX(o.x), yBot = SY(o.y), w = (int)(o.w * UNIT + 0.5f), h = (int)(o.h * UNIT + 0.5f);
    fillRect(x, yBot - h, x + w, yBot, mix(bg, ac, 4));
    line(x, yBot - h, x + w, yBot - h, mix(WHITE, ac, 10));
    line(x, yBot - h, x, yBot, mix(WHITE, bg, 9));
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
        const double objectH = o.t == T_SLAB ? 0.5 : o.h;
        int w = (int)(o.w * UNIT + 0.5f), h = (int)(objectH * UNIT + 0.5f), yTop = yBot - h;
        const int unit = (int)(UNIT + 0.5f);
        fillRect(x, yTop, x + w, yBot, BLACK);
        for (int tx = 0; tx < (int)ceilf((float)o.w); ++tx) for (int ty = 0; ty < (int)ceilf((float)objectH); ++ty) {
          int xx = x + (int)(tx * UNIT + 0.5f), yy = yTop + (int)(ty * UNIT + 0.5f);
          int hh = (int)((objectH - ty) * UNIT + 0.5f); if (hh > unit) hh = unit;
          if (hh <= 0) continue;
          fillRect(xx + 2, yy + 2, xx + unit - 2, yy + hh - 2, mix(bg, BLACK, 9));
          frameRect(xx + 2, yy + 2, xx + unit - 2, yy + hh - 2, mix(WHITE, bg, 13));
          line(xx + 4, yy + 3, xx + unit - 4, yy + 3, mix(ac, WHITE, 8));
        }
        frameRect(x, yTop, x + w, yBot, mix(WHITE, ac, 5));
        break; }
      case T_SPIKE: drawSpike(x, yBot, true, bg, ac); break;
      case T_CSPIKE: drawSpike(x, yBot, false, bg, ac); break;
      case T_ORB: case T_BORB: {
        u16 c = t == T_ORB ? RGB(255, 225, 79) : RGB(79, 200, 255);
        const int u = (int)(UNIT + 0.5f), cx = x + u / 2, cy = yBot - u / 2;
        disc(cx, cy, u / 2, mix(c, bg, 10)); disc(cx, cy, (u * 3) / 8, BLACK);
        disc(cx, cy, u / 4, S.used[j] ? mix(c, BLACK, 9) : c);
        disc(cx - 1, cy - 1, u / 8, WHITE); disc(cx + u / 8, cy + u / 8, 1, mix(c, WHITE, 9));
        break; }
      case T_PAD: {
        const int u = (int)(UNIT + 0.5f);
        fillRect(x + u / 16, yBot - (u * 3) / 8, x + u - u / 16, yBot, BLACK);
        fillRect(x + u / 8, yBot - (u * 5) / 16, x + u - u / 8, yBot - 1, S.used[j] ? mix(RGB(255, 225, 79), BLACK, 8) : RGB(255, 225, 79));
        line(x + u / 4, yBot - u / 4, x + (u * 3) / 4, yBot - u / 4, WHITE);
        break; }
        break;
      case T_COIN:
        if (!(S.coins & (1 << S.cid[j]))) {
          const int u = (int)(UNIT + 0.5f), cx = x + u / 2;
          int w = 2 + (int)(u * 0.18f * fabsf(cosf(worldFrame * 0.08f)));
          fillRect(cx - w - 2, yBot - (u * 15) / 16, cx + w + 3, yBot - 1, BLACK);
          fillRect(cx - w, yBot - (u * 14) / 16, cx + w + 1, yBot - 2, RGB(255, 210, 63));
          fillRect(cx - w + 2, yBot - (u * 12) / 16, cx + w - 1, yBot - (u * 4) / 16, RGB(255, 245, 170));
        }
        break;
      case T_SHIP: case T_CUBE: case T_WAVE: case T_GUP: case T_GDOWN: {
        u16 c = t == T_SHIP ? RGB(255, 90, 214) : t == T_CUBE ? RGB(92, 255, 122) : t == T_WAVE ? RGB(79, 255, 224) : t == T_GUP ? RGB(255, 225, 79) : RGB(79, 200, 255);
        const int u = (int)(UNIT + 0.5f), cx = x + u / 2, cy = yBot - (int)(UNIT * 1.5f);
        disc(cx, cy, (int)(UNIT * 0.75f), mix(c, bg, 10));
        disc(cx, cy, (int)(UNIT * 0.56f), BLACK); disc(cx, cy, (int)(UNIT * 0.44f), mix(c, BLACK, 4));
        line(cx, cy - u / 2, cx, cy + u / 2, WHITE); line(cx - u * 3 / 8, cy, cx + u * 3 / 8, cy, WHITE);
        break; }
      case T_SAW: {
        int r = (int)(o.w * UNIT * 0.5f + 0.5f), cx = x + r, cy = yBot - r;
        disc(cx, cy, r + 2, mix(ac, bg, 11)); disc(cx, cy, r, BLACK);
        for (int k = 0; k < 12; ++k) {
          int a = (worldFrame * 7 + k * 30) % 360;
          int x1 = cx + (CS[a] * (r - 1)) / 256, y1 = cy + (SN[a] * (r - 1)) / 256;
          int x2 = cx + (CS[a] * (r * 0.72f)) / 256, y2 = cy + (SN[a] * (r * 0.72f)) / 256;
          line(cx, cy, x1, y1, RGB(210, 215, 230)); line(cx, cy, x2, y2, RGB(70, 75, 90));
        }
        disc(cx, cy, r / 2, RGB(75, 80, 95)); disc(cx, cy, r / 4, WHITE);
        break; }
      default: break;
    }
  }
}

// Spieler + Wave-Spur
static float trailX[72], trailY[72]; static int trailN = 0;
static void drawPlayer(const Sim& S, float camx) {
  const int cx = (int)((S.x + 0.5f - camx) * UNIT + 0.5f);
  const int cy = (int)(GROUND_Y - (S.y + 0.5f) * UNIT + 0.5f);
  const int deg = (int)(S.rot * 180.0f / G::PI + (S.rot >= 0 ? 0.5f : -0.5f));
  // Der Browser rendert fuer alle Modi eine mitlaufende Sekundaerfarb-Spur.
  if (S.mode != M_WAVE) {
    for (int i = 1; i < trailN; ++i) {
      const int ax = (int)((trailX[i - 1] - camx) * UNIT + 0.5f);
      const int ay = (int)(GROUND_Y - trailY[i - 1] * UNIT + 0.5f);
      const int bx = (int)((trailX[i] - camx) * UNIT + 0.5f);
      const int by = (int)(GROUND_Y - trailY[i] * UNIT + 0.5f);
      const int shade = 13 - (i * 9 / (trailN > 1 ? trailN - 1 : 1));
      line(ax, ay, bx, by, mix(P2, BLACK, shade));
    }
  }
  if (S.mode == M_CUBE) {
    rotRect(cx, cy, (int)(UNIT * 0.5f), (int)(UNIT * 0.5f), deg, P1, BLACK, 2);
    rotRect(cx, cy, 5, 5, deg, P2, BLACK, 1);
    // Icon 0 (Diamant), Sky-Dash-Standardfarben.
    const int d = (deg + 360) % 360;
    (void)d;
    line(cx, cy - 3, cx + 3, cy, BLACK); line(cx + 3, cy, cx, cy + 3, BLACK);
    line(cx, cy + 3, cx - 3, cy, BLACK); line(cx - 3, cy, cx, cy - 3, BLACK);
  } else if (S.mode == M_SHIP) {
    // Kleines Raumschiff mit beweglicher Flamme und zweifarbiger Kabine.
    const int flame = 7 + (worldFrame % 4);
    triangleFill(cx - 7 - flame, cy + 1, cx - 7, cy - 3, cx - 7, cy + 5, RGB(255, 170, 40));
    triangleFill(cx - 7 - flame / 2, cy + 1, cx - 7, cy - 1, cx - 7, cy + 3, RGB(255, 255, 200));
    rotRect(cx, cy + 1, (int)(UNIT * 0.62f), (int)(UNIT * 0.31f), -deg, P1, BLACK, 1);
    rotRect(cx - 1, cy - 1, (int)(UNIT * 0.25f), (int)(UNIT * 0.19f), -deg, P2, BLACK, 1);
    disc(cx + 1, cy - 1, 2, WHITE);
  } else {
    // Wave-Pfeil und mehrteilige Zickzack-Spur.
    for (int i = 1; i < trailN; ++i) {
      int ax = (int)((trailX[i - 1] - camx) * UNIT), ay = (int)(GROUND_Y - trailY[i - 1] * UNIT);
      int bx = (int)((trailX[i] - camx) * UNIT), by = (int)(GROUND_Y - trailY[i] * UNIT);
      line(ax, ay, bx, by, mix(P2, BLACK, 7)); line(ax, ay + 1, bx, by + 1, P2);
    }
    rotRect(cx, cy, (int)(UNIT * 0.44f), (int)(UNIT * 0.25f), deg, P1, BLACK, 1);
    line(cx - 4, cy, cx + 5, cy, P2);
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
static uint8_t lastCoins = 0;

static const u32 JUMPKEYS = KEY_A | KEY_B | KEY_X | KEY_Y | KEY_UP | KEY_TOUCH;

// Sichtbare Drehung wird wie in game.js mit der Bildrate geglaettet,
// unabhaengig von den vier festen Physikschritten.
static void updatePlayerRotation(float dt) {
  if (S.mode == M_CUBE) {
    if (S.grounded) {
      const float target = floorf(S.rot / (G::PI * 0.5f) + 0.5f) * (G::PI * 0.5f);
      float k = dt * 22.0f; if (k > 1.0f) k = 1.0f;
      S.rot += (target - S.rot) * k;
    } else S.rot += 7.48f * (float)S.g * dt;
  } else if (S.mode == M_WAVE) {
    S.rot = -atan2f(S.vy, S.L->speed);
  } else {
    const float want = -atan2f(S.vy, S.L->speed) * 0.9f;
    float k = dt * 14.0f; if (k > 1.0f) k = 1.0f;
    S.rot += (want - S.rot) * k;
  }
}

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
static void startAttempt(bool jumpHeld = false) {
  S.reset(&LEVELS[sel]);
  S.prevHold = jumpHeld;
  trailN = 0; partT = 0; lastCoins = 0;
  musicStop(); musicStart(LEVELS[sel].song, LEVELS[sel].bpm);
  state = ST_PLAY; showPlayText();
}

static void drawEndGate(const Sim& sim, float camx, u16 accent);

static void renderTop(float camx) {
  u16 bg, gr, ac; levelColors(S.L, S.x, bg, gr, ac);
  drawBackground(bg, gr, ac, camx, (S.mode >= 1 || S.g == -1) ? 1.0f : 0.0f);
  drawLevel(S, camx, bg, gr, ac);
  drawEndGate(S, camx, ac);
  if (state != ST_DEAD) drawPlayer(S, camx);
  if (partT > 0) for (int i = 0; i < 16; i++) {
    int px = (int)((parts[i].x - camx) * UNIT), py = (int)(GROUND_Y - parts[i].y * UNIT);
    fillRect(px - 2, py - 2, px + 2, py + 2, (i & 1) ? P1 : P2);
  }
  // Fortschrittsbalken im oberen Rand
  int p = pctOf(S);
  fillRect(63, 4, 193, 11, BLACK); fillRect(65, 6, 191, 9, RGB(40, 40, 40)); fillRect(65, 6, 65 + p * 126 / 100, 9, RGB(100, 255, 60));
}
static void drawEndGate(const Sim& sim, float camx, u16 accent) {
  const int x = (int)((sim.L->length - camx) * UNIT + 0.5f);
  if (x < -64 || x > 320) return;
  const int floorY = (int)(GROUND_Y + 0.5f);
  fillRect(x - 4, 0, x + 5, floorY, mix(accent, BLACK, 10));
  fillRect(x - 2, 0, x + 3, floorY, mix(WHITE, accent, 7));
  line(x - 5, 0, x - 5, floorY, mix(accent, BLACK, 5));
  line(x + 5, 0, x + 5, floorY, WHITE);
  for (int k = 0; k < 7; ++k) {
    const int y = (worldFrame * 3 + k * 29) % (floorY > 0 ? floorY : 1);
    fillRect(x - 8 + (k % 3) * 3, y, x - 5 + (k % 3) * 3, y + 2, WHITE);
  }
}

static void renderAttract(int t) {
  const LLevel* L = &LEVELS[sel];
  u16 bg = L->colors[0].bg, gr = L->colors[0].ground, ac = L->colors[0].accent;
  float camx = t * 0.17f;
  drawBackground(bg, gr, ac, camx, 0.0f);
  u16 blockFill = mix(gr, BLACK, 7);
  for (int i = (int)camx - 1; i < (int)camx + 18; i++) {
    int x = (int)((i - camx) * UNIT);
    const int floorY = (int)(GROUND_Y + 0.5f), unit = (int)(UNIT + 0.5f);
    if (i % 7 == 0) drawSpike(x, floorY, true, bg, ac);
    if (i % 11 == 5) { fillRect(x, floorY - 2 * unit, x + 2 * unit, floorY, blockFill); frameRect(x, floorY - 2 * unit, x + 2 * unit, floorY, ac); }
  }
  int ph = t % 60;
  float h = (ph < 40) ? (ph / 40.f) : 0.f; float yy = 4.f * h * (1.f - h) * 2.2f;
  const int menuX = (int)(72.0f * UNIT / 16.0f + UNIT * 0.5f);
  const int menuY = (int)(GROUND_Y - UNIT * (0.5f + yy) + 0.5f);
  rotRect(menuX, menuY, (int)(UNIT * 0.44f), (int)(UNIT * 0.44f), (ph < 40) ? ph * 90 / 40 : 0, P1, BLACK, 2);
  rotRect(menuX, menuY, (int)(UNIT * 0.19f), (int)(UNIT * 0.19f), (ph < 40) ? ph * 90 / 40 : 0, P2, BLACK, 1);
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
    bool jh = (held & JUMPKEYS) != 0;
    worldFrame++; frame++;

    if (state == ST_MENU) {
      if (down & (KEY_UP | KEY_LEFT)) { sel = (sel + NLEVELS - 1) % NLEVELS; showMenu(); }
      if (down & (KEY_DOWN | KEY_RIGHT)) { sel = (sel + 1) % NLEVELS; showMenu(); }
      if (down & KEY_A) { attempt = 1; startAttempt((held & JUMPKEYS) != 0); }
      renderAttract(frame);
    } else {
      if (down & KEY_START) { musicStop(); state = ST_MENU; showMenu(); }
      else if (down & KEY_SELECT) { attempt++; startAttempt((held & JUMPKEYS) != 0); }
      else if (state == ST_PLAY) {
        // 4 x 1/240 s ergeben exakt den 60-Hz-Update-Takt des DS.
        for (int sub = 0; sub < 4 && !S.dead && !S.done; ++sub) S.step(jh);
        updatePlayerRotation(1.0f / 60.0f);
        if (!S.dead && !S.done) {
          const int maxTrail = S.mode == M_WAVE ? 70 : (S.mode == M_SHIP ? 22 : 12);
          if (trailN < maxTrail) ++trailN;
          else { for (int i = 1; i < maxTrail; ++i) { trailX[i - 1] = trailX[i]; trailY[i - 1] = trailY[i]; } }
          trailX[trailN - 1] = S.x + 0.5f; trailY[trailN - 1] = S.y + 0.5f;
        } else if (trailN > 0) --trailN;
        if (S.coinEvent || S.coins != lastCoins) { sfxCoin(); lastCoins = S.coins; }
        if (S.dead) {
          int p = pctOf(S); if (p > bestPct[sel]) bestPct[sel] = p;
          sfxDie(); musicStop(); spawnParts(S); state = ST_DEAD; deadT = 57;
        } else if (S.done) {
          bestPct[sel] = 100; coinSave[sel] |= S.coins;
          musicStop(); sfxWin(); state = ST_WIN;
          iprintf("\x1b[2J\x1b[8;6HLEVEL GESCHAFFT!\n\x1b[11;8HMuenzen: %d/3\x1b[13;8HVersuche: %d\x1b[20;3HA: Menue   SELECT: nochmal", bits(S.coins), attempt);
        }
        if (state == ST_PLAY && (frame % 8) == 0) iprintf("\x1b[6;2HFortschritt %3d%%   \x1b[10;2HMuenzen %d/3", pctOf(S), bits(S.coins));
      } else if (state == ST_DEAD) {
        const float dt = 1.0f / 60.0f;
        for (int i = 0; i < 16; i++) { parts[i].x += parts[i].vx * dt; parts[i].y += parts[i].vy * dt; parts[i].vy -= 20.f * dt; }
        if (partT > 0) partT--;
        if (--deadT <= 0) { attempt++; startAttempt((held & JUMPKEYS) != 0); }
      } else if (state == ST_WIN) {
        if (down & KEY_A) { state = ST_MENU; showMenu(); }
      }
      if (state != ST_MENU) renderTop(S.x - 4.9f);
      else renderAttract(frame);
    }

    musicUpdate();
    swiWaitForVBlank();
    DC_FlushRange(back, sizeof back);
    dmaCopy(back, VRAM_A, sizeof back);
  }
  return 0;
}
