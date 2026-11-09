// Pruebas del código C++ real, sin hardware. Ejemplo: g++ -std=c++17 reglas_test.cpp -o reglas_test
#include <assert.h>
#include <stdio.h>
#include "../CazaCamarasMS/Reglas.h"
#include "../CazaCamarasMS/TablaOui.h"

int main() {
  assert(cameraPattern("v380-TEST") >= 0);
  assert(cameraPattern("Mi red normal") == -1);
  assert(cameraPattern("HDWIFICAM-1234") == 0);
  assert(cameraPattern("xxHDWIFICAMxx") == -1);      // solo el principio del nombre
  assert(cameraPattern("MOVISTAR_A9F0") == -1);      // redes de casa corrientes
  assert(cameraPattern("SmartLife-1A2B") == -1);     // también lo usan enchufes
  assert(NET_CONF[cameraPattern("EZVIZ_C6N_123")] == ALTA);
  assert(NET_CONF[cameraPattern("CAMERA-01")] == MEDIA);
  assert(cameraPattern("IPC365-9") == 4);            // el largo antes que «IPC»
  const char* patterns[] = {"IPC","IPCAM","HIK","EZVIZ","DAHUA","IMOU","REOLINK","V380","WIFICAM","HDWIFICAM","LOOKCAM","IPC365","YI-","TAPO_CAM","CAMERA","CAM-","JXL"};
  for (const auto* p : patterns) assert(cameraPattern(p) >= 0);
  assert(bleHint("Tiny CaM") == BAJA);
  assert(bleHint("v380-test") == MEDIA);
  assert(bleHint("iPc") == MEDIA);
  assert(bleHint("SPY") == BAJA);
  assert(bleHint("Auriculares") == NINGUNA);
  assert(ouiSuspect(ALTA, false, 0));
  assert(!ouiSuspect(MEDIA, false, 30000));
  assert(ouiSuspect(MEDIA, false, 30001));
  assert(!ouiSuspect(BAJA, false, 3600000));
  assert(ouiSuspect(BAJA, true, 0));
  assert(ouiSuspect(NINGUNA, true, 0));
  uint8_t hik[] = {0x0C,0x75,0xD2,0,0,1};
  assert(lookupOui(hik) && lookupOui(hik)->confidence == ALTA);
  hik[0] |= 2; assert(!lookupOui(hik));  // dirección local: no atribuir
  hik[0] |= 1; assert(!lookupOui(hik));  // multidifusión
  uint8_t unregistered[] = {0,0x62,0x6E,0,0,1};
  assert(!lookupOui(unregistered));
  assert(uplinkData(0x08,0x01));
  assert(uplinkData(0x88,0x01));
  assert(!uplinkData(0x08,0x02));  // bajada del AP
  assert(!uplinkData(0x08,0x03));  // WDS
  assert(!uplinkData(0x08,0x09));  // reintento
  assert(!uplinkData(0x48,0x01));  // nulo
  assert(!uplinkData(0xC8,0x01));  // QoS nulo
  assert(!uplinkData(0x80,0x01));  // baliza
  assert(!payloadData(0x48));
  assert(payloadData(0x88));
  TrafficRun r;
  for (uint32_t t=250;t<=10250;t+=250) { r.sample(t, 7000, 6, 5, 250); assert(!r.recent(t)); }
  r.sample(10500,7000,6,5,250); assert(r.recent(10500));
  assert(!r.recent(16501));  // no mantener vídeo sin muestras
  r.sample(17000,7000,6,5,250); assert(!r.recent(17000));
  r.reset();
  for (uint32_t t=250;t<=10000;t+=3250) { r.sample(t,7000,6,5,250); assert(!r.recent(t)); }
  r.sample(13250,7000,6,5,250); assert(r.recent(13250));  // salto de 13 canales
  r.sample(16500,5000,6,5,250); assert(!r.recent(16500)); // umbral estricto
  for (uint32_t t=20000;t<40000;t+=250) r.sample(t,10000,10,7,250);
  assert(!r.recent(39750));  // exactamente 70 % no basta
  r.sample(40000,100000,100,100,2000); assert(!r.recent(40000)); // pausa larga
  r.reset();
  uint32_t start = UINT32_MAX - 11000;
  for (uint32_t dt=0;dt<=12000;dt+=250) r.sample(start+dt,7000,6,5,250);
  assert(r.recent(start+12000));  // desbordamiento de millis
  TrafficWindow w;
  w.add(1000,100); w.add(9999,200);
  uint32_t frames, bytes;
  w.totals(10000,frames,bytes); assert(frames==2 && bytes==300);
  w.totals(11000,frames,bytes); assert(frames==1 && bytes==200);
  w.add(19000,600); w.totals(19000,frames,bytes); assert(frames==1 && bytes==600);
  puts("Reglas: pruebas superadas.");
}
