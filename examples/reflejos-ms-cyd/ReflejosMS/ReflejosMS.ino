/*
  Reflejos MS · AB MakerIDE / MS Robotika
  Juego original de reflejos para la CYD ESP32-2432S028R (ILI9341 240x320,
  tactil resistivo XPT2046). Se enciende un chip: tocalo antes de que se acabe
  el tiempo. Cada acierto acelera el ritmo. El LED RGB trasero marca acierto
  (verde) o fallo (rojo).
  No usa Wi-Fi, microSD, altavoz ni archivos externos.
  Librerias: Adafruit GFX, Adafruit ILI9341, XPT2046_Touchscreen (Paul Stoffregen).
  Tras cada reinicio: tocar las tres cruces para calibrar el tactil (en RAM).
  Referencia de pines: github.com/witnessmenow/ESP32-Cheap-Yellow-Display/PINS.md
  Licencia MIT, como el resto de AB MakerIDE.
*/
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

SPIClass lcdBus(HSPI), touchBus(VSPI);
Adafruit_ILI9341 lcd(&lcdBus, 2, 15, -1);
XPT2046_Touchscreen touch(33, 36);

// LED RGB de la CYD: activo a nivel bajo.
constexpr int LED_R = 4, LED_G = 16, LED_B = 17;

constexpr uint16_t BG = 0x0843, PCB = 0x0A65, TRACE = 0x1C8A, CHIP = 0x2124;
constexpr uint16_t CHIP_EDGE = 0x52AA, PIN = 0xC618, ON = 0x07FF, HIT = 0x07E0, MISS = 0xF800;
constexpr int COLS = 3, ROWS = 4, GX = 12, GY = 84, PW = 64, PH = 46, GAPX = 12, GAPY = 12;
constexpr int BAR_X = 12, BAR_Y = 70, BAR_W = 216, BAR_H = 6;

enum State { READY, PLAY, OVER };
State state = READY;
int target = -1, lives = 3;
uint32_t score = 0, best = 0, streak = 0, windowMs = 1300, deadline = 0, ledOff = 0;
uint32_t lastTouch = 0;
bool touching = false;
int lastBar = -1;
float ax = 0, bx = 0, cx = 0, ay = 0, by = 0, cy = 0;

void text(int x, int y, const char* value, uint16_t color = ILI9341_WHITE, uint8_t size = 1, uint16_t bg = BG) {
  lcd.setCursor(x, y);
  lcd.setTextColor(color, bg);
  lcd.setTextSize(size);
  lcd.print(value);
}

void led(bool r, bool g, bool b, uint32_t ms) {
  digitalWrite(LED_R, r ? LOW : HIGH);
  digitalWrite(LED_G, g ? LOW : HIGH);
  digitalWrite(LED_B, b ? LOW : HIGH);
  ledOff = ms ? millis() + ms : 0;
}

void padRect(int i, int& x, int& y) {
  x = GX + (i % COLS) * (PW + GAPX);
  y = GY + (i / COLS) * (PH + GAPY);
}

// Dibuja un "chip" con sus patillas; el color de la cara indica su estado.
void drawPad(int i, uint16_t face) {
  int x, y;
  padRect(i, x, y);
  for (int k = 0; k < 4; k++) {
    lcd.fillRect(x + 10 + k * 13, y - 4, 5, 4, PIN);
    lcd.fillRect(x + 10 + k * 13, y + PH, 5, 4, PIN);
  }
  lcd.fillRoundRect(x, y, PW, PH, 5, face);
  lcd.drawRoundRect(x, y, PW, PH, 5, CHIP_EDGE);
  lcd.fillCircle(x + 7, y + 7, 2, face == CHIP ? CHIP_EDGE : BG);
}

