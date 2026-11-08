// Sky Dash DS / DSi - Spielphysik
// Portiert die Schrittlogik aus skydash/js/physics.js.
// Einheiten: 1 = ein Level-Block, y zeigt nach oben.
#pragma once
#include <stdint.h>
#include <math.h>
#include "levels.h"

enum { M_CUBE = 0, M_SHIP = 1, M_WAVE = 2 };

namespace G {
  // Die Browser-Version rechnet mit 240 festen Physikschritten/s.
  // main.cpp fuehrt vier Schritte je DS-Bildschirmbild (60 Hz) aus.
  const double DT = 1.0 / 240.0;
  const double G = 100.0;
  const double JUMP = 21.0;
  const double PAD = 28.0;
  const double ORB = 19.5;
  const double BORB = 10.0;
  const double TERM = 34.0;
  const double SHIP_ACC = 46.0;
  const double SHIP_MAX = 10.0;
  const double SHIP_PAD = 13.0;
  const double SHIP_ORB = 11.0;
  const double TOP = 10.0;
  const double BUF = 0.15;
  const double TOL = 0.45;
  const double CORE = 0.3;
  const double PI = 3.14159265358979323846;
}

struct Sim {
  const LLevel* L;
  double x, y, vy, rot, buf;
  int mode, g, frame;
  bool grounded, dead, done, prevHold, coinEvent;
  uint8_t coins;
  uint8_t used[MAX_OBJS];
  uint8_t cid[MAX_OBJS];

  void reset(const LLevel* level) {
    L = level;
    x = 0.0; y = 0.0; vy = 0.0; rot = 0.0; buf = 0.0;
    mode = M_CUBE; g = 1; frame = 0;
    grounded = true; dead = false; done = false; prevHold = false; coinEvent = false;
    coins = 0;
    int coinId = 0;
    for (int i = 0; i < MAX_OBJS; ++i) { used[i] = 0; cid[i] = 0; }
    for (int i = 0; i < L->nobjs; ++i)
      if (L->objs[i].t == T_COIN) cid[i] = (uint8_t)(coinId++ % 3);
  }

  int firstAt(double xmin) const {
    int lo = 0, hi = L->nobjs;
    while (lo < hi) {
      const int mid = (lo + hi) / 2;
      if (L->objs[mid].x < xmin) lo = mid + 1; else hi = mid;
    }
    return lo;
  }

  static bool hit(double ax0, double ay0, double ax1, double ay1,
                  double bx0, double by0, double bx1, double by1) {
    return ax0 < bx1 && ax1 > bx0 && ay0 < by1 && ay1 > by0;
  }

  // Gleiche Bounding-Boxen wie SDPhys.prepare() in physics.js.
  static void bounds(const LObj& o, double& bx, double& by, double& bw, double& bh, bool& solid) {
    bx = o.x; by = o.y; bw = o.w; bh = o.h; solid = false;
    if (o.t == T_BLOCK || o.t == T_SLAB) {
      solid = true;
      if (o.t == T_SLAB) bh = 0.5;
    }
    switch (o.t) {
      case T_SPIKE:  bx = o.x + 0.36; by = o.y;        bw = 0.28; bh = 0.5; break;
      case T_CSPIKE: bx = o.x + 0.36; by = o.y + 0.5; bw = 0.28; bh = 0.5; break;
      case T_ORB: case T_BORB: bx = o.x - 0.3; by = o.y - 0.3; bw = 1.6; bh = 1.6; break;
      case T_PAD: bx = o.x; by = o.y; bw = 1.0; bh = 0.4; break;
      case T_SAW: bx = o.x; by = o.y; bw = o.w; bh = o.w; break;
      case T_SHIP: case T_CUBE: case T_WAVE: case T_GUP: case T_GDOWN:
        bx = o.x; by = o.y; bw = 1.0; bh = 3.0; break;
      default: break;
    }
  }

