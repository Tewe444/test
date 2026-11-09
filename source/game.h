// Sky Dash DS - Spiellogik. 1:1-Port von physics.js (Sky Games), erweitert um
// Ball, UFO, Mini, Speed-Portale sowie rosa/rote Ringe und Pads (Werte nach Geometry Dash).
// Enthält nichts von libnds, damit man sie auch am PC testen kann.
#pragma once
#include <stdint.h>
#include <math.h>
#include "levels.h"

#define SIM_MAX 1100
#define MAXCOL 2100
#define BUCKET_MAX 26000

// zusätzliche Objekttypen (1..15 siehe levels.h)
enum { T_BALL = 16, T_UFO, T_MINI, T_BIG, T_SP05, T_SP1, T_SP2, T_SP3, T_SP4, T_PORB, T_PPAD, T_RORB, T_RPAD, T_END_ };
enum { M_CUBE = 0, M_SHIP = 1, M_WAVE = 2, M_BALL = 3, M_UFO = 4 };
enum { EV_JUMP = 1, EV_COIN = 2, EV_ORB = 4, EV_PAD = 8, EV_PORTAL = 16, EV_DIE = 32, EV_DONE = 64 };

namespace C {
  const float G = 100, JUMP = 21, PAD = 28, ORB = 19.5f, BORB = 10, TERM = 34;
  const float SHIP_ACC = 46, SHIP_MAX = 10, SHIP_PAD = 13, SHIP_ORB = 11;
  const float TOP = 10, DT = 1.0f / 240.0f, BUF = 0.15f, TOL = 0.45f, CORE = 0.3f;
  const float MINI = 0.6f;
  // Ball / UFO (aus Geometry Dash Advance, auf die Schwerkraft 100 umgerechnet)
  const float BALL_G = 57, BALL_MAX = 24, BALL_SW = 7.3f;
  const float UFO_UP = 55, UFO_DOWN = 36.7f, UFO_MAX = 12.8f, UFO_JUMP = 12.85f;
  const float SPEEDS[5] = {8.372f, 10.386f, 12.914f, 15.6f, 19.2f};
}

struct Lvl {   // fertig vorbereitetes Level (wie prepare() in physics.js)
  const LLevel* src; int n; float length, speed;
  uint8_t t[SIM_MAX]; float x[SIM_MAX], y[SIM_MAX], w[SIM_MAX], h[SIM_MAX];
  float bx[SIM_MAX], by[SIM_MAX], bw[SIM_MAX], bh[SIM_MAX];
  uint8_t solid[SIM_MAX], cid[SIM_MAX];
  uint16_t colStart[MAXCOL + 2]; uint16_t bucket[BUCKET_MAX]; int ncols;
  float maxW;