void drawBoard() {
  lcd.fillScreen(BG);
  text(12, 8, "MS ROBOTIKA", ILI9341_CYAN, 2);
  text(12, 28, "REFLEJOS  -  CYD 2.8", PIN);
  lcd.fillRect(0, GY - 10, 240, 320 - GY + 10, PCB);
  for (int r = 0; r < ROWS; r++) lcd.drawFastHLine(0, GY + r * (PH + GAPY) + PH / 2, 240, TRACE);
  for (int c = 0; c < COLS; c++) lcd.drawFastVLine(GX + c * (PW + GAPX) + PW / 2, GY - 10, 320, TRACE);
  for (int i = 0; i < COLS * ROWS; i++) drawPad(i, CHIP);
  lastBar = -1;
}

void drawStats() {
  char line[32];
  snprintf(line, sizeof(line), "PUNTOS %-6lu", (unsigned long)score);
  text(12, 46, line, ILI9341_WHITE, 1);
  snprintf(line, sizeof(line), "RACHA %-3lu", (unsigned long)streak);
  text(110, 46, line, ILI9341_YELLOW, 1);
  for (int k = 0; k < 3; k++) lcd.fillRoundRect(186 + k * 15, 44, 11, 11, 3, k < lives ? MISS : 0x3186);
  snprintf(line, sizeof(line), "RECORD %-6lu", (unsigned long)best);
  text(12, 58, line, PIN, 1);
}

void drawBar(uint32_t now) {
  int w = 0;
  if (state == PLAY && now < deadline) w = (int)((uint64_t)(deadline - now) * BAR_W / windowMs);
  if (w == lastBar) return;
  uint16_t color = w > BAR_W / 2 ? HIT : (w > BAR_W / 4 ? ILI9341_YELLOW : MISS);
  lcd.fillRect(BAR_X, BAR_Y, w, BAR_H, color);
  lcd.fillRect(BAR_X + w, BAR_Y, BAR_W - w, BAR_H, 0x18C3);
  lastBar = w;
}

void banner(const char* top, const char* bottom) {
  lcd.fillRoundRect(20, 150, 200, 64, 8, 0x18C6);
  lcd.drawRoundRect(20, 150, 200, 64, 8, ILI9341_CYAN);
  int tw = strlen(top) * 12;
  text(120 - tw / 2, 162, top, ILI9341_YELLOW, 2, 0x18C6);
  int bw = strlen(bottom) * 6;
  text(120 - bw / 2, 194, bottom, ILI9341_WHITE, 1, 0x18C6);
}

void nextTarget(uint32_t now) {
  int previous = target;
  do target = random(COLS * ROWS); while (target == previous);
  drawPad(target, ON);
  deadline = now + windowMs;
}

void startGame(uint32_t now) {
  score = streak = 0;
  lives = 3;
  windowMs = 1300;
  target = -1;
  state = PLAY;
  drawBoard();
  drawStats();
  nextTarget(now);
  Serial.println("MSR_REFLEJOS_START");
}

void loseLife(uint32_t now, int wrongPad) {
  lives--;
  streak = 0;
  led(true, false, false, 220);
  if (wrongPad >= 0 && wrongPad != target) drawPad(wrongPad, MISS);
  drawPad(target, CHIP);
  if (wrongPad >= 0 && wrongPad != target) {
    delay(120);
    drawPad(wrongPad, CHIP);
  }
  drawStats();
  if (lives <= 0) {
    state = OVER;
    if (score > best) best = score;
    drawStats();
    drawBar(now);
    banner("FIN", "Toca para jugar otra vez");
    Serial.printf("MSR_REFLEJOS_OVER score=%lu best=%lu\n", (unsigned long)score, (unsigned long)best);
    return;
  }
  nextTarget(now);
}

void hit(uint32_t now) {
  streak++;
  score += 10 + min<uint32_t>(streak, 20) * 2;
  if (windowMs > 380) windowMs -= 30;
  led(false, true, false, 120);
  drawPad(target, HIT);
  delay(60);
  drawPad(target, CHIP);
  drawStats();
  nextTarget(now);
}

int padAt(int x, int y) {
  for (int i = 0; i < COLS * ROWS; i++) {
    int px, py;
    padRect(i, px, py);
    if (x >= px - 4 && x < px + PW + 4 && y >= py - 4 && y < py + PH + 4) return i;
  }
  return -1;
}

