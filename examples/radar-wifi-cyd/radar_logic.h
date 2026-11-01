#pragma once
#include <stdint.h>
#include <string.h>
#include <math.h>

namespace Radar {
constexpr int MAX_NETWORKS = 48;
constexpr int PAGE_SIZE = 5;
struct Network {
  char ssid[34];
  uint8_t bssid[6];
  int16_t rssi;
  uint8_t channel;
  uint8_t auth;
};

inline int clamp(int x, int lo, int hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline int radius(int rssi) { return 12 + (-30 - clamp(rssi, -100, -30)) * 86 / 70; }
inline int quality(int rssi) { return clamp(2 * (rssi + 100), 0, 100); }
inline int pages(int count) { return count > 0 ? (count + PAGE_SIZE - 1) / PAGE_SIZE : 1; }
inline uint16_t bearing(const uint8_t *mac) {
  uint32_t h = 2166136261u;
  for (int i = 0; i < 6; ++i) { h ^= mac[i]; h *= 16777619u; }
  return h % 360;
}
inline bool same(const uint8_t *a, const uint8_t *b) { return memcmp(a, b, 6) == 0; }

// Sorted, bounded insertion; an AP is identified by BSSID, never SSID.
inline void insert(Network *records, int &count, const Network &item) {
  for (int i = 0; i < count; ++i) {
    if (!same(records[i].bssid, item.bssid)) continue;
    if (item.rssi <= records[i].rssi) return;
    for (int j = i; j + 1 < count; ++j) records[j] = records[j + 1];
    --count;
    break;
  }
  int pos = 0;
  while (pos < count && records[pos].rssi >= item.rssi) ++pos;
  if (pos >= MAX_NETWORKS) return;
  if (count < MAX_NETWORKS) ++count;
  for (int j = count - 1; j > pos; --j) records[j] = records[j - 1];
  records[pos] = item;
}

struct Calibration { float ax, bx, cx, ay, by, cy; };
inline bool solveCalibration(const int *rx, const int *ry, Calibration &c) {
  const float ux = rx[1] - rx[0], uy = ry[1] - ry[0];
  const float vx = rx[2] - rx[0], vy = ry[2] - ry[0];
  const float det = ux * vy - vx * uy;
  if (fabsf(det) < 10000) return false;
  // Targets (20,20), (220,20), (20,300), same as local reference.
  c.ax = 200 * vy / det; c.bx = -200 * vx / det;
  c.ay = -280 * uy / det; c.by = 280 * ux / det;
  c.cx = 20 - c.ax * rx[0] - c.bx * ry[0];
  c.cy = 20 - c.ay * rx[0] - c.by * ry[0];
  return true;
}
inline bool validCalibration(const Calibration &c) {
  return isfinite(c.ax) && isfinite(c.bx) && isfinite(c.cx) &&
         isfinite(c.ay) && isfinite(c.by) && isfinite(c.cy) &&
         fabsf(c.ax) < 2 && fabsf(c.bx) < 2 && fabsf(c.ay) < 2 && fabsf(c.by) < 2 &&
         fabsf(c.cx) < 2000 && fabsf(c.cy) < 2000 &&
         fabsf(c.ax * c.by - c.bx * c.ay) > 0.0001f;
}

// Built-in font: preserve ASCII, transliterate common Latin UTF-8, sanitize
// control sequences. Never let an SSID inject new lines into the layout.
inline void displayName(const char *src, char *dst, unsigned capacity) {
  unsigned o = 0;
  for (unsigned i = 0; src[i] && o + 1 < capacity; ++i) {
    uint8_t b = (uint8_t)src[i];
    if (b < 128) { dst[o++] = b >= 32 && b <= 126 ? (char)b : '?'; continue; }
    char mapped = '?';
    uint8_t next = (uint8_t)src[i + 1];
    if (b == 0xC3 && next >= 0x80 && next <= 0xBF) {
      if (next <= 0x85) mapped = 'A';
      else if (next == 0x87) mapped = 'C';
      else if (next <= 0x8B && next >= 0x88) mapped = 'E';
      else if (next <= 0x8F && next >= 0x8C) mapped = 'I';
      else if (next == 0x91) mapped = 'N';
      else if (next <= 0x96 && next >= 0x92) mapped = 'O';
      else if (next <= 0x9C && next >= 0x99) mapped = 'U';
      else if (next <= 0xA5 && next >= 0xA0) mapped = 'a';
      else if (next == 0xA7) mapped = 'c';
      else if (next <= 0xAB && next >= 0xA8) mapped = 'e';
      else if (next <= 0xAF && next >= 0xAC) mapped = 'i';
      else if (next == 0xB1) mapped = 'n';
      else if (next <= 0xB6 && next >= 0xB2) mapped = 'o';
      else if (next <= 0xBC && next >= 0xB9) mapped = 'u';
    }
    // Consume UTF-8 continuation bytes, keeping one placeholder per glyph.
    while (((uint8_t)src[i + 1] & 0xC0) == 0x80) ++i;
    dst[o++] = mapped;
  }
  if (capacity) dst[o] = 0;
}
}  // namespace Radar
