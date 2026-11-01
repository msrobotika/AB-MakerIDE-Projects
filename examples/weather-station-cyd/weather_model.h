#pragma once
#include <ArduinoJson.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace meteo {
constexpr uint32_t REFRESH_MS = 15UL * 60 * 1000;
constexpr uint32_t RETRY_MS = 2UL * 60 * 1000;
constexpr uint32_t MANUAL_MS = 60UL * 1000;
constexpr uint32_t STALE_MS = 30UL * 60 * 1000;
struct Day { char date[11]; float low, high, rainChance; int code; };
struct Weather {
  char place[81], region[97], time[20];
  float temperature, apparent, humidity, wind, rain, pressure;
  int code; Day days[3];
};
struct Request { char city[81], country[3]; uint32_t generation; };
struct Result { bool ok; uint32_t generation; char error[80]; Weather weather; };

inline bool elapsed(uint32_t now, uint32_t since, uint32_t interval) {
  return uint32_t(now - since) >= interval;
}
inline bool number(JsonVariantConst value, float low, float high, float &out) {
  if (!value.is<float>()) return false;
  const float n = value.as<float>();
  if (!isfinite(n) || n < low || n > high) return false;
  out = n; return true;
}
inline bool weatherCode(JsonVariantConst value, int &out) {
  float n;
  if (!number(value, 0, 99, n) || floorf(n) != n) return false;
  out = int(n); return true;
}
inline bool dateValid(const char *s) {
  if (!s || strlen(s) < 10 || s[4] != '-' || s[7] != '-') return false;
  for (int i=0; i<10; ++i) if (i!=4 && i!=7 && (s[i]<'0' || s[i]>'9')) return false;
  int m=(s[5]-'0')*10+s[6]-'0', d=(s[8]-'0')*10+s[9]-'0';
  return m>=1 && m<=12 && d>=1 && d<=31;
}
inline bool parseWeather(JsonDocument &doc, Weather &out) {
  if (!doc["current"].is<JsonObject>() || !doc["daily"].is<JsonObject>()) return false;
  const char *stamp = doc["current"]["time"] | "";
  if (strlen(stamp)!=16 || !dateValid(stamp) || stamp[10]!='T' || stamp[13]!=':' ||
      stamp[11]<'0' || stamp[11]>'2' || stamp[12]<'0' || stamp[12]>'9' ||
      (stamp[11]=='2' && stamp[12]>'3') || stamp[14]<'0' || stamp[14]>'5' ||
      stamp[15]<'0' || stamp[15]>'9') return false;
  const char *keys[]={"temperature_2m","apparent_temperature","relative_humidity_2m",
    "wind_speed_10m","precipitation","pressure_msl"};
  const char *units[]={"°C","°C","%","km/h","mm","hPa"};
  for (int i=0; i<6; ++i)
    if (strcmp(doc["current_units"][keys[i]] | "", units[i])) return false;
  if (strcmp(doc["daily_units"]["temperature_2m_min"] | "", "°C") ||
      strcmp(doc["daily_units"]["temperature_2m_max"] | "", "°C") ||
      strcmp(doc["daily_units"]["precipitation_probability_max"] | "", "%")) return false;
  JsonObjectConst cur=doc["current"];
  if (!number(cur[keys[0]],-100,70,out.temperature) ||
      !number(cur[keys[1]],-150,100,out.apparent) ||
      !number(cur[keys[2]],0,100,out.humidity) ||
      !number(cur[keys[3]],0,500,out.wind) ||
      !number(cur[keys[4]],0,2000,out.rain) ||
      !number(cur[keys[5]],800,1100,out.pressure) ||
      !weatherCode(cur["weather_code"],out.code)) return false;
  const char *dailyKeys[]={"time","temperature_2m_min","temperature_2m_max",
    "precipitation_probability_max","weather_code"};
  for (auto key: dailyKeys) if (!doc["daily"][key].is<JsonArray>() || doc["daily"][key].size()!=3) return false;
  for (int i=0;i<3;++i) {
    Day &d=out.days[i];
    const char *date=doc["daily"]["time"][i] | "";
    if (strlen(date)!=10 || !dateValid(date) ||
        !number(doc["daily"]["temperature_2m_min"][i],-100,70,d.low) ||
        !number(doc["daily"]["temperature_2m_max"][i],-100,70,d.high) || d.low>d.high ||
        !number(doc["daily"]["precipitation_probability_max"][i],0,100,d.rainChance) ||
        !weatherCode(doc["daily"]["weather_code"][i],d.code)) return false;
    snprintf(d.date,sizeof(d.date),"%s",date);
    if (i && strcmp(out.days[i-1].date,d.date)>=0) return false;
  }
  snprintf(out.time,sizeof(out.time),"%s",stamp);
  return true;
}
inline const char *condition(int code) {
  switch (code) {
    case 0:return "Despejado"; case 1:return "Casi despejado";
    case 2:return "Nubes y claros"; case 3:return "Cubierto";
    case 45:case 48:return "Niebla";
    case 51:case 53:case 55:return "Llovizna";
    case 56:case 57:return "Llovizna helada";
    case 61:case 63:case 65:return "Lluvia";
    case 66:case 67:return "Lluvia helada";
    case 71:case 73:case 75:case 77:return "Nieve";
    case 80:case 81:case 82:return "Chubascos";
    case 85:case 86:return "Chubascos nieve";
    case 95:case 96:case 99:return "Tormenta";
    default:return "Estado sin traducir";
  }
}
} // namespace meteo
