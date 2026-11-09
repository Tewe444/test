// Sky Dash DS - Grafik: Puffer, Formen, Schrift, Icons, Level zeichnen
#pragma once
#include <nds.h>
#include <string.h>
#include <math.h>
#include "game.h"

static u16 topBuf[256 * 192], botBuf[256 * 192];
static u16* cur = topBuf;   // aktuell beschriebener Bildschirm
#define RGB(r, g, b) ((u16)(((r) >> 3) | (((g) >> 3) << 5) | (((b) >> 3) << 10) | 0x8000))
static const u16 BLACK = RGB(0, 0, 0), WHITE = RGB(255, 255, 255), GOLD = RGB(255, 210, 30), GREEN = RGB(109, 255, 58), CYAN = RGB(47, 230, 255);

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
  for (int y = y0; y < y1; y++) { u16* p = cur + y * 256 + x0; for (int x = x0; x < x1; x++) *p++ = c; }
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
    if ((unsigned)x0 < 256 && (unsigned)y0 < 192) cur[y0 * 256 + x0] = c;
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
  int c = CS[deg], s = SN[deg], R = (hu > hv ? hu + hv / 2 : hv + hu / 2) + 1;
  for (int dy = -R; dy <= R; dy++) {
    int y = cy + dy; if ((unsigned)y >= 192) continue;
    for (int dx = -R; dx <= R; dx++) {
      int x = cx + dx; if ((unsigned)x >= 256) continue;
      int u = (dx * c + dy * s) >> 8, v = (-dx * s + dy * c) >> 8;
      int au = u < 0 ? -u : u, av = v < 0 ? -v : v;
      if (au <= hu && av <= hv) cur[y * 256 + x] = (au > hu - bw || av > hv - bw) ? border : fill;
    }
  }
}
static void gradBg(u16 top, u16 bot) {
  for (int y = 0; y < 192; y++) { u16 c = mix(top, bot, y * 16 / 191); u16* p = cur + y * 256; for (int x = 0; x < 256; x++) p[x] = c; }
}
static void rbox(int x0, int y0, int x1, int y1, u16 fill, u16 border) {   // Kasten mit abgerundeten Ecken
  fillRect(x0 + 2, y0, x1 - 2, y1, border); fillRect(x0, y0 + 2, x1, y1 - 2, border);
  fillRect(x0 + 3, y0 + 1, x1 - 3, y1 - 1, fill); fillRect(x0 + 1, y0 + 3, x1 - 1, y1 - 3, fill);
}

// ---------- Schrift (5x7) ----------
static const uint8_t FONT[59][5] = {
 {0,0,0,0,0},{0,0,0x5F,0,0},{0,7,0,7,0},{0x14,0x7F,0x14,0x7F,0x14},{0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,8,0x64,0x62},{0x36,0x49,0x56,0x20,0x50},{0,5,3,0,0},
 {0,0x1C,0x22,0x41,0},{0,0x41,0x22,0x1C,0},{0x2A,0x1C,0x7F,0x1C,0x2A},{8,8,0x3E,8,8},{0,0x80,0x70,0x30,0},{8,8,8,8,8},{0,0,0x60,0x60,0},{0x20,0x10,8,4,2},
 {0x3E,0x51,0x49,0x45,0x3E},{0,0x42,0x7F,0x40,0},{0x72,0x49,0x49,0x49,0x46},{0x21,0x41,0x49,0x4D,0x33},{0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x31},{0x41,0x21,0x11,9,7},{0x36,0x49,0x49,0x49,0x36},{0x46,0x49,0x49,0x29,0x1E},
 {0,0,0x14,0,0},{0,0x40,0x34,0,0},{0,8,0x14,0x22,0x41},{0x14,0x14,0x14,0x14,0x14},{0,0x41,0x22,0x14,8},{2,1,0x59,9,6},{0x3E,0x41,0x5D,0x59,0x4E},
 {0x7C,0x12,0x11,0x12,0x7C},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},{0x7F,0x41,0x41,0x41,0x3E},{0x7F,0x49,0x49,0x49,0x41},{0x7F,9,9,9,1},{0x3E,0x41,0x41,0x51,0x73},{0x7F,8,8,8,0x7F},
 {0,0x41,0x7F,0x41,0},{0x20,0x40,0x41,0x3F,1},{0x7F,8,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},{0x7F,2,0x1C,2,0x7F},{0x7F,4,8,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},{0x7F,9,9,9,6},
 {0x3E,0x41,0x51,0x21,0x5E},{0x7F,9,0x19,0x29,0x46},{0x26,0x49,0x49,0x49,0x32},{3,1,0x7F,1,3},{0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,8,0x14,0x63},{3,4,0x78,4,3},{0x61,0x59,0x49,0x4D,0x43}};