  void step(bool hold) {
    if (dead || done) return;
    ++frame;
    coinEvent = false;
    if (hold && !prevHold) buf = G::BUF;
    else if (buf > 0.0) buf -= G::DT;
    prevHold = hold;

    x += L->speed * G::DT;
    bool jumped = false;

    // Trigger und Gefahren vor der vertikalen Bewegung (wie physics.js).
    const int j0 = firstAt(x - L->maxW - 1.0);
    for (int j = j0; j < L->nobjs; ++j) {
      const LObj& o = L->objs[j];
      if (o.x > x + 3.0) break;
      double bx, by, bw, bh; bool solid;
      bounds(o, bx, by, bw, bh, solid);
      if (solid || o.t == T_DECO || !hit(x, y, x + 1.0, y + 1.0, bx, by, bx + bw, by + bh)) continue;

      const double ins = mode == M_WAVE ? 0.3 : 0.12;
      if (o.t == T_SPIKE || o.t == T_CSPIKE) {
        if (x + ins < bx + bw && x + 1.0 - ins > bx &&
            y + ins < by + bh && y + 1.0 - ins > by) { dead = true; return; }
        continue;
      }
      if (o.t == T_SAW) {
        const double r = o.w * 0.5;
        const double dx = x + 0.5 - (o.x + r);
        const double dy = y + 0.5 - (o.y + r);
        const double rr = r * 0.8 + 0.5 - ins;
        if (dx * dx + dy * dy < rr * rr) { dead = true; return; }
        continue;
      }
      if (used[j]) continue;

      switch (o.t) {
        case T_ORB:
          if (buf > 0.0 && mode != M_WAVE) {
            used[j] = 1; buf = 0.0; jumped = true; grounded = false;
            vy = (mode ? G::SHIP_ORB : G::ORB) * (double)g;
          }
          break;
        case T_BORB:
          if (buf > 0.0 && mode != M_WAVE) {
            used[j] = 1; buf = 0.0; jumped = true; grounded = false;
            g = -g; vy = -(double)g * G::BORB;
          }
          break;
        case T_PAD:
          if (mode != M_WAVE) {
            used[j] = 1; jumped = true; grounded = false;
            vy = (mode ? G::SHIP_PAD : G::PAD) * (double)g;
          }
          break;
        case T_SHIP:
          used[j] = 1;
          if (mode != M_SHIP) { mode = M_SHIP; vy *= 0.4; }
          break;
        case T_WAVE:
          used[j] = 1; mode = M_WAVE;
          break;
        case T_CUBE:
          used[j] = 1;
          if (mode != M_CUBE) { mode = M_CUBE; vy *= 0.5; }
          break;
        case T_GUP:
          used[j] = 1;
          if (g == 1) { g = -1; vy *= 0.5; grounded = false; }
          break;
        case T_GDOWN:
          used[j] = 1;
          if (g == -1) { g = 1; vy *= 0.5; grounded = false; }
          break;
        case T_COIN: {
          used[j] = 1;
          const uint8_t bit = (uint8_t)(1u << cid[j]);
          if (!(coins & bit)) { coins |= bit; coinEvent = true; }
          break;
        }
        default: break;
      }
    }

    // Bewegung exakt mit den Browser-Konstanten.
    if (mode == M_CUBE) {
      if (grounded && hold && !jumped) { vy = G::JUMP * (double)g; grounded = false; }
      vy -= G::G * (double)g * G::DT;
      if (vy > G::TERM) vy = G::TERM;
      else if (vy < -G::TERM) vy = -G::TERM;
    } else if (mode == M_WAVE) {
      vy = (hold ? 1.0 : -1.0) * (double)g * L->speed;
    } else {
      vy += (hold ? G::SHIP_ACC : -G::SHIP_ACC) * (double)g * G::DT;
      if (vy > G::SHIP_MAX) vy = G::SHIP_MAX;
      else if (vy < -G::SHIP_MAX) vy = -G::SHIP_MAX;
    }

    const double py = y;
    y += vy * G::DT;
    grounded = false;
    if (y < 0.0) {
      y = 0.0; if (vy < 0.0) vy = 0.0;
      if (g == 1) grounded = true;
    }
    if (y > G::TOP - 1.0) {
      y = G::TOP - 1.0; if (vy > 0.0) vy = 0.0;
      if (g == -1) grounded = true;
    }

    // Block- und Slab-Kollisionen mit denselben Kern-/Landeregeln.
    const bool canLandTop = mode >= 1 || g == 1;
    const bool canLandBottom = mode >= 1 || g == -1;
    for (int j = j0; j < L->nobjs; ++j) {
      const LObj& o = L->objs[j];
      if (o.x > x + 3.0) break;
      if (o.t != T_BLOCK && o.t != T_SLAB) continue;
      const double oh = o.t == T_SLAB ? 0.5 : o.h;
      if (!(x < o.x + o.w - 1e-6 && x + 1.0 > o.x + 1e-6 &&
            y < o.y + oh - 1e-6 && y + 1.0 > o.y + 1e-6)) continue;
      const double top = o.y + oh, bot = o.y;
      if (mode == M_WAVE) {
        if (x + 0.3 < o.x + o.w && x + 0.7 > o.x &&
            y + 0.3 < top && y + 0.7 > bot) { dead = true; return; }
        continue;
      }
      if (py >= top - G::TOL && vy <= 0.001) {
        if (!canLandTop) { dead = true; return; }
        y = top; vy = 0.0; if (g == 1) grounded = true;
      } else if (py + 1.0 <= bot + G::TOL && vy >= -0.001) {
        if (!canLandBottom) { dead = true; return; }
        y = bot - 1.0; vy = 0.0; if (g == -1) grounded = true;
      } else {
        if (x + G::CORE < o.x + o.w && x + 1.0 - G::CORE > o.x &&
            y + G::CORE < top && y + 1.0 - G::CORE > bot) { dead = true; return; }
      }
    }

    if (x >= L->length) done = true;
  }
};
