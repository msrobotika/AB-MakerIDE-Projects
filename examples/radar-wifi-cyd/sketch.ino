/*
 * RADAR Wi-Fi / MS Robotika / CYD ESP32-2432S028R
 * Physical ILI9341 + XPT2046, portrait. No PC interface, no SD.
 * Pin source and local reference: MAKERIDE_HARDWARE_REFERENCE.txt.
 * RSSI controls radius; angle is decorative, not a measured bearing.
 */
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include "radar_logic.h"
#include "radar_selftest.h"

SPIClass lcdBus(HSPI), touchBus(VSPI);
Adafruit_ILI9341 lcd(&lcdBus, 2, 15, -1);
XPT2046_Touchscreen touch(33, 36);
// Banded framebuffer: 7 KB instead of a 100 KB full radar framebuffer.
GFXcanvas16 strip(224, 16);
constexpr uint16_t BG = 0x0020, PANEL = 0x0862, GREEN = 0x07E8;
constexpr uint16_t DIM = 0x02A4, WHITE = 0xDFDD, MUTED = 0x6C4E;
constexpr uint16_t AMBER = 0xFEA0, GRID = 0x0162;
constexpr int RADAR_X = 8, RADAR_Y = 38, RADAR_C = 112;
constexpr uint32_t SCAN_INTERVAL = 8000;
enum View { RADAR_VIEW, LIST_VIEW, DETAIL_VIEW };
View view = RADAR_VIEW, returnView = LIST_VIEW;
Radar::Network networks[Radar::MAX_NETWORKS] = {};
Radar::Calibration cal = {};
int networkCount = 0, detectedCount = 0, page = 0;
uint8_t selectedBssid[6] = {};
bool selected = false, scanning = false, scanError = false;
bool haveResults = false, dirty = true, touchDown = false, wifiReady = false;
uint32_t lastScanEnd = 0, scanStarted = 0, lastFrame = 0, lastTap = 0;
uint32_t lastStatusSecond = UINT32_MAX, frameCount = 0, lastHealth = 0;
bool calibrated = false, calibrating = false, calWaitRelease = true;
int calPoint = 0, calSamples = 0, calRx[3], calRy[3];
int32_t calSumX = 0, calSumY = 0;
uint32_t lastCalSample = 0, bootPressedAt = 0;
bool bootWasDown = false, bootHandled = false;