struct Gl { int g; bool uml; };
static int decode(const char* s, Gl* out, int max) {   // UTF-8 -> Glyphen (ä ö ü ß werden zu A O U S mit Punkten)
  int n = 0;
  while (*s && n < max) {
    unsigned char c = (unsigned char)*s++; bool u = false;
    if (c == 0xC3 && *s) {
      unsigned char d = (unsigned char)*s++;
      c = d == 0xA4 || d == 0x84 ? 'A' : d == 0xB6 || d == 0x96 ? 'O' : d == 0xBC || d == 0x9C ? 'U' : d == 0x9F ? 'S' : '?';
      u = d != 0x9F;
    }
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c < 32 || c > 90) c = '?';
    out[n].g = c - 32; out[n].uml = u; n++;
  }
  return n;
}
static int textW(const char* s, int sc) { Gl g[96]; int n = decode(s, g, 96); return n ? n * 6 * sc - sc : 0; }
static void glyphPass(int x, int y, int sc, const Gl& gl, u16 top, u16 bot, int ol, bool outline) {
  for (int col = 0; col < 5; col++) for (int row = 0; row < 7; row++) {
    if (!(FONT[gl.g][col] & (1 << row))) continue;
    int px = x + col * sc, py = y + row * sc;
    if (outline) fillRect(px - ol, py - ol, px + sc + ol, py + sc + ol, BLACK);
    else fillRect(px, py, px + sc, py + sc, mix(top, bot, row * 16 / 6));
  }
  if (gl.uml) {
    for (int k = 0; k < 2; k++) {
      int px = x + (1 + k * 2) * sc, py = y - 2 * sc;
      if (outline) fillRect(px - ol, py - ol, px + sc + ol, py + sc + ol, BLACK); else fillRect(px, py, px + sc, py + sc, top);
    }
  }
}
// Text mit schwarzem Rand und Farbverlauf (top→bot); x<0: zentriert um -x
static void text(int x, int y, int sc, const char* s, u16 top = WHITE, u16 bot = WHITE, int ol = 1) {
  Gl g[96]; int n = decode(s, g, 96);
  if (x < 0) x = -x - textW(s, sc) / 2;
  if (ol > 0) for (int i = 0; i < n; i++) glyphPass(x + i * 6 * sc, y, sc, g[i], top, bot, ol, true);
  for (int i = 0; i < n; i++) glyphPass(x + i * 6 * sc, y, sc, g[i], top, bot, ol, false);
}
static void gtext(int x, int y, int sc, const char* s) { text(x, y, sc, s, RGB(255, 251, 196), RGB(255, 138, 0), sc > 3 ? 3 : 2); }   // goldener Titel
static void textf(int x, int y, int sc, u16 col, const char* fmt, int a = 0, int b = 0, int c = 0) {
  char buf[64]; snprintf(buf, sizeof buf, fmt, a, b, c); text(x, y, sc, buf, col, col);
}

// ---------- Spielwelt zeichnen ----------
static int worldFrame = 0;
static const u16 PAL[12] = {RGB(0,234,255), RGB(255,225,79), RGB(109,255,58), RGB(255,90,214), RGB(255,80,80), RGB(255,150,40),
                            RGB(80,140,255), RGB(150,90,255), RGB(255,255,255), RGB(150,150,160), RGB(30,30,40), RGB(120,70,30)};