  int firstAt(float xmin) const {
    int lo = 0, hi = n;
    while (lo < hi) { int m = (lo + hi) / 2; if (x[m] < xmin) lo = m + 1; else hi = m; }
    return lo;
  }
  void prepare(const LLevel* L) {
    src = L; n = L->nobjs < SIM_MAX ? L->nobjs : SIM_MAX; length = L->length; speed = L->speed; maxW = 1;
    int coin = 0;
    for (int i = 0; i < n; i++) {
      const LObj& o = L->objs[i];
      t[i] = o.t; x[i] = o.x; y[i] = o.y; w[i] = o.w > 0 ? o.w : 1; h[i] = o.h > 0 ? o.h : 1;
      if (o.t == T_SLAB) h[i] = 0.5f;
      if (w[i] > maxW) maxW = w[i];
      solid[i] = o.t == T_BLOCK || o.t == T_SLAB; cid[i] = 0;
      bx[i] = x[i]; by[i] = y[i]; bw[i] = w[i]; bh[i] = h[i];
      switch (o.t) {
        case T_SPIKE: bx[i] = x[i] + 0.36f; bw[i] = 0.28f; bh[i] = 0.5f; break;
        case T_CSPIKE: bx[i] = x[i] + 0.36f; by[i] = y[i] + 0.5f; bw[i] = 0.28f; bh[i] = 0.5f; break;
        case T_ORB: case T_BORB: case T_PORB: case T_RORB: bx[i] = x[i] - 0.3f; by[i] = y[i] - 0.3f; bw[i] = 1.6f; bh[i] = 1.6f; break;
        case T_PAD: case T_PPAD: case T_RPAD: bw[i] = 1; bh[i] = 0.4f; break;
        case T_SAW: bw[i] = bh[i] = w[i]; break;
        case T_COIN: cid[i] = (uint8_t)(coin++ % 3); break;
        case T_SHIP: case T_CUBE: case T_GUP: case T_GDOWN: case T_WAVE: case T_BALL: case T_UFO: case T_MINI: case T_BIG:
        case T_SP05: case T_SP1: case T_SP2: case T_SP3: case T_SP4: bw[i] = 1; bh[i] = 3; break;
        default: break;
      }
    }
    // Spalten-Index: pro ganzer Spalte die Liste der Objekte, die sie berühren
    ncols = (int)length + 4; if (ncols > MAXCOL) ncols = MAXCOL;
    static uint16_t cnt[MAXCOL + 2];
    for (int c = 0; c <= ncols; c++) cnt[c] = 0;
    for (int i = 0; i < n; i++) {
      int a = (int)floorf(bx[i]); if (a < 0) a = 0; int b = (int)floorf(bx[i] + bw[i]);
      for (int c = a; c <= b && c < ncols; c++) cnt[c]++;
    }
    int pos = 0;
    for (int c = 0; c < ncols; c++) { colStart[c] = (uint16_t)pos; pos += cnt[c]; cnt[c] = 0; }
    colStart[ncols] = (uint16_t)pos;
    if (pos > BUCKET_MAX) { n = 0; return; }   // viel zu groß: Level wird nicht geladen
    for (int i = 0; i < n; i++) {
      int a = (int)floorf(bx[i]); if (a < 0) a = 0; int b = (int)floorf(bx[i] + bw[i]);
      for (int c = a; c <= b && c < ncols; c++) bucket[colStart[c] + cnt[c]++] = (uint16_t)i;
    }
  }
};

struct Sim {
  float x, y, vy, buf, speed;
  int mode, g, ev;
  bool grounded, dead, done, prevHold, mini;
  uint8_t coins;
  uint8_t used[SIM_MAX];

  float sz() const { return mini ? C::MINI : 1.0f; }

  void reset(const Lvl& L, float sx = 0) {
    x = sx; y = 0; vy = 0; buf = 0; speed = L.speed; mode = M_CUBE; g = 1; ev = 0;
    grounded = true; dead = false; done = false; prevHold = false; mini = false; coins = 0;
    for (int i = 0; i < SIM_MAX; i++) used[i] = 0;
  }

  // Höhe (Blöcke) -> Startgeschwindigkeit für diese Schwerkraft
  static float vFor(float G, float height) { return sqrtf(2 * G * height); }

  // Geschwindigkeit eines Rings/Pads. kind: 0 gelb, 1 rosa, 2 rot. pad: true = Pad.
  float boost(int kind, bool pad) const {
    static const float cubeO[3] = {0, 1.1f, 3.8f}, cubeP[3] = {0, 1.8f, 6.8f};
    static const float miniO[3] = {1.4f, 0.7f, 2.5f}, miniP[3] = {2.8f, 1.2f, 4.4f};
    static const float ballO[3] = {1.7f, 1.1f, 3.0f}, ballP[3] = {2.5f, 1.2f, 4.05f};
    static const float ufoO[3] = {2.0f, 0.7f, 3.8f}, ufoP[3] = {2.0f, 1.2f, 2.7f};
    static const float shipO[3] = {2.9f, 1.2f, 4.8f}, shipP[3] = {2.34f, 1.2f, 3.7f};
    float h, G = C::G;
    switch (mode) {
      case M_SHIP: h = pad ? shipP[kind] : shipO[kind]; return (pad ? C::SHIP_PAD : C::SHIP_ORB) * sqrtf(h / (pad ? 2.34f : 2.9f)) * (mini ? 0.9f : 1.f);
      case M_BALL: h = (pad ? ballP : ballO)[kind]; G = C::BALL_G; if (mini) h *= 0.65f; break;
      case M_UFO: h = (pad ? ufoP : ufoO)[kind]; G = C::UFO_UP; break;
      default:
        if (kind == 0 && !mini) return pad ? C::PAD : C::ORB;
        h = mini ? (pad ? miniP : miniO)[kind] : (pad ? cubeP : cubeO)[kind];
    }
    return vFor(G, h);
  }