void label(int x, int y, const char *s, uint16_t color = WHITE, uint8_t size = 1) {
  lcd.setCursor(x, y); lcd.setTextColor(color); lcd.setTextSize(size); lcd.print(s);
}
void button(int x, int y, int w, int h, const char *s, bool active = false) {
  lcd.fillRoundRect(x, y, w, h, 5, active ? DIM : PANEL);
  lcd.drawRoundRect(x, y, w, h, 5, active ? GREEN : GRID);
  label(x + (w - strlen(s) * 6) / 2, y + (h - 8) / 2, s, active ? WHITE : GREEN);
}
void shortName(const char *src, char *out, int maxChars) {
  char safe[34]; Radar::displayName(src, safe, sizeof(safe));
  if (!safe[0]) strcpy(safe, "(Red oculta)");
  snprintf(out, maxChars + 1, "%s", safe);
  if ((int)strlen(safe) > maxChars && maxChars >= 3) strcpy(out + maxChars - 3, "...");
}
const char *security(uint8_t auth) {
  switch (auth) {
    case WIFI_AUTH_OPEN: return "Abierta";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2 Enterprise";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
    default: return "Protegida";
  }
}
int selectedIndex() {
  if (selected) for (int i = 0; i < networkCount; ++i)
    if (Radar::same(selectedBssid, networks[i].bssid)) return i;
  return -1;
}
void openDetail(int index) {
  if (index < 0 || index >= networkCount) return;
  memcpy(selectedBssid, networks[index].bssid, 6); selected = true;
  returnView = view; view = DETAIL_VIEW; dirty = true;
}
void pointPosition(int index, int &x, int &y) {
  float a = Radar::bearing(networks[index].bssid) * DEG_TO_RAD;
  int r = Radar::radius(networks[index].rssi);
  x = RADAR_C + lroundf(sinf(a) * r); y = RADAR_C - lroundf(cosf(a) * r);
}
void drawHeader() {
  lcd.fillRect(0, 0, 240, 34, PANEL);
  label(10, 8, "WIFI RADAR", GREEN, 2);
  label(187, 7, "2.4 GHz", MUTED);
  label(187, 20, "CYD / R", GREEN);
  lcd.drawFastHLine(0, 33, 240, DIM);
}
void drawNav() {
  button(4, 285, 74, 31, "RADAR", view == RADAR_VIEW);
  button(83, 285, 74, 31, "REDES", view == LIST_VIEW);
  button(162, 285, 74, 31, scanning ? "BUSCANDO" : "BUSCAR");
}
void drawStatus(uint32_t now) {
  lcd.fillRect(0, 264, 240, 18, BG);
  char line[40];
  if (!wifiReady) snprintf(line, sizeof(line), "Error al iniciar Wi-Fi");
  else if (scanning) snprintf(line, sizeof(line), "%d redes  |  Escaneando...", networkCount);
  else if (scanError) snprintf(line, sizeof(line), "Error scan | lectura anterior");
  else if (detectedCount > networkCount) snprintf(line, sizeof(line), "%d/%d redes | mas fuertes", networkCount, detectedCount);
  else snprintf(line, sizeof(line), "%d redes | actualizada hace %lus", networkCount, (unsigned long)((now - lastScanEnd) / 1000));
  label(8, 269, line, scanError ? AMBER : MUTED);
}
void drawRadar(uint32_t now) {
  const int sweep = (now / 22) % 360;
  for (int row = 0; row < 224; row += 16) {
    strip.fillScreen(BG);
    // Graduated phosphor tail, with a brighter leading edge.
    for (int tail = 30; tail >= 0; tail -= 2) {
      float a = (sweep - tail) * DEG_TO_RAD;
      uint16_t color = tail > 20 ? 0x0141 : (tail > 10 ? 0x0243 : 0x0385);
      strip.drawLine(RADAR_C, RADAR_C - row,
                     RADAR_C + lroundf(sinf(a) * 103), RADAR_C - lroundf(cosf(a) * 103) - row, color);
    }
    for (int radius : {26, 50, 76, 103}) strip.drawCircle(RADAR_C, RADAR_C - row, radius, GRID);
    strip.drawFastHLine(9, RADAR_C - row, 207, GRID);
    strip.drawFastVLine(RADAR_C, 9 - row, 207, GRID);
    for (int degree = 0; degree < 360; degree += 15) {
      float a = degree * DEG_TO_RAD;
      strip.drawLine(RADAR_C + lroundf(sinf(a) * 100), RADAR_C - lroundf(cosf(a) * 100) - row,
                     RADAR_C + lroundf(sinf(a) * 103), RADAR_C - lroundf(cosf(a) * 103) - row, DIM);
    }
    strip.setTextSize(1); strip.setTextColor(MUTED);
    strip.setCursor(116, 12 - row); strip.print("-100");
    strip.setCursor(116, 39 - row); strip.print("-80");
    strip.setCursor(116, 65 - row); strip.print("-60");
    strip.setCursor(116, 90 - row); strip.print("-40");
    // Draw weaker APs first so the strongest remain legible in a collision.
    for (int i = networkCount - 1; i >= 0; --i) {
      int x, y; pointPosition(i, x, y);
      int behind = (sweep - Radar::bearing(networks[i].bssid) + 360) % 360;
      uint16_t color = behind < 50 ? WHITE : (behind < 160 ? GREEN : DIM);
      strip.fillCircle(x, y - row, i < 8 ? 3 : 2, color);
      if (behind < 30) strip.drawCircle(x, y - row, 5, GREEN);
      if (i < 12) {
        strip.setTextColor(color); strip.setCursor(x > 194 ? x - 15 : x + 5, y - 3 - row); strip.print(i + 1);
      }
    }
    strip.fillCircle(RADAR_C, RADAR_C - row, 3, WHITE);
    strip.drawCircle(RADAR_C, RADAR_C - row, 6, GREEN);
    strip.setTextColor(MUTED); strip.setCursor(1, 216 - row); strip.print("SENAL, NO METROS / ANGULO DECORATIVO");
    if (networkCount == 0) {
      strip.fillRect(23, 154 - row, 178, 18, PANEL);
      strip.setCursor(31, 160 - row); strip.setTextColor(GREEN);
      strip.print(!haveResults ? "Buscando redes de 2.4 GHz" : "Sin redes en este barrido");
    }
    lcd.drawRGBBitmap(RADAR_X, RADAR_Y + row, strip.getBuffer(), 224, 16);
  }
  ++frameCount;
}
void signalBars(int x, int y, int rssi) {
  int bars = (Radar::quality(rssi) + 24) / 25;
  for (int i = 0; i < 4; ++i) lcd.fillRect(x + i * 5, y + 15 - (i + 1) * 4, 3, (i + 1) * 4, i < bars ? GREEN : GRID);
}
void drawList() {
  char line[40];
  label(10, 39, "REDES DISPONIBLES", WHITE);
  label(10, 52, "Por intensidad / toca para ver", MUTED);
  for (int row = 0; row < Radar::PAGE_SIZE; ++row) {
    int i = page * Radar::PAGE_SIZE + row, y = 68 + row * 32;
    if (i >= networkCount) break;
    lcd.fillRoundRect(6, y, 228, 29, 3, PANEL);
    char name[28]; shortName(networks[i].ssid, name, 27);
    label(12, y + 4, name, WHITE);
    snprintf(line, sizeof(line), "%ddBm  %s  CH%d", networks[i].rssi, security(networks[i].auth), networks[i].channel);
    label(12, y + 17, line, MUTED);
    signalBars(209, y + 12, networks[i].rssi);
  }
  if (!networkCount) {
    label(20, 112, scanning ? "Buscando redes..." : "Ninguna red detectada", GREEN);
    label(20, 132, "Solo redes de 2.4 GHz", MUTED);
  }
  button(6, 232, 59, 28, "< ANT"); button(175, 232, 59, 28, "SIG >");
  snprintf(line, sizeof(line), "%d / %d", page + 1, Radar::pages(networkCount));
  label(96, 242, line, GREEN);
}
void drawDetail() {
  button(8, 39, 78, 27, "< VOLVER"); label(107, 48, "DETALLE RED", GREEN);
  int index = selectedIndex();
  if (index < 0) {
    label(12, 90, "Red fuera del ultimo barrido", AMBER);
    label(12, 110, "Vuelve a Redes o pulsa Buscar", MUTED); return;
  }
  const Radar::Network &n = networks[index];
  char name[34]; shortName(n.ssid, name, 33);
  // Full 32-byte SSID on two lines; keep surrounding fields fixed.
  char first[19] = {}; strncpy(first, name, 18);
  label(12, 79, first, WHITE, 2);
  if (strlen(name) > 18) label(12, 101, name + 18, WHITE, 2);
  char line[40];
  snprintf(line, sizeof(line), "%d dBm", n.rssi); label(12, 133, line, GREEN, 3);
  signalBars(207, 138, n.rssi);
  snprintf(line, sizeof(line), "Intensidad relativa: %d%%", Radar::quality(n.rssi)); label(12, 164, line, MUTED);
  label(12, 184, "Seguridad", MUTED); label(88, 184, security(n.auth), WHITE);
  snprintf(line, sizeof(line), "Canal %d  /  2.4 GHz", n.channel); label(12, 202, line, WHITE);
  snprintf(line, sizeof(line), "%02X:%02X:%02X:%02X:%02X:%02X", n.bssid[0], n.bssid[1], n.bssid[2], n.bssid[3], n.bssid[4], n.bssid[5]);
  label(12, 223, "BSSID", MUTED); label(52, 223, line, WHITE);
  label(12, 246, "Solo consulta / sin conexion", MUTED);
}
void render(uint32_t now) {
  if (calibrating) return;
  if (dirty) {
    lcd.fillScreen(BG); drawHeader(); drawNav();
    if (view == LIST_VIEW) drawList();
    if (view == DETAIL_VIEW) drawDetail();
    drawStatus(now); lastStatusSecond = now / 1000;
  }
  if (view == RADAR_VIEW && (dirty || now - lastFrame >= 100)) { drawRadar(now); lastFrame = now; }
  if (now / 1000 != lastStatusSecond) { drawStatus(now); lastStatusSecond = now / 1000; }
  dirty = false;
}