static void levelColors(const LLevel* L, float x, u16& bg, u16& gr, u16& ac) {
  int i = 0;
  while (i + 1 < L->ncolors && L->colors[i + 1].x <= x) i++;
  const LColor& c = L->colors[i];
  bg = c.bg; gr = c.ground; ac = c.accent;
  if (i > 0) {
    float t = (x - c.x) / 8.0f;
    if (t < 1.0f) { int tt = (int)(t * 16); const LColor& p = L->colors[i - 1]; bg = mix(p.bg, c.bg, tt); gr = mix(p.ground, c.ground, tt); ac = mix(p.accent, c.accent, tt); }
  }
}
static void drawSpike(int x, int yb, bool up) {
  int cx = x + 8;
  for (int r = 0; r < 16; r++) {
    int hw = 1 + (r * 7) / 15, y = up ? (yb - 16 + r) : (yb - 1 - r);
    fillRect(cx - hw, y, cx + hw, y + 1, WHITE);
    if (r >= 2 && r < 15) fillRect(cx - hw + 2, y, cx + hw - 2, y + 1, BLACK);
  }
}
static bool glowOn = true;
// ein Objekt an Pixelposition x (links) / yBot (unten) zeichnen
static void drawObj(int t, int x, int yBot, float w, float h, u16 bg, u16 gr, u16 ac, bool used, bool taken) {
  u16 blockFill = mix(gr, BLACK, 7);
  switch (t) {
    case T_DECO: fillRect(x, yBot - (int)(h * 16), x + (int)(w * 16), yBot, mix(bg, ac, 3)); break;
    case T_BLOCK: case T_SLAB: {
      int pw = (int)(w * 16), ph = (int)(h * 16), yTop = yBot - ph;
      fillRect(x, yTop, x + pw, yBot, blockFill); frameRect(x, yTop, x + pw, yBot, ac);
      if (pw > 4 && ph > 4) frameRect(x + 2, yTop + 2, x + pw - 2, yBot - 2, mix(blockFill, ac, 4));
      break; }
    case T_SPIKE: drawSpike(x, yBot, true); break;
    case T_CSPIKE: drawSpike(x, yBot, false); break;
    case T_ORB: case T_BORB: {
      u16 c = t == T_ORB ? RGB(255, 225, 40) : RGB(60, 160, 255);
      if (glowOn && !used) disc(x + 8, yBot - 8, 10, mix(bg, c, 5));
      disc(x + 8, yBot - 8, 7, BLACK); disc(x + 8, yBot - 8, 5, used ? mix(c, BLACK, 8) : c); disc(x + 8, yBot - 8, 2, WHITE);
      break; }
    case T_PAD:
      fillRect(x + 1, yBot - 5, x + 15, yBot, BLACK); fillRect(x + 2, yBot - 4, x + 14, yBot - 1, used ? RGB(160, 120, 0) : RGB(255, 225, 40));
      break;
    case T_COIN:
      if (!taken) {
        if (glowOn) disc(x + 8, yBot - 8, 10, mix(bg, GOLD, 5));
        disc(x + 8, yBot - 8, 7, BLACK); disc(x + 8, yBot - 8, 5, GOLD); disc(x + 8, yBot - 8, 2, RGB(255, 250, 190));
      }
      break;
    case T_PORB: case T_RORB: {
      u16 c = t == T_PORB ? RGB(255, 90, 214) : RGB(255, 60, 60);
      if (glowOn && !used) disc(x + 8, yBot - 8, 10, mix(bg, c, 5));
      disc(x + 8, yBot - 8, 7, BLACK); disc(x + 8, yBot - 8, 5, used ? mix(c, BLACK, 8) : c); disc(x + 8, yBot - 8, 2, WHITE);
      break; }
    case T_PPAD: case T_RPAD: {
      u16 c = t == T_PPAD ? RGB(255, 90, 214) : RGB(255, 60, 60);
      fillRect(x + 1, yBot - 5, x + 15, yBot, BLACK); fillRect(x + 2, yBot - 4, x + 14, yBot - 1, used ? mix(c, BLACK, 8) : c);
      break; }
    case T_SHIP: case T_CUBE: case T_WAVE: case T_GUP: case T_GDOWN: case T_BALL: case T_UFO: case T_MINI: case T_BIG: {
      u16 c = t == T_SHIP ? RGB(255, 80, 214) : t == T_CUBE ? RGB(109, 255, 58) : t == T_WAVE ? RGB(0, 234, 255) : t == T_GUP ? RGB(255, 225, 79) :
              t == T_GDOWN ? RGB(80, 140, 255) : t == T_BALL ? RGB(255, 140, 40) : t == T_UFO ? RGB(190, 120, 255) : t == T_MINI ? RGB(255, 120, 160) : RGB(120, 255, 200);
      int ph = (worldFrame >> 2) & 3;
      for (int k = 0; k < 5; k++) { int xo = (k == 0 || k == 4) ? 3 : (k == 1 || k == 3) ? 1 : 0; u16 cc = (k + ph) % 3 == 0 ? WHITE : c;
        fillRect(x + 3 + xo, yBot - 48 + k * 9, x + 6 + xo, yBot - 48 + k * 9 + 9, k == ph + 0 ? WHITE : c); (void)cc; }
      frameRect(x + 3, yBot - 48, x + 13, yBot, c); frameRect(x + 4, yBot - 47, x + 12, yBot - 1, mix(c, BLACK, 6));
      break; }
    case T_SP05: case T_SP1: case T_SP2: case T_SP3: case T_SP4: {
      static const u16 SC[5] = {RGB(255, 170, 40), RGB(80, 160, 255), RGB(110, 255, 90), RGB(255, 80, 220), RGB(255, 70, 70)};
      u16 c = SC[t - T_SP05];
      frameRect(x + 1, yBot - 48, x + 15, yBot, c);
      int n = t - T_SP05 + 1; if (n > 4) n = 4; if (t == T_SP05) n = 1;
      for (int k = 0; k < n; k++) { int ay = yBot - 40 + k * 10; line(x + 4, ay, x + 8, ay + 4, c); line(x + 8, ay + 4, x + 12, ay, c); line(x + 4, ay + 1, x + 8, ay + 5, c); line(x + 8, ay + 5, x + 12, ay + 1, c); }
      break; }
    case T_SAW: {
      int r = (int)(w * 8), cx = x + r, cy = yBot - r;
      disc(cx, cy, r, BLACK); disc(cx, cy, r - 2, RGB(200, 200, 210)); disc(cx, cy, r / 3 + 1, RGB(60, 60, 70));
      for (int k = 0; k < 4; k++) { int a = (worldFrame * 9 + k * 90) % 360; line(cx, cy, cx + (CS[a] * (r - 1)) / 256, cy + (SN[a] * (r - 1)) / 256, BLACK); }
      break; }
    default: break;
  }
}
static u16 portalColor(int t) {
  return t == T_SHIP ? RGB(255, 80, 214) : t == T_CUBE ? RGB(109, 255, 58) : t == T_WAVE ? RGB(0, 234, 255) : t == T_GUP ? RGB(255, 225, 79) :
         t == T_GDOWN ? RGB(80, 140, 255) : t == T_BALL ? RGB(255, 140, 40) : t == T_UFO ? RGB(190, 120, 255) : t == T_MINI ? RGB(255, 120, 160) :
         t == T_BIG ? RGB(120, 255, 200) : t == T_SP05 ? RGB(255, 170, 40) : t == T_SP1 ? RGB(80, 160, 255) : t == T_SP2 ? RGB(110, 255, 90) : t == T_SP3 ? RGB(255, 80, 220) : RGB(255, 70, 70);
}
#define SX(wx) ((int)(((wx) - camx + 64.f) * 16.f) - 1024)
#define SY(wy) (176 - (int)(((wy) + 64.f) * 16.f) + 1024)
static void drawLevel(const Lvl& L, const Sim& S, float camx, u16 bg, u16 gr, u16 ac) {
  int j0 = L.firstAt(camx - L.maxW - 1.0f);
  for (int pass = 0; pass < 2; pass++)
    for (int j = j0; j < L.n; j++) {
      if (L.x[j] > camx + 17.0f) break;
      int t = L.t[j];
      if ((t == T_DECO) != (pass == 0)) continue;
      drawObj(t, SX(L.x[j]), SY(L.y[j]), L.w[j], L.h[j], bg, gr, ac, S.used[j] != 0, (S.coins & (1 << L.cid[j])) != 0);
    }
}

