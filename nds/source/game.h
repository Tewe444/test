// Sky Dash DS - Spiellogik (Physik, Kollisionen, Objekte)
// Enthält absichtlich nichts von libnds, damit man sie auch am PC testen kann.
#pragma once
#include <stdint.h>
#include <math.h>
#include "levels.h"

enum { M_CUBE = 0, M_SHIP = 1, M_WAVE = 2 };

namespace G {
  const float DT = 1.0f / 60.0f;
  const float JUMP_V = 14.5f;   // Sprunggeschwindigkeit des Würfels (Blöcke/s)
  const float GRAV = 50.0f;     // Schwerkraft (Blöcke/s²)
  const float PAD_V = 20.0f;    // Sprungpad
  const float ORB_V = 14.5f;    // gelber Ring
  const float VMAX = 28.0f;     // maximale Fallgeschwindigkeit
  const float SHIP_UP = 34.0f, SHIP_DN = 28.0f, SHIP_VMAX = 9.5f;
  const float STEP = 0.12f;     // Schiff/Wave: kleine Kanten werden "hochgerutscht"
  const float HEIGHT = 10.0f;   // Levelhöhe in Blöcken
}

struct Sim {
  const LLevel* L;
  float x, y, vy, rot, buf;     // x,y = linke untere Ecke der Hitbox
  int mode, grav, frame;        // grav = +1: normal (fällt nach unten), -1: kopfüber
  bool ground, dead, won;
  uint8_t coins;                // Bitmaske der in diesem Versuch eingesammelten Münzen
  uint8_t used[MAX_OBJS];       // Ringe/Pads, die schon benutzt wurden
  uint8_t cid[MAX_OBJS];        // Münznummer (0..2) pro Objekt

  float pw() const { return mode == M_WAVE ? 0.35f : 0.9f; }
  float ph() const { return mode == M_WAVE ? 0.35f : (mode == M_SHIP ? 0.7f : 0.9f); }

  void reset(const LLevel* l) {
    L = l; x = 0; y = 0; vy = 0; rot = 0; buf = 0; mode = M_CUBE; grav = 1; frame = 0;
    ground = true; dead = false; won = false; coins = 0;
    int c = 0;
    for (int i = 0; i < MAX_OBJS; i++) { used[i] = 0; cid[i] = 0; }
    for (int i = 0; i < L->nobjs; i++) if (L->objs[i].t == T_COIN) cid[i] = (uint8_t)(c++ % 3);
  }

  int firstAt(float xmin) const {
    int lo = 0, hi = L->nobjs;
    while (lo < hi) { int m = (lo + hi) / 2; if (L->objs[m].x < xmin) lo = m + 1; else hi = m; }
    return lo;
  }

  void setMode(int m) {
    if (m == mode) return;
    float cy = y + ph() * 0.5f;
    mode = m;
    y = cy - ph() * 0.5f;
    rot = 0;
    if (vy > G::SHIP_VMAX) vy = G::SHIP_VMAX;
    if (vy < -G::SHIP_VMAX) vy = -G::SHIP_VMAX;
  }

  static bool hit(float ax0, float ay0, float ax1, float ay1, float bx0, float by0, float bx1, float by1) {
    return ax0 < bx1 && ax1 > bx0 && ay0 < by1 && ay1 > by0;
  }