bool sampleRaw(int& x, int& y) {
  if (!touch.tirqTouched() || !touch.touched()) return false;
  TS_Point p = touch.getPoint();
  if (p.z < 250) return false;
  x = p.x;
  y = p.y;
  return true;
}

void calibrate() {
  const int sx[3] = {20, 220, 20}, sy[3] = {20, 20, 300};
  int rx[3], ry[3];
  while (true) {
    for (int point = 0; point < 3; point++) {
      lcd.fillScreen(BG);
      text(42, 120, "CALIBRAR TACTIL", ILI9341_CYAN, 2);
      text(42, 148, "Toca el centro de la cruz");
      lcd.drawFastHLine(sx[point] - 10, sy[point], 21, ILI9341_YELLOW);
      lcd.drawFastVLine(sx[point], sy[point] - 10, 21, ILI9341_YELLOW);
      lcd.drawCircle(sx[point], sy[point], 8, ILI9341_YELLOW);
      int x, y;
      while (sampleRaw(x, y)) delay(10);
      int32_t sumx = 0, sumy = 0;
      int samples = 0;
      while (samples < 12) {
        if (sampleRaw(x, y)) {
          sumx += x;
          sumy += y;
          samples++;
        } else {
          samples = 0;
          sumx = sumy = 0;
        }
        delay(12);
      }
      rx[point] = sumx / samples;
      ry[point] = sumy / samples;
      while (sampleRaw(x, y)) delay(10);
      delay(150);
    }
    float ux = rx[1] - rx[0], uy = ry[1] - ry[0], vx = rx[2] - rx[0], vy = ry[2] - ry[0];
    float det = ux * vy - vx * uy;
    if (abs(det) < 10000) {
      text(30, 190, "Repite las tres cruces", ILI9341_RED);
      delay(1000);
      continue;
    }
    ax = ((sx[1] - sx[0]) * vy - (sx[2] - sx[0]) * uy) / det;
    bx = (ux * (sx[2] - sx[0]) - vx * (sx[1] - sx[0])) / det;
    cx = sx[0] - ax * rx[0] - bx * ry[0];
    ay = ((sy[1] - sy[0]) * vy - (sy[2] - sy[0]) * uy) / det;
    by = (ux * (sy[2] - sy[0]) - vx * (sy[1] - sy[0])) / det;
    cy = sy[0] - ay * rx[0] - by * ry[0];
    Serial.println("MSR_CYD_CALIBRATION_OK RAM_ONLY");
    return;
  }
}

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("MSR_REFLEJOS_BOOT profile=ESP32-2432S028R display=ILI9341 touch=XPT2046");
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  led(false, false, false, 0);
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);  // microSD deseleccionada; no se monta.
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);  // Retroiluminacion.
  lcdBus.begin(14, 12, 13, 15);
  lcd.begin(20000000);
  lcd.setRotation(0);
  touchBus.begin(25, 39, 32, 33);
  touch.begin(touchBus);
  touch.setRotation(0);
  randomSeed(esp_random());
  calibrate();
  drawBoard();
  drawStats();
  banner("REFLEJOS", "Toca un chip para empezar");
  led(false, false, true, 400);
}

void loop() {
  uint32_t now = millis();
  int rawx, rawy;
  bool down = sampleRaw(rawx, rawy);
  if (down && !touching && now - lastTouch > 90) {
    lastTouch = now;
    int x = lroundf(ax * rawx + bx * rawy + cx), y = lroundf(ay * rawx + by * rawy + cy);
    if (state != PLAY) {
      startGame(now);
    } else {
      int pad = padAt(x, y);
      if (pad == target) hit(now);
      else if (pad >= 0) loseLife(now, pad);
    }
  }
  touching = down;
  if (state == PLAY && now >= deadline) loseLife(now, -1);
  drawBar(now);
  if (ledOff && now >= ledOff) led(false, false, false, 0);
  delay(5);
}