// ---------- Hintergrund mit Ebenen (Parallax), pulsiert im Takt ----------
static inline unsigned hash32(unsigned v) { v ^= v >> 16; v *= 0x7feb352dU; v ^= v >> 15; v *= 0x846ca68bU; v ^= v >> 16; return v; }
static void tri(int cx, int cy, int r, u16 c, bool up) {
  for (int i = 0; i < r * 2; i++) { int hw = i / 2, y = up ? cy - r + i : cy + r - i; fillRect(cx - hw, y, cx + hw + 1, y + 1, c); }
}
static void ringShape(int cx, int cy, int r, u16 c) { disc(cx, cy, r, c); disc(cx, cy, r / 2, mix(c, BLACK, 4)); }
static void diamond(int cx, int cy, int r, u16 c) { for (int dy = -r; dy <= r; dy++) { int w = r - (dy < 0 ? -dy : dy); fillRect(cx - w, cy + dy, cx + w + 1, cy + dy + 1, c); } }
static void parallax(float camx, u16 bg, u16 ac, int theme, int pulse) {
  for (int layer = 0; layer < 3; layer++) {
    float f = 0.12f + layer * 0.14f, sp = 7.0f - layer * 1.5f;
    u16 col = mix(bg, ac, 2 + layer + (pulse > 8 ? 1 : 0));
    float off = camx * f;
    int first = (int)floorf(off / sp) - 1;
    for (int i = first; i < first + (layer == 2 ? 9 : 12); i++) {
      unsigned h = hash32((unsigned)(i * 7 + layer * 131 + theme * 17));
      int sx = (int)((i * sp - off) * 16.f), sy = 24 + (int)(h % 120);
      int r = 8 + (int)((h >> 8) % 14) + layer * 6;
      if (sx < -40 || sx > 300) continue;
      switch ((theme + (h >> 20)) % 4) {
        case 0: tri(sx, sy, r, col, (h >> 4) & 1); break;
        case 1: ringShape(sx, sy, r, col); break;
        case 2: diamond(sx, sy, r, col); diamond(sx, sy, r / 2, mix(col, WHITE, 3)); break;
        default: fillRect(sx - r, sy - r / 3, sx + r, sy + r / 3, col); fillRect(sx - r / 3, sy - r, sx + r / 3, sy + r, col); break;
      }
    }
  }
}
static void drawBackground(u16 bg, u16 gr, u16 ac, float camx, int theme, int pulse) {
  gradBg(mix(bg, WHITE, 1), mix(bg, BLACK, 9));
  if (theme >= 0) parallax(camx, bg, ac, theme, pulse);
  u16 g1 = mix(gr, BLACK, 5);
  fillRect(0, 176, 256, 192, g1); fillRect(0, 176, 256, 178, mix(ac, WHITE, pulse > 8 ? 8 : 3)); fillRect(0, 178, 256, 180, mix(ac, BLACK, 6));
  for (int x = -((int)(camx * 16)) % 32; x < 256; x += 32) fillRect(x < 0 ? 0 : x, 182, x + 14, 190, mix(g1, ac, 2));
  fillRect(0, 0, 256, 16, g1); fillRect(0, 14, 256, 16, mix(ac, WHITE, pulse > 8 ? 8 : 3));
}

