#pragma once
#include <stddef.h>
#include <stdint.h>

namespace nas {
constexpr uint64_t MAX_FILE = 128ULL * 1024 * 1024;
constexpr uint64_t RESERVE = 64ULL * 1024;
constexpr size_t MAX_NAME = 80;
constexpr unsigned PAGE_SIZE = 40;
constexpr char lower(char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; }
constexpr bool sameStem(const char* s, const char* word) {
  size_t i = 0;
  while (word[i]) { if (lower(s[i]) != word[i]) return false; ++i; }
  return s[i] == '\0' || s[i] == '.';
}
constexpr bool reserved(const char* s) {
  if (sameStem(s, "con") || sameStem(s, "prn") || sameStem(s, "aux") || sameStem(s, "nul")) return true;
  const bool serial = lower(s[0]) == 'c' && lower(s[1]) == 'o' && lower(s[2]) == 'm';
  const bool printer = lower(s[0]) == 'l' && lower(s[1]) == 'p' && lower(s[2]) == 't';
  return (serial || printer) && s[3] >= '1' && s[3] <= '9' && (s[4] == '\0' || s[4] == '.');
}
constexpr bool validName(const char* s) {
  if (!s || !s[0] || s[0] == '.' || s[0] == ' ') return false;
  size_t n = 0;
  while (s[n]) {
    char c = s[n];
    if (++n > MAX_NAME) return false;
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == ' ')) return false;
  }
  if (s[n - 1] == '.' || s[n - 1] == ' ') return false;
  return !reserved(s);
}
constexpr bool canStart(uint64_t expected, uint64_t freeBytes) {
  return expected <= MAX_FILE && freeBytes >= RESERVE && expected <= freeBytes - RESERVE;
}
constexpr bool canAppend(uint64_t received, uint64_t chunk, uint64_t expected) {
  return expected <= MAX_FILE && received <= expected && chunk <= expected - received;
}
constexpr bool complete(uint64_t received, uint64_t expected) { return expected <= MAX_FILE && received == expected; }
constexpr bool recoverCalibration(bool calibrated, bool pressed, bool previousPressed, bool transferActive) {
  return !calibrated && pressed && !previousPressed && !transferActive;
}
constexpr bool pairUsable(bool available,uint32_t now,uint32_t deadline) {
  return available&&int32_t(deadline-now)>0;
}
constexpr bool acceptPair(bool& available,uint32_t now,uint32_t deadline,bool matches) {
  if(!matches||!pairUsable(available,now,deadline))return false;
  available=false;return true;
}
struct TouchGate {
  bool down=false,handled=false;
  uint32_t since=0;
  constexpr void update(bool pressed,uint32_t now) {
    if(!pressed){down=false;handled=false;return;}
    if(!down){down=true;handled=false;since=now;}
  }
  constexpr bool sample(uint32_t now)const {return down&&!handled&&uint32_t(now-since)>=20;}
  constexpr void consume(){handled=true;}
};
}  // namespace nas