  static bool ov(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
  }

  void step(const Lvl& L, bool hold, float dt) {
    if (dead || done) return;
    const bool fresh = hold && !prevHold;
    if (fresh) buf = C::BUF; else if (buf > 0) buf -= dt;
    prevHold = hold;
    x += speed * dt;
    const float S = sz();
    bool jumped = false;
    int c0 = (int)floorf(x) - 1;
    if (c0 < 0) c0 = 0;

    // Ringe, Pads, Portale, Münzen, Stacheln
    for (int c = c0; c <= c0 + 3 && c < L.ncols; c++) {
      for (int k = L.colStart[c]; k < L.colStart[c + 1]; k++) {
        int i = L.bucket[k];
        int t = L.t[i];
        if (L.solid[i] || t == T_DECO) continue;
        if (!ov(x, y, S, S, L.bx[i], L.by[i], L.bw[i], L.bh[i])) continue;
        const float ins = S * (mode == M_WAVE ? 0.3f : 0.12f);
        if (t == T_SPIKE || t == T_CSPIKE) {
          if (ov(x + ins, y + ins, S - 2 * ins, S - 2 * ins, L.bx[i], L.by[i], L.bw[i], L.bh[i])) { dead = true; ev |= EV_DIE; return; }
          continue;
        }
        if (t == T_SAW) {
          float r = L.w[i] * 0.5f, dx = x + S * 0.5f - (L.x[i] + r), dy = y + S * 0.5f - (L.y[i] + r), rr = r * 0.8f + S * 0.5f - ins;
          if (dx * dx + dy * dy < rr * rr) { dead = true; ev |= EV_DIE; return; }
          continue;
        }
        if (used[i]) continue;
        switch (t) {
          case T_ORB: case T_PORB: case T_RORB:
            if (buf > 0 && mode != M_WAVE) {
              used[i] = 1; buf = 0; jumped = true; grounded = false;
              vy = boost(t == T_ORB ? 0 : t == T_PORB ? 1 : 2, false) * g; ev |= EV_ORB;
            } break;
          case T_BORB:
            if (buf > 0 && mode != M_WAVE) { used[i] = 1; buf = 0; jumped = true; grounded = false; g = -g; vy = -g * C::BORB; ev |= EV_ORB; }
            break;
          case T_PAD: case T_PPAD: case T_RPAD:
            if (mode == M_WAVE) break;
            used[i] = 1; jumped = true; grounded = false;
            vy = boost(t == T_PAD ? 0 : t == T_PPAD ? 1 : 2, true) * g; ev |= EV_PAD; break;
          case T_SHIP: used[i] = 1; if (mode != M_SHIP) { mode = M_SHIP; vy *= 0.4f; } ev |= EV_PORTAL; break;
          case T_WAVE: used[i] = 1; mode = M_WAVE; ev |= EV_PORTAL; break;
          case T_CUBE: used[i] = 1; if (mode != M_CUBE) { mode = M_CUBE; vy *= 0.5f; } ev |= EV_PORTAL; break;
          case T_BALL: used[i] = 1; if (mode != M_BALL) { mode = M_BALL; vy *= 0.4f; } ev |= EV_PORTAL; break;
          case T_UFO: used[i] = 1; if (mode != M_UFO) { mode = M_UFO; vy *= 0.4f; } ev |= EV_PORTAL; break;
          case T_MINI: used[i] = 1; if (!mini) { y += (1 - C::MINI) * 0.5f; mini = true; } ev |= EV_PORTAL; break;
          case T_BIG: used[i] = 1; if (mini) { y -= (1 - C::MINI) * 0.5f; mini = false; } ev |= EV_PORTAL; break;
          case T_SP05: case T_SP1: case T_SP2: case T_SP3: case T_SP4: used[i] = 1; speed = C::SPEEDS[t - T_SP05]; ev |= EV_PORTAL; break;
          case T_GUP: used[i] = 1; if (g == 1) { g = -1; vy *= 0.5f; grounded = false; } ev |= EV_PORTAL; break;
          case T_GDOWN: used[i] = 1; if (g == -1) { g = 1; vy *= 0.5f; grounded = false; } ev |= EV_PORTAL; break;
          case T_COIN: used[i] = 1; coins |= (uint8_t)(1 << L.cid[i]); ev |= EV_COIN; break;
          default: break;
        }
      }
    }
    const float S2 = sz();

    // Bewegung
    if (mode == M_CUBE) {
      if (grounded && hold && !jumped) { vy = C::JUMP * (mini ? 0.8f : 1.f) * g; grounded = false; ev |= EV_JUMP; }
      vy -= C::G * g * dt;
      if (vy > C::TERM) vy = C::TERM; else if (vy < -C::TERM) vy = -C::TERM;
    } else if (mode == M_BALL) {
      if (grounded && buf > 0 && !jumped) { g = -g; vy = -C::BALL_SW * g; grounded = false; buf = 0; ev |= EV_JUMP; }
      vy -= C::BALL_G * g * dt;
      if (vy > C::BALL_MAX) vy = C::BALL_MAX; else if (vy < -C::BALL_MAX) vy = -C::BALL_MAX;
    } else if (mode == M_UFO) {
      if (buf > 0 && !jumped) { vy = C::UFO_JUMP * (mini ? 0.9f : 1.f) * g; buf = 0; ev |= EV_JUMP; }
      else vy -= ((vy * g > 0) ? C::UFO_UP : C::UFO_DOWN) * g * dt;
      if (vy > C::UFO_MAX) vy = C::UFO_MAX; else if (vy < -C::UFO_MAX) vy = -C::UFO_MAX;
    } else if (mode == M_WAVE) {
      vy = (hold ? 1 : -1) * g * speed * (mini ? 2.0f : 1.0f);
    } else {
      vy += (hold ? C::SHIP_ACC : -C::SHIP_ACC) * g * dt;
      if (vy > C::SHIP_MAX) vy = C::SHIP_MAX; else if (vy < -C::SHIP_MAX) vy = -C::SHIP_MAX;
    }
    const float py = y;
    y += vy * dt;
    grounded = false;

    if (y < 0) { y = 0; if (vy < 0) vy = 0; if (g == 1) grounded = true; }
    if (y > C::TOP - S2) { y = C::TOP - S2; if (vy > 0) vy = 0; if (g == -1) grounded = true; }

    // Blöcke
    const bool both = mode == M_SHIP || mode == M_UFO;
    const bool up = both || g == 1, down = both || g == -1;
    const float tol = C::TOL * S2, core = C::CORE * S2;
    for (int c = c0; c <= c0 + 3 && c < L.ncols; c++) {
      for (int k = L.colStart[c]; k < L.colStart[c + 1]; k++) {
        int i = L.bucket[k];
        if (!L.solid[i]) continue;
        const float ox = L.x[i], oy = L.y[i], ow = L.w[i], top = oy + L.h[i], bot = oy;
        if (!(x < ox + ow - 1e-6f && x + S2 > ox + 1e-6f && y < top - 1e-6f && y + S2 > bot + 1e-6f)) continue;
        if (mode == M_WAVE) {
          if (x + 0.3f * S2 < ox + ow && x + 0.7f * S2 > ox && y + 0.3f * S2 < top && y + 0.7f * S2 > bot) { dead = true; ev |= EV_DIE; return; }
          continue;
        }
        if (py >= top - tol && vy <= 0.001f) {
          if (!up) { dead = true; ev |= EV_DIE; return; }
          y = top; vy = 0; if (g == 1) grounded = true;
        } else if (py + S2 <= bot + tol && vy >= -0.001f) {
          if (!down) { dead = true; ev |= EV_DIE; return; }
          y = bot - S2; vy = 0; if (g == -1) grounded = true;
        } else {
          if (x + core < ox + ow && x + S2 - core > ox && y + core < top && y + S2 - core > bot) { dead = true; ev |= EV_DIE; return; }
        }
      }
    }
    if (x >= L.length) { done = true; ev |= EV_DONE; }
  }
};