// ---------- Icons (Würfel-Designs) ----------
#define NICONS 8
static void drawIcon(int cx, int cy, int half, int id, u16 c1, u16 c2, int deg) {
  rotRect(cx, cy, half, half, deg, c1, BLACK, half > 9 ? 3 : 2);
  deg %= 360; if (deg < 0) deg += 360;
  int cs = CS[deg], sn = SN[deg];
  auto piece = [&](int ox, int oy, int hu, int hv) {   // Teil mit Versatz (in Einheiten von half/8) drehen
    int ux = ox * half / 8, uy = oy * half / 8;
    int px = cx + ((ux * cs - uy * sn) >> 8), py = cy + ((ux * sn + uy * cs) >> 8);
    rotRect(px, py, hu * half / 8 + 1, hv * half / 8 + 1, deg, c2, BLACK, 1);
  };
  switch (id) {
    case 0: piece(0, 0, 3, 3); break;
    case 1: piece(-3, -2, 1, 2); piece(3, -2, 1, 2); piece(0, 3, 3, 0); break;
    case 2: piece(0, 0, 5, 1); piece(0, 0, 1, 5); break;
    case 3: rotRect(cx, cy, half * 5 / 8, half * 5 / 8, deg, c2, BLACK, 1); rotRect(cx, cy, half * 3 / 8, half * 3 / 8, deg, c1, c1, 1); break;
    case 4: piece(0, -3, 5, 1); piece(0, 3, 5, 1); break;
    case 5: rotRect(cx, cy, half * 5 / 8, half * 5 / 8, deg + 45, c2, BLACK, 1); break;
    case 6: disc(cx, cy, half / 2, BLACK); disc(cx, cy, half / 2 - 1, c2); break;
    default: piece(-4, -4, 1, 1); piece(4, -4, 1, 1); piece(-4, 4, 1, 1); piece(4, 4, 1, 1); piece(0, 0, 2, 2); break;
  }
}

