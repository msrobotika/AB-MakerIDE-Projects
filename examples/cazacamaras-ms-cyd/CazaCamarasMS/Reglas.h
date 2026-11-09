#pragma once
#include <stdint.h>
#include <stddef.h>

// Reglas sin hardware para poder probar los mismos límites en un ordenador.
enum Confidence : uint8_t { NINGUNA, BAJA, MEDIA, ALTA };
static const char* const CONF_NAME[] = {"SIN DATOS", "BAJA", "MEDIA", "ALTA"};

inline bool containsIgnoreCase(const char* text, const char* pattern) {
  for (; *text; ++text) {
    const char* a = text;
    const char* b = pattern;
    while (*a && *b) {
      char c = *a >= 'a' && *a <= 'z' ? *a - 'a' + 'A' : *a;
      char d = *b >= 'a' && *b <= 'z' ? *b - 'a' + 'A' : *b;
      if (c != d) break;
      ++a; ++b;
    }
    if (!*b) return true;
  }
  return false;
}

inline bool startsWithIgnoreCase(const char* text, const char* prefix) {
  for (; *prefix; ++text, ++prefix) {
    char c = *text >= 'a' && *text <= 'z' ? *text - 'a' + 'A' : *text;
    if (!*text || c != *prefix) return false;
  }
  return true;
}

// Redes que crean las cámaras al configurarlas. Se compara el principio del nombre:
// así «MOVISTAR_A9F0» no cuenta. Primero los largos para no llamar «IPC» a «IPC365».
// Los de marca son ALTA; los genéricos, MEDIA. Fuera «SMARTLIFE»: también lo usan enchufes.
static const char* const NET_PATTERNS[] = {
  "HDWIFICAM", "WIFICAM", "TAPO_CAM", "LOOKCAM", "IPC365", "REOLINK",
  "IPCAM", "EZVIZ", "DAHUA", "IMOU", "V380", "YI-", "HIK",
  "CAMERA", "CAM-", "IPC", "JXL"
};
static const Confidence NET_CONF[] = {
  ALTA, ALTA, ALTA, ALTA, ALTA, ALTA,
  ALTA, ALTA, ALTA, ALTA, ALTA, ALTA, ALTA,
  MEDIA, MEDIA, MEDIA, MEDIA
};
static_assert(sizeof(NET_PATTERNS) / sizeof(NET_PATTERNS[0]) == sizeof(NET_CONF) / sizeof(NET_CONF[0]), "patrones y confianzas");
inline int cameraPattern(const char* name) {
  for (size_t i = 0; i < sizeof(NET_PATTERNS) / sizeof(NET_PATTERNS[0]); ++i)
    if (startsWithIgnoreCase(name, NET_PATTERNS[i])) return (int)i;
  return -1;
}
inline Confidence bleHint(const char* name) {
  if (containsIgnoreCase(name, "IPC") || containsIgnoreCase(name, "V380")) return MEDIA;
  if (containsIgnoreCase(name, "cam") || containsIgnoreCase(name, "spy")) return BAJA;
  return NINGUNA;
}
inline bool ouiSuspect(Confidence c, bool video, uint32_t presenceMs) {
  return c == ALTA || video || (c == MEDIA && presenceMs > 30000);
}
inline bool uplinkData(uint8_t fc0, uint8_t fc1) {
  // Solo datos con carga, hacia el AP, sin reintento. No cuenta bajada, nulos ni WDS.
  return (fc0 & 0x0F) == 0x08 && !(fc0 & 0x40) && (fc1 & 0x03) == 0x01 && !(fc1 & 0x08);
}
inline bool payloadData(uint8_t fc0) { return (fc0 & 0x0F) == 0x08 && !(fc0 & 0x40); }

struct TrafficRun {
  uint32_t since = 0, last = 0;
  bool running = false, video = false;
  void reset() { running = video = false; }
  void sample(uint32_t now, uint32_t upBytes, uint32_t frames, uint32_t data, uint32_t dwellMs) {
    bool good = dwellMs >= 150 && dwellMs <= 1500 && frames &&
                (uint64_t)data * 100 > (uint64_t)frames * 70 &&
                (uint64_t)upBytes * 1000 > (uint64_t)20000 * dwellMs;
    if (!good) { reset(); return; }
    if (!running || now - last > 6000) { since = now; running = true; }
    last = now;
    video = now - since > 10000;
  }
  bool recent(uint32_t now) const { return video && running && now - last <= 6000; }
};

struct TrafficBucket {
  uint32_t second = 0, frames = 0, bytes = 0;
};
struct TrafficWindow {
  TrafficBucket bins[10] = {};
  void add(uint32_t now, uint16_t len) {
    uint32_t second = now / 1000;
    TrafficBucket& b = bins[second % 10];
    if (b.second != second) b = TrafficBucket{second, 0, 0};
    ++b.frames; b.bytes += len;
  }
  void totals(uint32_t now, uint32_t& frames, uint32_t& bytes) const {
    frames = bytes = 0;
    uint32_t second = now / 1000;
    for (const auto& b : bins) if (second - b.second < 10) { frames += b.frames; bytes += b.bytes; }
  }
};