void acceptScan(int result, uint32_t now) {
  networkCount = 0; detectedCount = result;
  // Public WiFi accessors take uint8_t indices: use raw record access so APs
  // beyond index 255 cannot wrap and duplicate entries.
  for (int i = 0; i < result; ++i) {
    const wifi_ap_record_t *ap = static_cast<const wifi_ap_record_t *>(WiFiScanClass::getScanInfoByIndex(i));
    if (!ap) continue;
    Radar::Network n = {};
    memcpy(n.ssid, ap->ssid, 32); n.ssid[32] = 0;
    memcpy(n.bssid, ap->bssid, 6); n.rssi = ap->rssi;
    n.channel = ap->primary; n.auth = ap->authmode;
    Radar::insert(networks, networkCount, n);
  }
  page = Radar::clamp(page, 0, Radar::pages(networkCount) - 1);
  WiFi.scanDelete(); scanning = false; scanError = false; haveResults = true;
  lastScanEnd = now; dirty = true;
  Serial.printf("RADAR_SCAN_OK found=%d retained=%d strongest_dbm=%d heap=%lu\n", detectedCount, networkCount,
                networkCount ? networks[0].rssi : 0, (unsigned long)ESP.getFreeHeap());
}
void failScan(uint32_t now) {
  esp_wifi_scan_stop(); WiFi.scanDelete(); scanning = false; scanError = true;
  lastScanEnd = now; dirty = true;
  Serial.println("RADAR_SCAN_ERROR previous_results=retained retry=8s");
}
void startScan(uint32_t now) {
  if (!wifiReady || scanning) return;
  scanning = true; scanError = false; scanStarted = now; dirty = true;
  // Passive scan: listen to beacon frames, never associate with an AP.
  int result = WiFi.scanNetworks(true, true, true, 180);
  Serial.println("RADAR_SCAN_START passive=1 async=1 band=2.4GHz");
  if (result >= 0) acceptScan(result, now);
  else if (result != WIFI_SCAN_RUNNING) failScan(now);
}
void serviceScan(uint32_t now) {
  if (scanning) {
    int result = WiFi.scanComplete();
    if (result >= 0) acceptScan(result, now);
    else if (result == WIFI_SCAN_FAILED || now - scanStarted > 15000) failScan(now);
  } else if (now - lastScanEnd >= SCAN_INTERVAL) startScan(now);
}

