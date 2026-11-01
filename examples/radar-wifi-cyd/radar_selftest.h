#pragma once
#include "radar_logic.h"

// Tests execute the production helpers on ESP32 before radio/display startup.
// A failure is reported and inhibits operation; this is not a physical UI test.
inline bool radarSelfTest() {
  using namespace Radar;
  if (radius(-30) != 12 || radius(-100) != 98 || radius(-150) != 98 || radius(10) != 12) return false;
  for (int r = -149; r <= 10; ++r) {
    if (radius(r) > radius(r - 1) || radius(r) < 12 || radius(r) > 98) return false;
    if (quality(r) < quality(r - 1) || quality(r) < 0 || quality(r) > 100) return false;
  }
  if (pages(0) != 1 || pages(5) != 1 || pages(6) != 2 || pages(48) != 10) return false;
  Radar::Network records[MAX_NETWORKS] = {};
  int count = 0;
  for (int i = 0; i < 80; ++i) {
    Radar::Network item = {}; item.bssid[5] = i; item.rssi = -110 + i;
    strcpy(item.ssid, "same-name");
    insert(records, count, item);
  }
  if (count != 48 || records[0].rssi != -31 || records[47].rssi != -78) return false;
  for (int i = 1; i < count; ++i) if (records[i - 1].rssi < records[i].rssi) return false;
  Radar::Network duplicate = records[47]; duplicate.rssi = -20;
  insert(records, count, duplicate);
  if (count != 48 || records[0].rssi != -20 || records[0].bssid[5] != duplicate.bssid[5]) return false;
  duplicate.rssi = -120; insert(records, count, duplicate);
  if (count != 48 || records[0].rssi != -20) return false;
  const uint8_t mac[6] = {0,1,2,3,4,5};
  if (bearing(mac) >= 360 || bearing(mac) != bearing(mac)) return false;
  // Rotated and inverted axes: catches swapped calibration coefficients.
  const int rx[3] = {3500,3500,300}, ry[3] = {300,3700,300};
  Calibration c;
  if (!solveCalibration(rx, ry, c) || !validCalibration(c)) return false;
  const int sx[3] = {20,220,20}, sy[3] = {20,20,300};
  for (int i = 0; i < 3; ++i) {
    if (fabsf(c.ax * rx[i] + c.bx * ry[i] + c.cx - sx[i]) > 0.1f ||
        fabsf(c.ay * rx[i] + c.by * ry[i] + c.cy - sy[i]) > 0.1f) return false;
  }
  const int bad[3] = {0,0,0};
  if (solveCalibration(bad, bad, c)) return false;
  char name[34]; displayName("Caf\xC3\xA9\n\xE2\x98\x83", name, sizeof(name));
  if (strcmp(name, "Cafe??")) return false;
  char tiny[3]; displayName("abcdef", tiny, sizeof(tiny));
  return strcmp(tiny, "ab") == 0;
}