// ---------- eigene Figur ----------
static uint8_t sprite[16][16];     // Farbindex 0 = durchsichtig, 1..15 = Palette
static const u16 SPAL[16] = {0, RGB(0,0,0), RGB(255,255,255), RGB(150,150,160), RGB(255,60,60), RGB(255,150,40), RGB(255,225,79), RGB(109,255,58),
                             RGB(0,234,255), RGB(80,140,255), RGB(150,90,255), RGB(255,90,214), RGB(120,70,30), RGB(255,205,160), RGB(40,40,70), RGB(0,120,60)};
static bool hasSprite = false;
static void drawSprite(int cx, int cy, int half, int deg) {
  deg %= 360; if (deg < 0) deg += 360;
  int c = CS[deg], s = SN[deg], R = half * 3 / 2 + 1;
  for (int dy = -R; dy <= R; dy++) {
    int y = cy + dy; if ((unsigned)y >= 192) continue;
    for (int dx = -R; dx <= R; dx++) {
      int x = cx + dx; if ((unsigned)x >= 256) continue;
      int u = (dx * c + dy * s) >> 8, v = (-dx * s + dy * c) >> 8;
      if (u < -half || u >= half || v < -half || v >= half) continue;
      int su = (u + half) * 16 / (2 * half), sv = (v + half) * 16 / (2 * half);
      uint8_t p = sprite[sv][su]; if (p) cur[y * 256 + x] = SPAL[p];
    }
  }
}
static int selIcon = 0, selC1 = 6, selC2 = 0;   // selIcon == NICONS: eigene Figur
static void drawHero(int cx, int cy, int half, int deg) {
  if (selIcon >= NICONS && hasSprite) drawSprite(cx, cy, half, deg);
  else drawIcon(cx, cy, half, selIcon % NICONS, PAL[selC1], PAL[selC2], deg);
}
// alle 5 Spielformen; half = halbe Kantenlänge in Pixeln (8 = normal, 5 = mini)
static void drawPlayer(int mode, int cx, int cy, int half, int deg, bool flip) {
  u16 c1 = PAL[selC1], c2 = PAL[selC2];
  switch (mode) {
    case M_CUBE: drawHero(cx, cy, half, deg); break;
    case M_BALL: disc(cx, cy, half + 1, BLACK); disc(cx, cy, half - 1, c1); drawIcon(cx, cy, half * 2 / 3, selIcon % NICONS, c2, c1, deg * 2); break;
    case M_SHIP: {
      int fy = flip ? -1 : 1;
      rotRect(cx, cy + fy * half / 3, half + 3, half / 2 + 1, -deg * fy, c1, BLACK, 2);
      disc(cx + half / 3, cy - fy * half / 4, half / 2 + 1, BLACK); disc(cx + half / 3, cy - fy * half / 4, half / 2 - 1, c2);
      fillRect(cx - half - 3, cy + fy * half / 3 - 1, cx - half, cy + fy * half / 3 + 2, WHITE); break; }
    case M_UFO: {
      disc(cx, cy - half / 4, half * 2 / 3 + 1, BLACK); disc(cx, cy - half / 4, half * 2 / 3 - 1, c2);
      rotRect(cx, cy + half / 3, half + 3, half / 3 + 1, 0, c1, BLACK, 2); fillRect(cx - 2, cy + half / 3 - 1, cx + 3, cy + half / 3 + 1, WHITE); break; }
    default: rotRect(cx, cy, half * 3 / 5 + 1, half * 3 / 5 + 1, 45 + deg, c1, BLACK, 1); rotRect(cx, cy, half / 3, half / 3, 45 + deg, c2, c2, 1); break;
  }
}