bool sampleRaw(int &x, int &y) {
  if (!touch.tirqTouched() || !touch.touched()) return false;
  TS_Point p = touch.getPoint();
  if (p.z < 250) return false;
  x = p.x; y = p.y; return true;
}
void drawCalPoint() {
  const int sx[3] = {20,220,20}, sy[3] = {20,20,300};
  lcd.fillScreen(BG);
  label(30, 100, "AJUSTAR TACTIL", GREEN, 2);
  label(24, 132, "Toca y manten la cruz", WHITE);
  char step[30]; snprintf(step, sizeof(step), "Punto %d de 3", calPoint + 1); label(78, 154, step, GREEN);
  label(24, 179, "Despues: levanta el dedo", MUTED);
  label(24, 211, "Se guarda para el proximo uso", MUTED);
  lcd.drawFastHLine(sx[calPoint] - 10, sy[calPoint], 21, AMBER);
  lcd.drawFastVLine(sx[calPoint], sy[calPoint] - 10, 21, AMBER);
  lcd.drawCircle(sx[calPoint], sy[calPoint], 8, AMBER);
}
void beginCalibration() {
  calibrating = true; calPoint = 0; calSamples = 0;
  calSumX = calSumY = 0; calWaitRelease = true; drawCalPoint();
  Serial.println("RADAR_CAL_WAIT point=1 of=3 scan_continues=1");
}
void saveCalibration() {
  Preferences prefs;
  if (!prefs.begin("msr-radar", false)) { Serial.println("RADAR_CAL_SAVE_ERROR ram_only=1"); return; }
  size_t bytes = prefs.putBytes("affine-v1", &cal, sizeof(cal));
  prefs.end();
  Serial.println(bytes == sizeof(cal) ? "RADAR_CAL_SAVED" : "RADAR_CAL_SAVE_ERROR ram_only=1");
}
bool loadCalibration() {
  Preferences prefs;
  if (!prefs.begin("msr-radar", true)) return false;
  bool ok = prefs.getBytesLength("affine-v1") == sizeof(cal) && prefs.getBytes("affine-v1", &cal, sizeof(cal)) == sizeof(cal);
  prefs.end(); return ok && Radar::validCalibration(cal);
}
void serviceCalibration(bool down, int x, int y, uint32_t now) {
  if (calWaitRelease) {
    if (down) return;
    calWaitRelease = false;
    if (calSamples == 12) {
      calRx[calPoint] = calSumX / 12; calRy[calPoint] = calSumY / 12;
      ++calPoint; calSamples = 0; calSumX = calSumY = 0;
      if (calPoint == 3) {
        Radar::Calibration candidate;
        if (!Radar::solveCalibration(calRx, calRy, candidate) || !Radar::validCalibration(candidate)) {
          Serial.println("RADAR_CAL_RETRY degenerate=1"); beginCalibration(); return;
        }
        cal = candidate; saveCalibration(); calibrated = true; calibrating = false;
        dirty = true; touchDown = false;
        Serial.println("RADAR_READY touch_calibrated=1 display=ILI9341 view=radar"); return;
      }
      drawCalPoint(); Serial.printf("RADAR_CAL_WAIT point=%d of=3\n", calPoint + 1);
    }
  }
  if (!down) { calSamples = 0; calSumX = calSumY = 0; return; }
  if (now - lastCalSample < 15) return;
  lastCalSample = now;
  // A sliding finger must settle before collecting the three-point fit.
  if (calSamples && (abs(x - calSumX / calSamples) > 90 || abs(y - calSumY / calSamples) > 90)) {
    calSamples = 0; calSumX = calSumY = 0;
  }
  calSumX += x; calSumY += y; ++calSamples;
  if (calSamples == 12) {
    calWaitRelease = true;
    label(54, 237, "OK / levanta el dedo", GREEN);
  }
}
void handleTap(int x, int y, uint32_t now) {
  if (x < 0 || x >= 240 || y < 0 || y >= 320) return;
  if (y >= 285 && y < 316) {
    if (x >= 4 && x < 78) { view = RADAR_VIEW; dirty = true; }
    else if (x >= 83 && x < 157) { view = LIST_VIEW; dirty = true; }
    else if (x >= 162 && x < 236) startScan(now);
    return;
  }
  if (view == LIST_VIEW) {
    if (y >= 68 && y < 228) {
      int row = (y - 68) / 32;
      if ((y - 68) % 32 < 29) openDetail(page * Radar::PAGE_SIZE + row);
    } else if (y >= 232 && y < 260) {
      if (x < 65 && page > 0) { --page; dirty = true; }
      if (x >= 175 && page + 1 < Radar::pages(networkCount)) { ++page; dirty = true; }
    }
  } else if (view == DETAIL_VIEW) {
    if (x >= 8 && x < 86 && y >= 39 && y < 66) { view = returnView; dirty = true; }
  } else if (y >= RADAR_Y && y < RADAR_Y + 224) {
    int nearest = -1, best = 13 * 13;
    for (int i = 0; i < networkCount; ++i) {
      int px, py; pointPosition(i, px, py);
      int dx = x - (RADAR_X + px), dy = y - (RADAR_Y + py), d = dx * dx + dy * dy;
      if (d < best) { nearest = i; best = d; }
    }
    if (nearest >= 0) openDetail(nearest);
  }
}
void serviceBootButton(uint32_t now) {
  bool down = digitalRead(0) == LOW;
  if (down && !bootWasDown) { bootPressedAt = now; bootHandled = false; }
  if (down && !bootHandled && now - bootPressedAt >= 1200) {
    bootHandled = true; beginCalibration();
  }
  bootWasDown = down;
}
void setup() {
  Serial.begin(115200); delay(250);
  Serial.println("RADAR_START version=1.0.0 profile=ESP32-2432S028R declared=1");
  bool testsOk = radarSelfTest();
  Serial.printf("RADAR_SELFTEST %s suites=rssi,top48,bssid,pages,calibration,ssid\n", testsOk ? "PASS" : "FAIL");
  pinMode(5, OUTPUT); digitalWrite(5, HIGH);  // SD deselected, never mounted.
  pinMode(21, OUTPUT); digitalWrite(21, HIGH);
  pinMode(0, INPUT_PULLUP);
  pinMode(4, OUTPUT); digitalWrite(4, HIGH);
  pinMode(16, OUTPUT); digitalWrite(16, HIGH);
  pinMode(17, OUTPUT); digitalWrite(17, HIGH);
  lcdBus.begin(14, 12, 13, 15); lcd.begin(20000000); lcd.setRotation(0); lcd.setTextWrap(false);
  touchBus.begin(25, 39, 32, 33); bool touchOk = touch.begin(touchBus); touch.setRotation(0);
  lcd.fillScreen(BG);
  if (!testsOk || !strip.getBuffer()) {
    label(12, 120, !testsOk ? "Error de autoprueba" : "Memoria insuficiente", AMBER);
    Serial.println("RADAR_FATAL startup_inhibited=1"); while (true) delay(100);
  }
  Serial.printf("RADAR_DISPLAY_INIT width=%d height=%d touch_spi_init=%d buffer=7168\n", lcd.width(), lcd.height(), touchOk);
  wifiReady = WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(false);
  // Cancel any association retained by a previous application without erasing its credentials.
  if (wifiReady) { WiFi.disconnect(false, false); WiFi.setSleep(false); WiFi.setScanTimeout(12000); }
  calibrated = loadCalibration();
  if (!calibrated) beginCalibration();
  else Serial.println("RADAR_READY touch_calibrated=1 saved=1 display=ILI9341 view=radar");
  startScan(millis());
}
void loop() {
  uint32_t now = millis();
  serviceScan(now); serviceBootButton(now);
  int rawX = 0, rawY = 0; bool down = sampleRaw(rawX, rawY);
  if (calibrating) serviceCalibration(down, rawX, rawY, now);
  else if (calibrated && down && !touchDown && now - lastTap >= 160) {
    int x = lroundf(cal.ax * rawX + cal.bx * rawY + cal.cx);
    int y = lroundf(cal.ay * rawX + cal.by * rawY + cal.cy);
    lastTap = now; handleTap(x, y, now);
  }
  touchDown = down; render(now);
  if (now - lastHealth >= 10000) {
    lastHealth = now;
    Serial.printf("RADAR_HEALTH uptime_s=%lu heap=%lu min_heap=%lu radar_frames=%lu calibration=%d view=%d\n",
                  (unsigned long)(now / 1000), (unsigned long)ESP.getFreeHeap(),
                  (unsigned long)ESP.getMinFreeHeap(), (unsigned long)frameCount, calibrating, (int)view);
  }
  delay(5);
}