  // Ein Schritt = 1/60 s. held = Taste gedrückt, pressed = Taste in diesem Frame neu gedrückt.
  void step(bool held, bool pressed) {
    if (dead || won) return;
    frame++;
    const float V = L->speed;
    const float sg = grav > 0 ? 1.0f : -1.0f;
    if (pressed) buf = 0.13f; else if (buf > 0) buf -= G::DT;
    const float py = y;

    x += V * G::DT;

    if (mode == M_CUBE) {
      bool wasGround = ground; ground = false;
      if (wasGround && (held || buf > 0)) { vy = G::JUMP_V * sg; buf = 0; }
      vy -= G::GRAV * sg * G::DT;
      if (vy > G::VMAX) vy = G::VMAX; if (vy < -G::VMAX) vy = -G::VMAX;
      rot += 400.0f * G::DT * sg;     // dreht sich in der Luft
    } else if (mode == M_SHIP) {
      float a = (held ? G::SHIP_UP : -G::SHIP_DN) * sg;
      vy += a * G::DT;
      if (vy > G::SHIP_VMAX) vy = G::SHIP_VMAX; if (vy < -G::SHIP_VMAX) vy = -G::SHIP_VMAX;
      rot = -vy * 4.0f; if (rot > 40) rot = 40; if (rot < -40) rot = -40;
    } else { // Wave: gedrückt = schräg hoch, sonst schräg runter
      vy = (held ? V : -V) * sg;
      rot = held ? 45.0f * sg : -45.0f * sg;
    }
    y += vy * G::DT;

    // Boden und Decke
    if (y < 0) { y = 0; if (vy < 0) vy = 0; if (mode == M_CUBE && grav > 0) ground = true; }
    if (y + ph() > G::HEIGHT) { y = G::HEIGHT - ph(); if (vy > 0) vy = 0; if (mode == M_CUBE && grav < 0) ground = true; }

    const float PW = pw();
    int j0 = firstAt(x - L->maxW - 1.0f);
    // ---- Blöcke ----
    for (int j = j0; j < L->nobjs; j++) {
      const LObj& o = L->objs[j];
      if (o.x > x + 2.0f) break;
      if (o.t != T_BLOCK && o.t != T_SLAB) continue;
      const float bx = o.x, by = o.y, bw = o.w, bh = o.h;
      if (x + PW <= bx + 1e-4f || x >= bx + bw - 1e-4f) continue;
      float PH = ph();
      if (y + PH <= by + 1e-4f || y >= by + bh - 1e-4f) continue;
      bool above = py >= by + bh - 1e-3f;
      bool below = py + PH <= by + 1e-3f;
      if (above) {
        y = by + bh; if (vy < 0) vy = 0;
        if (mode == M_CUBE && grav > 0) ground = true;
      } else if (below) {
        y = by - PH; if (vy > 0) vy = 0;
        if (mode == M_CUBE && grav < 0) ground = true;
      } else if (mode != M_CUBE && (by + bh - y) < G::STEP) {
        y = by + bh; if (vy < 0) vy = 0;
      } else { dead = true; return; }
    }

    // ---- Gefahren und Auslöser ----
    const float s = mode == M_WAVE ? 0.0f : 0.08f;
    const float hx0 = x + s, hx1 = x + PW - s, hy0 = y + s, hy1 = y + ph() - s;
    for (int j = j0; j < L->nobjs; j++) {
      const LObj& o = L->objs[j];
      if (o.x > x + 2.0f) break;
      switch (o.t) {
        case T_SPIKE:
          if (hit(hx0, hy0, hx1, hy1, o.x + 0.3f, o.y, o.x + 0.7f, o.y + 0.45f)) { dead = true; return; }
          break;
        case T_CSPIKE:
          if (hit(hx0, hy0, hx1, hy1, o.x + 0.3f, o.y + 0.55f, o.x + 0.7f, o.y + 1.0f)) { dead = true; return; }
          break;
        case T_SAW: {
          float r = o.w * 0.5f * 0.82f, cx = o.x + o.w * 0.5f, cy = o.y + o.w * 0.5f;
          float qx = cx < hx0 ? hx0 : (cx > hx1 ? hx1 : cx), qy = cy < hy0 ? hy0 : (cy > hy1 ? hy1 : cy);
          float dx = qx - cx, dy = qy - cy;
          if (dx * dx + dy * dy < r * r) { dead = true; return; }
          break; }
        case T_ORB: case T_BORB:
          if (buf > 0 && !used[j] && hit(x, y, x + PW, y + ph(), o.x - 0.1f, o.y - 0.1f, o.x + 1.1f, o.y + 1.1f)) {
            used[j] = 1; buf = 0; ground = false;
            if (o.t == T_ORB) vy = G::ORB_V * sg;
            else { grav = -grav; vy = grav > 0 ? -4.0f : 4.0f; }
          }
          break;
        case T_PAD:
          if (!used[j] && hit(x, y, x + PW, y + ph(), o.x, o.y, o.x + 1.0f, o.y + 0.3f)) {
            used[j] = 1; vy = G::PAD_V * sg; ground = false;
          }
          break;
        case T_COIN:
          if (hit(x, y, x + PW, y + ph(), o.x + 0.1f, o.y + 0.1f, o.x + 0.9f, o.y + 0.9f)) coins |= (1 << cid[j]);
          break;
        case T_SHIP: case T_CUBE: case T_WAVE:
          if (hit(x, y, x + PW, y + ph(), o.x, o.y, o.x + 1.0f, o.y + 3.0f))
            setMode(o.t == T_SHIP ? M_SHIP : (o.t == T_CUBE ? M_CUBE : M_WAVE));
          break;
        case T_GUP: case T_GDOWN:
          if (hit(x, y, x + PW, y + ph(), o.x, o.y, o.x + 1.0f, o.y + 3.0f)) {
            int g = o.t == T_GUP ? -1 : 1;
            if (g != grav) { grav = g; vy *= 0.5f; ground = false; }
          }
          break;
        default: break;
      }
    }

    if (mode == M_CUBE && ground) rot = floorf(rot / 90.0f + 0.5f) * 90.0f;   // landet gerade
    if (x >= L->length) won = true;
  }
};
