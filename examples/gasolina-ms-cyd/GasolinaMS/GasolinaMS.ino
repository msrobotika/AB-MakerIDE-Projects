/*
  Gasolina MS · AB MakerIDE / MS Robotika
  Las gasolineras públicas más baratas de tu zona: España, Francia y Austria (datos oficiales).
  CYD ESP32-2432S028R, ILI9341 horizontal 320x240, táctil XPT2046.
  Desliza: CERCA, AHORA, TOP 5, TENDENCIA y AJUSTES.
  Pueblo cada 30 minutos, provincia cada 60, posición Wi-Fi cada 2 minutos.
  LED: verde si baja o iguala ayer; ámbar si sube; rojo intermitente en error.

  Datos abiertos del Ministerio para la Transición Ecológica y el Reto Demográfico.
  API pública sedeaplicaciones.minetur.gob.es, sin cuenta ni clave.
  HTTPS validado con DigiCert Global Root G2, incluida abajo. Nunca TLS inseguro.
  Primera vez: red Gasolina-MS y QR para configurar solo la Wi-Fi.
  Después: QR de ajustes en la red local para elegir pueblo, combustible y litros.
  Preferencias y credenciales se guardan solo en la placa (NVS).
  Cerca de mí usa beaconDB: MAC/señal de redes fijas, sin nombres ni hotspot conectado.
  Posición solo en RAM; nunca se muestran ni registran redes o coordenadas.

  Bibliotecas: Adafruit GFX, Adafruit ILI9341, XPT2046_Touchscreen (Paul Stoffregen),
  ArduinoJson 7. Placa: ESP32 Dev Module, núcleo esp32 3.x.
  Pines: github.com/witnessmenow/ESP32-Cheap-Yellow-Display/PINS.md
  Pantalla, calibración y ayudantes adaptados de Kilovatio MS.
  Precios orientativos: pueden tardar en actualizarse; confirma en el surtidor.
  Evidencia: probado en la placa el 10 nov 2026 en España, Francia y Austria (README, «Evidencia»).
  Licencia MIT, como el resto de AB MakerIDE.
*/
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <qrcode.h>
#include <time.h>

// ---------- Hardware de la CYD ----------
SPIClass lcdBus(HSPI), touchBus(VSPI);
Adafruit_ILI9341 lcd(&lcdBus, 2, 15, -1);
Adafruit_GFX* gfx = &lcd;  // Todo se dibuja por aquí; las capturas por USB lo cambian un momento.
XPT2046_Touchscreen touch(33, 36);
constexpr int LED_R = 4, LED_G = 16, LED_B = 17;  // activo a nivel bajo
constexpr int PIN_BL = 21, PIN_LDR = 34, PIN_SPK = 26;
constexpr int W = 320, H = 240;

// ---------- Colores ----------
constexpr uint16_t BG = 0x0861, PANEL = 0x18E4, LINE = 0x31A6, TXT = 0xFFFF, DIM = 0x9CF3;
constexpr uint16_t CHEAP = 0x2E8A, MID = 0xFE41, DEAR = 0xF8A6, ACC = 0x05FF, NEG = 0x6E7F;
constexpr uint16_t INK = 0x0000;  // texto sobre fondos de color: negro puro para que se lea bien

// ---------- CA real: DigiCert Global Root G2 ----------
// SHA-256: CB3CCBB76031E5E0138F8DD39A23F9DE47FFC35E43C1144CEA27D46A5AB1CB5F
const char ROOT_CA[] PROGMEM = R"PEM(-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBhMQswCQYD
VQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3d3cuZGlnaWNlcnQu
Y29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBHMjAeFw0xMzA4MDExMjAwMDBa
Fw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVTMRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMx
GTAXBgNVBAsTEHd3dy5kaWdpY2VydC5jb20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBS
b290IEcyMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/
RrohCgiN9RlUyfuI2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxps
MNzaHxmx1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5WztCO7TG1F
8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQvIOlCsRnKPZzFBQ9
RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAPBgNVHRMBAf8EBTADAQH/MA4G
A1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcN
AQELBQADggEBAGBnKJRvDkhj6zHd6mcY1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVn
NeyIv/iPsGEMNKSuIEyExtv4NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2Z
L7tcu7XUIOGZX1NGFdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUT
Fy8bJZ918rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTflMrY=
-----END CERTIFICATE-----
)PEM";

// ---------- beaconDB: ra?ces oficiales, https://letsencrypt.org/certificates/ ----------
// X2 SHA-256: 69729B8E15A86EFC177A57AFB7171DFC64ADD28C2FCA8CF1507E34453CCB1470
// YE SHA-256: E14FFCAD5B0025731006CAA43A121A22D8E9700F4FB9CF852F02A708AA5D5666
// Ambas autofirmadas; YE tambi?n contrastada con su certificado cruzado por X2.
const char BEACON_CA[] PROGMEM = R"PEM(-----BEGIN CERTIFICATE-----
MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw
CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg
R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00
MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT
ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw
EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW
+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9
ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T
AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI
zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW
tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1
/q4AaOeMSQ+2b1tbFfLn
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIB2TCCAWCgAwIBAgIRAKQCa6LvbHwg1AR+XmWmk4AwCgYIKoZIzj0EAwMwLjEL
MAkGA1UEBhMCVVMxDTALBgNVBAoTBElTUkcxEDAOBgNVBAMTB1Jvb3QgWUUwHhcN
MjUwOTAzMDAwMDAwWhcNNDUwOTAyMjM1OTU5WjAuMQswCQYDVQQGEwJVUzENMAsG
A1UEChMESVNSRzEQMA4GA1UEAxMHUm9vdCBZRTB2MBAGByqGSM49AgEGBSuBBAAi
A2IABDwS/6vhrcVqcbBo+wgdI3fwn9x7DNJJOY/lTOti0vkwuRN87RhEhTH17E7X
yFjWsPYhIPt/wzOqxTd2b+4ZJNy9ID04YywF9U5zasDVyGSNErVNtz8uSGh5izW8
7j77GaNCMEAwDgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0O
BBYEFKPIJlqOoUzQNWP8myPIOq5W809WMAoGCCqGSM49BAMDA2cAMGQCMHhMr8N9
LdL1VQKs9BdV81r76eXRB6mtjuNjzk6/lBsPNToWLTDzGYgtQKO1jl63uAIwGV7m
onyF377c+MM1oqVNs17sgu7F9YKZwgLmVbeOMDbKAXHtKMDLbiGllCcs8f47
-----END CERTIFICATE-----
)PEM";

// ---------- Francia (data.economie.gouv.fr): USERTrust RSA Certification Authority ----------
// SHA-256: E793C9B02FD8AA13E21C31228ACCB08119643B749C898964B1746D46C3D4CBD2 (Sectigo/USERTrust)
const char FR_CA[] PROGMEM = R"PEM(-----BEGIN CERTIFICATE-----
MIIF3jCCA8agAwIBAgIQAf1tMPyjylGoG7xkDjUDLTANBgkqhkiG9w0BAQwFADCB
iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl
cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV
BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAw
MjAxMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNV
BAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU
aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2Vy
dGlmaWNhdGlvbiBBdXRob3JpdHkwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIK
AoICAQCAEmUXNg7D2wiz0KxXDXbtzSfTTK1Qg2HiqiBNCS1kCdzOiZ/MPans9s/B
3PHTsdZ7NygRK0faOca8Ohm0X6a9fZ2jY0K2dvKpOyuR+OJv0OwWIJAJPuLodMkY
tJHUYmTbf6MG8YgYapAiPLz+E/CHFHv25B+O1ORRxhFnRghRy4YUVD+8M/5+bJz/
Fp0YvVGONaanZshyZ9shZrHUm3gDwFA66Mzw3LyeTP6vBZY1H1dat//O+T23LLb2
VN3I5xI6Ta5MirdcmrS3ID3KfyI0rn47aGYBROcBTkZTmzNg95S+UzeQc0PzMsNT
79uq/nROacdrjGCT3sTHDN/hMq7MkztReJVni+49Vv4M0GkPGw/zJSZrM233bkf6
c0Plfg6lZrEpfDKEY1WJxA3Bk1QwGROs0303p+tdOmw1XNtB1xLaqUkL39iAigmT
Yo61Zs8liM2EuLE/pDkP2QKe6xJMlXzzawWpXhaDzLhn4ugTncxbgtNMs+1b/97l
c6wjOy0AvzVVdAlJ2ElYGn+SNuZRkg7zJn0cTRe8yexDJtC/QV9AqURE9JnnV4ee
UB9XVKg+/XRjL7FQZQnmWEIuQxpMtPAlR1n6BB6T1CZGSlCBst6+eLf8ZxXhyVeE
Hg9j1uliutZfVS7qXMYoCAQlObgOK6nyTJccBz8NUvXt7y+CDwIDAQABo0IwQDAd
BgNVHQ4EFgQUU3m/WqorSs9UgOHYm8Cd8rIDZsswDgYDVR0PAQH/BAQDAgEGMA8G
A1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAFzUfA3P9wF9QZllDHPF
Up/L+M+ZBn8b2kMVn54CVVeWFPFSPCeHlCjtHzoBN6J2/FNQwISbxmtOuowhT6KO
VWKR82kV2LyI48SqC/3vqOlLVSoGIG1VeCkZ7l8wXEskEVX/JJpuXior7gtNn3/3
ATiUFJVDBwn7YKnuHKsSjKCaXqeYalltiz8I+8jRRa8YFWSQEg9zKC7F4iRO/Fjs
8PRF/iKz6y+O0tlFYQXBl2+odnKPi4w2r78NBc5xjeambx9spnFixdjQg3IM8WcR
iQycE0xyNN+81XHfqnHd4blsjDwSXWXavVcStkNr/+XeTWYRUc+ZruwXtuhxkYze
Sf7dNXGiFSeUHM9h4ya7b6NnJSFd5t0dCy5oGzuCr+yDZ4XUmFF0sbmZgIn/f3gZ
XHlKYC6SQK5MNyosycdiyA5d9zZbyuAlJQG03RoHnHcAP9Dc1ew91Pq7P8yF1m9/
qS3fuQL39ZeatTXaw2ewh0qpKJ4jjv9cJ2vhsE/zB+4ALtRZh8tSQZXq9EfX7mRB
VXyNWQKV3WKdwrnuWih0hKWbt5DHDAff9Yk2dDLWKMGwsAvgnEzDHNb842m1R0aB
L6KCq9NjRHDEjf8tM7qtj3u1cIiuPhnPQCjY/MiQu12ZIvVS5ljFH4gxQ+6IHdfG
jjxDah2nGN59PRbxYvnKkKj9
-----END CERTIFICATE-----
)PEM";

// ---------- Austria (api.e-control.at): HARICA TLS RSA Root CA 2021 + RootCA 2015 (firma cruzada) ----------
// 2015 SHA-256: A040929A02CE53B4ACF4F2FFC6981CE4496F755E6D45FE0B2A692BCD52523F36
// 2021 SHA-256: D95D0E8EDA79525BF9BEB11B14D2100D3294985F0C62D9FABD9CD999ECCB7B1D
const char AT_CA[] PROGMEM = R"PEM(-----BEGIN CERTIFICATE-----
MIIGCzCCA/OgAwIBAgIBADANBgkqhkiG9w0BAQsFADCBpjELMAkGA1UEBhMCR1Ix
DzANBgNVBAcTBkF0aGVuczFEMEIGA1UEChM7SGVsbGVuaWMgQWNhZGVtaWMgYW5k
IFJlc2VhcmNoIEluc3RpdHV0aW9ucyBDZXJ0LiBBdXRob3JpdHkxQDA+BgNVBAMT
N0hlbGxlbmljIEFjYWRlbWljIGFuZCBSZXNlYXJjaCBJbnN0aXR1dGlvbnMgUm9v
dENBIDIwMTUwHhcNMTUwNzA3MTAxMTIxWhcNNDAwNjMwMTAxMTIxWjCBpjELMAkG
A1UEBhMCR1IxDzANBgNVBAcTBkF0aGVuczFEMEIGA1UEChM7SGVsbGVuaWMgQWNh
ZGVtaWMgYW5kIFJlc2VhcmNoIEluc3RpdHV0aW9ucyBDZXJ0LiBBdXRob3JpdHkx
QDA+BgNVBAMTN0hlbGxlbmljIEFjYWRlbWljIGFuZCBSZXNlYXJjaCBJbnN0aXR1
dGlvbnMgUm9vdENBIDIwMTUwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoIC
AQDC+Kk/G4n8PDwEXT2QNrCROnk8ZlrvbTkBSRq0t89/TSNTt5AA4xMqKKYx8ZEA
4yjsriFBzh/a/X0SWwGDD7mwX5nh8hKDgE0GPt+sr+ehiGsxr/CL0BgzuNtFajT0
AoAkKAoCFZVedioNmToUW/bLy1O8E00BiDeUJRtCvCLYjqOWXjrZMts+6PAQZe10
4S+nfK8nNLspfZu2zwnI5dMK/IhlZXQK3HMcXM1AsRzUtoSMTFDPaI6oWa7CJ06C
ojXdFPQf/7J31Ycvqm59JCfnxssm5uX+Zwdj2EUN3TpZZTlYepKZcj2chF6IIbjV
9Cz82XBST3i4vTwri5WY9bPRaM8gFH5MXF/ni+X1NYEZN9cRCLdmvtNKzoNXADrD
gfgXy5I2XdGj2HUb4Ysn6npIQf1FGQatJ5lOwXBH3bWfgVMS5bGMSF0xQxfjjMZ6
Y5ZLKTBOhE5iGV48zpeQpX8B653g+IuJ3SWYPZK2fu/Z8VFRfS0myGlZYeCsargq
NhEEelC9MoS+L9xy1dcdFkfkR2YgP/SWxa+OAXqlD3pk9Q0Yh9muiNX6hME6wGko
LfINaFGq46V3xqSQDqE3izEjR8EJCOtu93ib14L8hCCZSRm2Ekax+0VVFqmjZayc
Bw/qa9wfLgZy7IaIEuQt218FL+TwA9MmM+eAws1CoRc0CwIDAQABo0IwQDAPBgNV
HRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBBjAdBgNVHQ4EFgQUcRVnyMjJvXVd
ctA4GGqd83EkVAswDQYJKoZIhvcNAQELBQADggIBAHW7bVRLqhBYRjTyYtcWNl0I
XtVsyIe9tC5G8jH4fOpCtZMWVdyhDBKg2mF+D1hYc2Ryx+hFjtyp8iY/xnmMsVMI
M4GwVhO+5lFc2JsKT0ucVlMC6U/2DWDqTUJV6HwbISHTGzrMd/K4kPFox/la/vot
9L/J9UUbzjgQKjeKeaO04wlshYaT/4mWJ3iBj2fjRnRUjtkNaeJK9E10A/+yd+2V
Z5fkscWrv2oj6NSU4kQoYsRL4vDY4ilrGnB+JGGTe08DMiUNRSQrlrRGar9KC/ea
j8GsGsVn82800vpzY4zvFrCopEYq+OsS7HK07/grfoxSwIuEVPkvPuNVqNxmsdnh
X9izjFk0WaSrT2y7HxjbdavYy5LNlDhhDgcGH0tGEPEVvo2FXDtKK4F5D7Rpn0lQ
l033DlZdwJVqwjbDG2jJ9SrcR5q+ss7FJej6A7na+RZukYT1HCjI/CbM1xyQVqdf
bzoEvM14iQuODy+jqk+iGxI9FghAD/FGTNeqewjBCvVtJ94Cj8rDtSvK6evIIVM4
pcw72Hc3MKJP2W/R8kCtQXoXxdZKNYm3QdV8hn9VTYNKpXMgwDqvkPGaJI7ZjnHK
e7iG2rKPmT4dEw0SEe7Uq/DpFXYC5ODfqiAeW2GFZECpkJcNrVPSWh2HagCXZWK0
vm9qp/UsQu0yrbYhnr68
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIFpDCCA4ygAwIBAgIQOcqTHO9D88aOk8f0ZIk4fjANBgkqhkiG9w0BAQsFADBs
MQswCQYDVQQGEwJHUjE3MDUGA1UECgwuSGVsbGVuaWMgQWNhZGVtaWMgYW5kIFJl
c2VhcmNoIEluc3RpdHV0aW9ucyBDQTEkMCIGA1UEAwwbSEFSSUNBIFRMUyBSU0Eg
Um9vdCBDQSAyMDIxMB4XDTIxMDIxOTEwNTUzOFoXDTQ1MDIxMzEwNTUzN1owbDEL
MAkGA1UEBhMCR1IxNzA1BgNVBAoMLkhlbGxlbmljIEFjYWRlbWljIGFuZCBSZXNl
YXJjaCBJbnN0aXR1dGlvbnMgQ0ExJDAiBgNVBAMMG0hBUklDQSBUTFMgUlNBIFJv
b3QgQ0EgMjAyMTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAIvC569l
mwVnlskNJLnQDmT8zuIkGCyEf3dRywQRNrhe7Wlxp57kJQmXZ8FHws+RFjZiPTgE
4VGC/6zStGndLuwRo0Xua2s7TL+MjaQenRG56Tj5eg4MmOIjHdFOY9TnuEFE+2uv
a9of08WRiFukiZLRgeaMOVig1mlDqa2YUlhu2wr7a89o+uOkXjpFc5gH6l8Cct4M
pbOfrqkdtx2z/IpZ525yZa31MJQjB/OCFks1mJxTuy/K5FrZx40d/JiZ+yykgmvw
Kh+OC19xXFyuQnspiYHLA6OZyoieC0AJQTPb5lh6/a6ZcMBaD9YThnEvdmn8kN3b
LW7R8pv1GmuebxWMevBLKKAiOIAkbDakO/IwkfN4E8/BPzWr8R0RI7VDIp4BkrcY
AuUR0YLbFQDMYTfBKnya4dC6s1BG7oKsnTH4+yPiAwBIcKMJJnkVU2DzOFytOOqB
AGMUuTNe3QvboEUHGjMJ+E20pwKmafTCWQWIZYVWrkvL4N48fS0ayOn7H6NhStYq
E613TBoYm5EPWNgGVMWX+Ko/IIqmhaZ39qb8HOLubpQzKoNQhArlT4b4UEV4AIHr
W2jjJo3Me1xR9BQsQL4aYB16cmEdH2MtiKrOokWQCPxrvrNQKlr9qEgYRtaQQJKQ
CoReaDH46+0N0x3GfZkYVVYnZS6NRcUk7M7jAgMBAAGjQjBAMA8GA1UdEwEB/wQF
MAMBAf8wHQYDVR0OBBYEFApII6ZgpJIKM+qTW8VX6iVNvRLuMA4GA1UdDwEB/wQE
AwIBhjANBgkqhkiG9w0BAQsFAAOCAgEAPpBIqm5iFSVmewzVjIuJndftTgfvnNAU
X15QvWiWkKQUEapobQk1OUAJ2vQJLDSle1mESSmXdMgHHkdt8s4cUCbjnj1AUz/3
f5Z2EMVGpdAgS1D0NTsY9FVqQRtHBmg8uwkIYtlfVUKqrFOFrJVWNlar5AWMxaja
H6NpvVMPxP/cyuN+8kyIhkdGGvMA9YCRotxDQpSbIPDRzbLrLFPCU3hKTwSUQZqP
JzLB5UkZv/HywouoCjkxKLR9YjYsTewfM7Z+d21+UPCfDtcRj88YxeMn/ibvBZ3P
zzfF0HvaO7AWhAw6k9a+F9sPPg4ZeAnHqQJyIkv3N3a6dcSFA1pj1bF1BcK5vZSt
jBWZp5N99sXzqnTPBIWUmAD04vnKJGW/4GKvyMX6ssmeVkjaef2WdhW+o45WxLM0
/L5H9MG0qPzVMIho7suuyWPEdr6sOBjhXlzPrjoiUevRi7PzKzMHVIf6tLITe7pT
BGIBnfHAT+7hOtSLIBD6Alfm78ELt5BGnBkpjNxvoEppaZS3JGWg/6w/zgH7IS79
aPib8qXPMThcFarmlwDB31qlpzmq6YR/PFGoOtmUW4y/Twhx5duoXNTSpv4Ao8YW
xw/ogM4cKGR0GQjTQuPOAF1/sdwTsOEFy9EgqoZ0njnnkf3/W9b3raYvAwtt41dU
63ZTGI0RmLo=
-----END CERTIFICATE-----
)PEM";

// ---------- Estado y tipos, antes de los ayudantes ----------
Preferences prefs;
WebServer web(80);
DNSServer dns;
const char BASE_URL[] = "https://sedeaplicaciones.minetur.gob.es/ServiciosRESTCarburantes/PreciosCarburantes";
const uint8_t PRODUCT_IDS[] = {1, 3, 4, 5, 17};
const char* const FUELS[] = {"Gasolina 95 E5", "Gasolina 98 E5", "Gasóleo A", "Gasóleo Premium", "GLP"};
// Países: 0 España (Ministerio), 1 Francia, 2 Austria. Clave NVS "pais"; Config no cambia de formato.
// En Francia `config.town` es el código postal y en Austria el código de distrito (Bezirk) de E-Control.
const char* const COUNTRIES[] = {"España", "Francia", "Austria"};
const char* const SOURCE[] = {"Ministerio", "carburants.gouv.fr", "E-Control"};
const char FR_BASE[] = "https://data.economie.gouv.fr/api/explore/v2.1/catalog/datasets/"
                       "prix-des-carburants-en-france-flux-instantane-v2/records";
const char AT_BASE[] = "https://api.e-control.at/sprit/1.0";
const char* const FR_FIELDS[] = {"gazole_prix", "sp95_prix", "e10_prix", "sp98_prix", "gplc_prix"};
const char* const FR_FUELS[] = {"Gazole (diésel)", "SP95", "SP95-E10", "SP98", "GPLc"};
const char* const AT_CODES[] = {"DIE", "SUP"};  // Sin GNC: se vende por kg, no por litro.
const char* const AT_FUELS[] = {"Diésel", "Super 95"};
uint8_t country = 0;
float fetchLat = 0, fetchLon = 0;  // Punto de la última consulta CERCA en Francia/Austria.
enum Screen { S_NEAR, S_NOW, S_TOP, S_TREND, S_SETTINGS, S_COUNT };
const char* const SCREEN_NAMES[] = {"CERCA", "AHORA", "TOP 5", "TENDENCIA", "AJUSTES"};
struct Btn { int x, y, w, h; };
struct Config {
  uint32_t version = 1;
  char province[4] = "", town[8] = "", name[100] = "";
  uint16_t litres = 50;
  uint8_t fuel = 0;
};
struct Station { char brand[96] = "", address[160] = ""; int mills = 0; };
struct Summary {
  Station best[5];
  uint32_t count = 0;
  uint64_t sum = 0;
  int lo = 0, hi = 0, date = 0;
  char stamp[24] = "", clock[6] = "--:--";
};
struct Sample { int date = 0, lo = 0; float avg = 0; };
struct History {
  uint32_t version = 1;
  char town[8] = "";
  uint8_t fuel = 0, count = 0;
  Sample days[14];
};
Config config;
Summary data;
History history;
// Config conserva su formato binario probado en placa. Solo el modo tiene otra clave NVS.
bool nearEnabled = true, positionWanted = true, provinceWanted = true;
bool havePosition = false, positionOld = false, provinceValid = false;
float positionLat = 0, positionLon = 0, positionAccuracy = 0;
// Centro del pueblo (media de sus gasolineras): CERCA lo usa si las redes Wi-Fi no dan ubicación.
float townLat = 0, townLon = 0;
bool positionTown = false;
int beaconCode = 0, beaconNets = 0;  // Diagnóstico de la última consulta: solo código HTTP y nº de redes.
uint8_t positionFailures = 0, positionProvince = 0, cachedProvince = 0, cachedFuel = 255;
uint32_t lastPositionTry = 0, lastProvinceTry = 0, provinceFetchedAt = 0;
constexpr uint16_t MAX_NEAR_STATIONS = 1200;
struct NearStation {
  float lat, lon;
  uint32_t mills;
  char brand[21], address[35];
};
struct NearChoice { int cheapest = -1, closest = -1, radius = 5; float cheapKm = 0, closeKm = 0; };
NearStation* nearStations = nullptr;  // Un solo array, sin duplicar ~82 kB al renovar.
uint16_t nearCapacity = 0;
float nearCompleteKm = 1e9f;  // Francia: si la consulta llegó cortada, la lista solo está completa hasta esta distancia.  // 1200 en España; 100 en Francia/Austria (sus consultas no devuelven más).
uint16_t nearCount = 0;
Summary provinceMeta;  // Solo fecha/hora; no mezcla estadísticas con las del pueblo.
char nearError[80] = "";
int screen = S_NOW;
bool portalMode = false, autoBright = true, refreshWanted = true, busy = false;
String apPass, settingsUrl, formToken;
float ax = 0, bx = 0, cx = 0, ay = 0, by = 0, cy = 0;
uint32_t lastFetchTry = 0, lastBright = 0, lastTick = 0, lastReconnect = 0, fetchedAt = 0;
char lastError[80] = "";
const Btn B_REFRESH = {8, 87, 150, 26}, B_BRIGHT = {8, 118, 150, 26};
const Btn B_CAL = {8, 149, 150, 26}, B_WIFI = {8, 180, 150, 16};
const Btn B_RETRY = {75, 151, 170, 30};
uint8_t fuelCount(uint8_t c) { return c == 2 ? 2 : 5; }
const char* fuelName() {
  return country == 1 ? FR_FUELS[config.fuel] : country == 2 ? AT_FUELS[config.fuel] : FUELS[config.fuel];
}
// TLS + JSON de Francia/Austria desbordaban los 8 kB de pila por defecto de loop().
SET_LOOP_TASK_STACK_SIZE(16 * 1024);
// ---------- Texto con acentos (fuente clasica CP437) y simbolo euro ----------
const uint8_t EURO[7] = {0x07, 0x08, 0x1E, 0x08, 0x1E, 0x08, 0x07};

void euro(int x, int y, uint8_t s, uint16_t fg, int32_t bg) {
  if (bg >= 0) gfx->fillRect(x, y, 6 * s, 8 * s, bg);
  for (int r = 0; r < 7; r++)
    for (int c = 0; c < 5; c++)
      if (EURO[r] & (0x10 >> c)) gfx->fillRect(x + c * s, y + r * s, s, s, fg);
}

uint8_t mapUtf8(const uint8_t*& p, bool& isEuro) {
  isEuro = false;
  uint8_t c = *p++;
  if (c < 0x80) return c;
  if (c == 0xC3) {
    if (!*p) return '?';
    switch (*p++) {
      case 0xA1: return 0xA0; case 0xA9: return 0x82; case 0xAD: return 0xA1;
      case 0xB3: return 0xA2; case 0xBA: return 0xA3; case 0xB1: return 0xA4;
      case 0x91: return 0xA5; case 0xBC: return 0x81; case 0x89: return 0x90;
      case 0x81: return 'A'; case 0x8D: return 'I'; case 0x93: return 'O'; case 0x9A: return 'U';
      // Francés y alemán (direcciones de Francia y Austria), en CP437.
      case 0xA4: return 0x84; case 0xB6: return 0x94; case 0x84: return 0x8E; case 0x96: return 0x99;
      case 0x9C: return 0x9A; case 0x9F: return 0xE1; case 0xA8: return 0x8A; case 0xAA: return 0x88;
      case 0xA0: return 0x85; case 0xA2: return 0x83; case 0xA7: return 0x87; case 0xB4: return 0x93;
      case 0xBB: return 0x96; case 0xAE: return 0x8C; case 0xAB: return 0x89; case 0xAF: return 0x8B;
      case 0x87: return 0x80;
      default: return '?';
    }
  }
  if (c == 0xC2) {
    if (!*p) return '?';
    uint8_t d = *p++;
    return d == 0xBF ? 0xA8 : d == 0xA1 ? 0xAD : d == 0xBA ? 0xF8 : d == 0xB7 ? 0xFA : d == 0xB1 ? 0xF1 : '?';
  }
  if (c == 0xE2 && p[0] == 0x82 && p[1] == 0xAC) { p += 2; isEuro = true; return ' '; }
  if (c == 0xE2 && p[0] == 0x88 && p[1] == 0x92) { p += 2; return '-'; }
  while ((*p & 0xC0) == 0x80) p++;
  return '?';
}

int textWidth(const char* s, uint8_t size) {
  int n = 0;
  bool e;
  for (const uint8_t* p = (const uint8_t*)s; *p;) { mapUtf8(p, e); n++; }
  return n * 6 * size;
}

void put(int x, int y, const char* s, uint16_t fg, uint8_t size = 1, int32_t bg = -1) {
  gfx->setFont();
  int cx0 = x;
  bool isEuro;
  for (const uint8_t* p = (const uint8_t*)s; *p;) {
    uint8_t ch = mapUtf8(p, isEuro);
    if (isEuro) euro(cx0, y, size, fg, bg);
    else gfx->drawChar(cx0, y, ch, fg, bg < 0 ? fg : (uint16_t)bg, size);
    cx0 += 6 * size;
  }
}

void putCenter(int cxm, int y, const char* s, uint16_t fg, uint8_t size = 1, int32_t bg = -1) {
  put(cxm - textWidth(s, size) / 2, y, s, fg, size, bg);
}

void putRight(int xr, int y, const char* s, uint16_t fg, uint8_t size = 1, int32_t bg = -1) {
  put(xr - textWidth(s, size), y, s, fg, size, bg);
}

void price(char* out, size_t n, float v, int decimals = 3) {
  snprintf(out, n, "%.*f", decimals, v);
  for (char* c = out; *c; c++) if (*c == '.') *c = ',';
}

void setLed(bool r, bool g, bool b) {
  digitalWrite(LED_R, r ? LOW : HIGH);
  digitalWrite(LED_G, g ? LOW : HIGH);
  digitalWrite(LED_B, b ? LOW : HIGH);
}

void updateBrightness() {
  int level = 230;
  if (autoBright) {
    int v = analogRead(PIN_LDR);  // con 0 dB: luz fuerte ~0, oscuridad sube
    level = constrain(map(v, 0, 450, 255, 35), 35, 255);
  }
  ledcWrite(PIN_BL, level);
}

// ---------- Tactil ----------
bool sampleRaw(int& x, int& y) {
  if (!touch.tirqTouched() || !touch.touched()) return false;
  TS_Point p = touch.getPoint();
  if (p.z < 250) return false;
  x = p.x;
  y = p.y;
  return true;
}

void toScreen(int rx, int ry, int& x, int& y) {
  x = lroundf(ax * rx + bx * ry + cx);
  y = lroundf(ay * rx + by * ry + cy);
}

void calibrate() {
  const int sx[3] = {24, 296, 24}, sy[3] = {24, 24, 216};
  int rx[3], ry[3];
  while (true) {
    for (int i = 0; i < 3; i++) {
      gfx->fillScreen(BG);
      putCenter(160, 100, "CALIBRAR TÁCTIL", ACC, 2);
      putCenter(160, 128, "Toca el centro de la cruz", TXT);
      gfx->drawFastHLine(sx[i] - 10, sy[i], 21, MID);
      gfx->drawFastVLine(sx[i], sy[i] - 10, 21, MID);
      gfx->drawCircle(sx[i], sy[i], 8, MID);
      int x, y;
      while (sampleRaw(x, y)) delay(10);
      int32_t sxr = 0, syr = 0;
      int n = 0;
      while (n < 12) {
        if (sampleRaw(x, y)) { sxr += x; syr += y; n++; }
        else { n = 0; sxr = syr = 0; }
        delay(12);
      }
      rx[i] = sxr / n;
      ry[i] = syr / n;
      while (sampleRaw(x, y)) delay(10);
      delay(150);
    }
    float ux = rx[1] - rx[0], uy = ry[1] - ry[0], vx = rx[2] - rx[0], vy = ry[2] - ry[0];
    float det = ux * vy - vx * uy;
    if (fabsf(det) < 10000) {
      putCenter(160, 160, "Repite las tres cruces", DEAR);
      delay(1000);
      continue;
    }
    ax = ((sx[1] - sx[0]) * vy - (sx[2] - sx[0]) * uy) / det;
    bx = (ux * (sx[2] - sx[0]) - vx * (sx[1] - sx[0])) / det;
    cx = sx[0] - ax * rx[0] - bx * ry[0];
    ay = ((sy[1] - sy[0]) * vy - (sy[2] - sy[0]) * uy) / det;
    by = (ux * (sy[2] - sy[0]) - vx * (sy[1] - sy[0])) / det;
    cy = sy[0] - ay * rx[0] - by * ry[0];
    prefs.putFloat("ax", ax); prefs.putFloat("bx", bx); prefs.putFloat("cx", cx);
    prefs.putFloat("ay", ay); prefs.putFloat("by", by); prefs.putFloat("cy", cy);
    prefs.putBool("cal", true);
    return;
  }
}

// ---------- Fechas, precios y memoria ----------
bool now(struct tm& t) { return getLocalTime(&t, 50); }
int ymdOf(const struct tm& t) { return (t.tm_year + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday; }

int previousDate(struct tm t) {
  t.tm_mday--; t.tm_hour = 12; t.tm_min = t.tm_sec = 0; t.tm_isdst = -1;
  mktime(&t);
  return ymdOf(t);
}

bool digits(const String& s, unsigned maxLen) {
  if (s.isEmpty() || s.length() > maxLen) return false;
  for (char c : s) if (c < '0' || c > '9') return false;
  return s.toInt() > 0;
}

// Milésimas de euro: evitar redondeos al ordenar y sumar.
bool parsePrice(const char* src, int& mills) {
  if (!src || !*src) return false;
  int whole = 0, fraction = 0, decimals = 0, n = 0;
  while (*src >= '0' && *src <= '9') {
    if (++n > 2) return false;
    whole = whole * 10 + *src++ - '0';
  }
  if (!n || *src++ != ',') return false;
  while (*src >= '0' && *src <= '9') {
    if (++decimals > 3) return false;
    fraction = fraction * 10 + *src++ - '0';
  }
  if (*src || !decimals) return false;
  while (decimals++ < 3) fraction *= 10;
  mills = whole * 1000 + fraction;
  return mills > 0;
}

void addStation(Summary& d, const Station& station) {
  d.sum += station.mills;
  if (!d.count || station.mills < d.lo) d.lo = station.mills;
  if (!d.count || station.mills > d.hi) d.hi = station.mills;
  int n = min((uint32_t)5, d.count), at = 0;
  while (at < n && d.best[at].mills <= station.mills) at++;
  if (at < 5) {
    for (int j = min(n, 4); j > at; j--) d.best[j] = d.best[j - 1];
    d.best[at] = station;
  }
  d.count++;
}

float average(const Summary& d) { return d.count ? (float)d.sum / d.count / 1000.0f : 0; }

bool parseStamp(const char* s, Summary& d) {
  int day, month, year, hour, minute, second, used = 0;
  if (sscanf(s, "%d/%d/%d %d:%d:%d%n", &day, &month, &year, &hour, &minute, &second, &used) != 6 ||
      s[used] || year < 2020 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31 ||
      hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) return false;
  struct tm t = {};
  t.tm_year = year - 1900; t.tm_mon = month - 1; t.tm_mday = day;
  t.tm_hour = 12; t.tm_isdst = -1;
  if (mktime(&t) == -1 || t.tm_mday != day || t.tm_mon != month - 1) return false;
  d.date = year * 10000 + month * 100 + day;
  strlcpy(d.stamp, s, sizeof(d.stamp));
  snprintf(d.clock, sizeof(d.clock), "%02d:%02d", hour, minute);
  return true;
}

void resetHistory() {
  history = History();
  strlcpy(history.town, config.town, sizeof(history.town));
  history.fuel = config.fuel;
}

bool saveHistory() { return prefs.putBytes("history", &history, sizeof(history)) == sizeof(history); }

void recordHistory() {
  struct tm t;
  if (!now(t)) return;
  struct tm cutoff = t;
  cutoff.tm_mday -= 13; cutoff.tm_hour = 12; cutoff.tm_isdst = -1;
  mktime(&cutoff);
  int n = 0;
  for (int i = 0; i < history.count; i++)
    if (history.days[i].date >= ymdOf(cutoff) && history.days[i].date <= ymdOf(t))
      history.days[n++] = history.days[i];
  bool trimmed = n != history.count;
  history.count = n;
  if (!data.count || t.tm_hour < 8 || data.date != ymdOf(t)) {
    if (trimmed && !saveHistory()) strlcpy(lastError, "No se pudo guardar la tendencia", sizeof(lastError));
    return;
  }
  Sample sample;
  sample.date = data.date; sample.lo = data.lo; sample.avg = average(data);
  if (n && history.days[n - 1].date == data.date) history.days[n - 1] = sample;
  else {
    if (n == 14) {
      for (int i = 1; i < 14; i++) history.days[i - 1] = history.days[i];
      n = 13;
    }
    history.days[n++] = sample;
    history.count = n;
  }
  if (!saveHistory()) strlcpy(lastError, "No se pudo guardar la tendencia", sizeof(lastError));
}

// Ayer natural, sin sustituirlo por otro día si falta.
bool yesterdayMin(int& mills) {
  struct tm t;
  if (!now(t) || data.date != ymdOf(t)) return false;
  int yesterday = previousDate(t);
  for (int i = 0; i < history.count; i++)
    if (history.days[i].date == yesterday) { mills = history.days[i].lo; return true; }
  return false;
}

void applyLed() {
  if (lastError[0]) { setLed((millis() / 400) % 2, false, false); return; }
  if (!data.count) { setLed(false, false, false); return; }
  int yesterday;
  bool dearer = yesterdayMin(yesterday) && data.lo > yesterday;
  setLed(dearer, true, false);
}

// ---------- Distancias y provincia aproximada (capital más cercana, no límites) ----------
struct Capital { uint8_t id; float lat, lon; };
// Coordenadas urbanas redondeadas. IDs INE/Ministerio 01..52, incluidas Ceuta y Melilla.
// Referencia geográfica: IGN, Anuario 2025, «Posiciones geográficas».
const Capital CAPITALS[] = {
  {1,42.8467f,-2.6731f},   // Vitoria-Gasteiz
  {2,38.9943f,-1.8585f},   // Albacete
  {3,38.3452f,-0.4810f},   // Alicante
  {4,36.8381f,-2.4597f},   // Almería
  {5,40.6567f,-4.6812f},   // Ávila
  {6,38.8794f,-6.9707f},   // Badajoz
  {7,39.5696f,2.6502f},    // Palma
  {8,41.3874f,2.1686f},    // Barcelona
  {9,42.3439f,-3.6969f},   // Burgos
  {10,39.4753f,-6.3724f},  // Cáceres
  {11,36.5271f,-6.2886f},  // Cádiz
  {12,39.9864f,-0.0513f},  // Castelló de la Plana
  {13,38.9848f,-3.9274f},  // Ciudad Real
  {14,37.8882f,-4.7794f},  // Córdoba
  {15,43.3623f,-8.4115f},  // A Coruña
  {16,40.0704f,-2.1374f},  // Cuenca
  {17,41.9794f,2.8214f},   // Girona
  {18,37.1773f,-3.5986f},  // Granada
  {19,40.6330f,-3.1669f},  // Guadalajara
  {20,43.3183f,-1.9812f},  // Donostia / San Sebastián
  {21,37.2614f,-6.9447f},  // Huelva
  {22,42.1401f,-0.4089f},  // Huesca
  {23,37.7796f,-3.7849f},  // Jaén
  {24,42.5987f,-5.5671f},  // León
  {25,41.6176f,0.6200f},   // Lleida
  {26,42.4627f,-2.4449f},  // Logroño
  {27,43.0097f,-7.5568f},  // Lugo
  {28,40.4168f,-3.7038f},  // Madrid
  {29,36.7213f,-4.4214f},  // Málaga
  {30,37.9922f,-1.1307f},  // Murcia
  {31,42.8125f,-1.6458f},  // Pamplona / Iruña
  {32,42.3358f,-7.8639f},  // Ourense
  {33,43.3619f,-5.8494f},  // Oviedo
  {34,42.0096f,-4.5288f},  // Palencia
  {35,28.1235f,-15.4363f}, // Las Palmas de Gran Canaria
  {36,42.4310f,-8.6444f},  // Pontevedra
  {37,40.9701f,-5.6635f},  // Salamanca
  {38,28.4636f,-16.2518f}, // Santa Cruz de Tenerife
  {39,43.4623f,-3.8100f},  // Santander
  {40,40.9429f,-4.1088f},  // Segovia
  {41,37.3891f,-5.9845f},  // Sevilla
  {42,41.7666f,-2.4790f},  // Soria
  {43,41.1189f,1.2445f},   // Tarragona
  {44,40.3457f,-1.1065f},  // Teruel
  {45,39.8628f,-4.0273f},  // Toledo
  {46,39.4699f,-0.3763f},  // València
  {47,41.6523f,-4.7245f},  // Valladolid
  {48,43.2630f,-2.9350f},  // Bilbao
  {49,41.5035f,-5.7444f},  // Zamora
  {50,41.6488f,-0.8891f},  // Zaragoza
  {51,35.8894f,-5.3213f},  // Ceuta
  {52,35.2923f,-2.9381f}   // Melilla
};

float distanceKm(float lat1, float lon1, float lat2, float lon2) {
  constexpr float RAD = 0.017453292519943295f;
  float a = sinf((lat2 - lat1) * RAD / 2), b = sinf((lon2 - lon1) * RAD / 2);
  float h = a * a + cosf(lat1 * RAD) * cosf(lat2 * RAD) * b * b;
  return 12742.0f * asinf(sqrtf(constrain(h, 0.0f, 1.0f)));
}

uint8_t nearestProvince(float lat, float lon) {
  float best = 1e9f;
  uint8_t id = 0;
  for (const Capital& c : CAPITALS) {
    float d = distanceKm(lat, lon, c.lat, c.lon);
    if (d < best) { best = d; id = c.id; }
  }
  return id;
}

bool parseCoordinate(const char* src, float limit, float& out) {
  if (!src || !*src || strlen(src) >= 24) return false;
  char buf[24]; strlcpy(buf, src, sizeof(buf));
  char* p = buf;
  if (*p == '-' || *p == '+') p++;
  bool digit = false, separator = false;
  for (; *p; p++) {
    if (*p >= '0' && *p <= '9') { digit = true; continue; }
    if ((*p == ',' || *p == '.') && digit && !separator && p[1] >= '0' && p[1] <= '9') {
      separator = true; *p = '.'; continue;
    }
    return false;
  }
  out = strtof(buf, nullptr);
  return digit && isfinite(out) && fabsf(out) <= limit;
}

// No cortar un carácter UTF-8 por la mitad al limitar los campos compactos.
void compactText(char* dst, size_t size, const char* src) {
  size_t n = min(strlen(src), size - 1);
  if (src[n]) while (n && (((uint8_t)src[n] & 0xC0) == 0x80)) n--;
  memcpy(dst, src, n); dst[n] = 0;
}

bool usableAP(const uint8_t* mac, const uint8_t* connected, const String& ssid) {
  if (!mac || (mac[0] & 0x03) || !memcmp(mac, connected, 6) || ssid.endsWith("_nomap")) return false;
  uint8_t any = 0;
  for (int i = 0; i < 6; i++) any |= mac[i];
  return any != 0;
}

// ---------- HTTPS y lectura incremental ----------
// Los listados se leen enteros. Dejarles RAM para cuerpo + JSON + TLS; CERCA se recupera después.
void freeNear() {
  struct tm t;
  if (!nearStations || WiFi.status() != WL_CONNECTED || !now(t)) return;
  delete[] nearStations; nearStations = nullptr; nearCount = 0; provinceValid = false;
  provinceWanted = nearEnabled;
}

bool openUrl(HTTPClient& http, WiFiClientSecure& client, const char* ca, const String& url, const char* who,
             String& error) {
  struct tm t;
  if (WiFi.status() != WL_CONNECTED) { error = "Sin conexión Wi-Fi"; return false; }
  if (!now(t)) { error = "Sin hora: espera y reintenta"; return false; }
  client.setCACert(ca);
  client.setTimeout(15000);
  client.setHandshakeTimeout(15);
  http.useHTTP10(true);  // El flujo recibe JSON sin codificación chunked.
  http.setReuse(false);
  http.setTimeout(15000);
  http.setConnectTimeout(10000);
  if (!http.begin(client, url)) { error = "No se pudo abrir HTTPS"; return false; }
  http.setUserAgent("Gasolina-MS/1.0 (CYD)");
  http.addHeader("Accept", "application/json");
  const char* headers[] = {"Transfer-Encoding"};
  http.collectHeaders(headers, 1);
  int code = http.GET();
  if (code != HTTP_CODE_OK) { error = String(who) + ": error HTTPS " + String(code); return false; }
  if (http.header("Transfer-Encoding").length()) { error = "Formato HTTP no admitido"; return false; }
  http.getStream().setTimeout(15000);
  return true;
}

bool beginApi(HTTPClient& http, WiFiClientSecure& client, const String& path, String& error) {
  if (path.startsWith("/Listados/")) freeNear();
  return openUrl(http, client, ROOT_CA, String(BASE_URL) + path, "Ministerio", error);
}

// Lee la conexión segura a trozos de 1 kB: leer byte a byte tardaba ~30 s con Madrid y dejaba
// la página de ajustes sin responder mientras tanto.
class BufferedInput : public Stream {
 public:
  explicit BufferedInput(Client& c) : client(c) {}
  int available() override { return pos < len ? len - pos : client.available(); }
  int read() override { return fill() ? buf[pos++] : -1; }
  int peek() override { return fill() ? buf[pos] : -1; }
  size_t write(uint8_t) override { return 0; }
 private:
  bool fill() {
    if (pos < len) return true;
    uint32_t start = millis();
    while (millis() - start < 15000) {
      int got = client.read(buf, sizeof(buf));
      if (got > 0) { len = got; pos = 0; return true; }
      if (!client.connected() && !client.available()) return false;
      delay(1);
    }
    return false;
  }
  Client& client;
  uint8_t buf[1024];
  int len = 0, pos = 0;
};

int nextChar(Stream& input) {
  char c;
  while (input.readBytes(&c, 1) == 1) {
    if (c != ' ' && c != '\r' && c != '\n' && c != '\t') return (uint8_t)c;
  }
  return -1;
}

int peekChar(Stream& input) {
  while (true) {
    int c = input.peek();
    if (c == -1) {
      uint32_t start = millis();
      while ((c = input.peek()) == -1 && millis() - start < 15000) delay(1);
      if (c == -1) return -1;
    }
    if (c != ' ' && c != '\r' && c != '\n' && c != '\t') return c;
    input.read();
  }
}

bool arrayStart(Stream& input) {
  int c = nextChar(input);
  if (c == 0xEF) {  // BOM UTF-8 opcional.
    if (nextChar(input) != 0xBB || nextChar(input) != 0xBF) return false;
    c = nextChar(input);
  }
  return c == '[';
}

bool readPrices(Stream& input, Summary& out, String& error, bool provincial = false) {
  int c = nextChar(input);
  if (c == 0xEF) {
    if (nextChar(input) != 0xBB || nextChar(input) != 0xBF) return false;
    c = nextChar(input);
  }
  if (c != '{') { error = "Respuesta de precios no válida"; return false; }
  JsonDocument filter, key, value, item, discard;
  filter["Rótulo"] = true;
  filter["Dirección"] = true;
  filter["PrecioProducto"] = true;
  filter["Tipo Venta"] = true;
  filter["Latitud"] = true; filter["Longitud (WGS84)"] = true;
  discard.set(false);
  double sumLat = 0, sumLon = 0;
  int coords = 0;
  bool haveArray = false, haveDate = false, haveResult = false;
  while (true) {
    if (peekChar(input) != '"' || deserializeJson(key, input) || !key.is<const char*>()) break;
    if (nextChar(input) != ':') break;
    const char* name = key.as<const char*>();
    if (!strcmp(name, "ListaEESSPrecio")) {
      if (haveArray || nextChar(input) != '[') break;
      haveArray = true;
      if (peekChar(input) == ']') input.read();
      else while (true) {
        if (peekChar(input) != '{' || deserializeJson(item, input, DeserializationOption::Filter(filter)) ||
            !item.is<JsonObject>()) { error = "Datos incompletos: reintenta"; return false; }
        Station station;
        if (!strcmp(item["Tipo Venta"] | "", "P") && parsePrice(item["PrecioProducto"] | "", station.mills)) {
          if (provincial) {
            float lat, lon;
            if (parseCoordinate(item["Latitud"] | "", 90, lat) &&
                parseCoordinate(item["Longitud (WGS84)"] | "", 180, lon) && (lat != 0 || lon != 0)) {
              if (nearCount == nearCapacity) { error = "Provincia supera 1200 estaciones"; return false; }
              NearStation& s = nearStations[nearCount++];
              s.lat = lat; s.lon = lon; s.mills = station.mills;
              compactText(s.brand, sizeof(s.brand), item["Rótulo"] | "Sin rótulo");
              compactText(s.address, sizeof(s.address), item["Dirección"] | "Sin dirección");
            }
          } else {
            float lat, lon;
            if (parseCoordinate(item["Latitud"] | "", 90, lat) &&
                parseCoordinate(item["Longitud (WGS84)"] | "", 180, lon) && (lat != 0 || lon != 0)) {
              sumLat += lat; sumLon += lon; coords++;
            }
            strlcpy(station.brand, item["Rótulo"] | "Sin rótulo", sizeof(station.brand));
            strlcpy(station.address, item["Dirección"] | "Sin dirección", sizeof(station.address));
            addStation(out, station);
          }
        }
        c = nextChar(input);
        if (c == ']') break;
        if (c != ',') { error = "Lista de precios incompleta"; return false; }
      }
    } else if (!strcmp(name, "Fecha") || !strcmp(name, "ResultadoConsulta")) {
      if (deserializeJson(value, input) || !value.is<const char*>()) break;
      if (!strcmp(name, "Fecha")) {
        if (haveDate || !parseStamp(value.as<const char*>(), out)) break;
        haveDate = true;
      } else {
        if (haveResult || strcmp(value.as<const char*>(), "OK")) break;
        haveResult = true;
      }
    } else if (deserializeJson(value, input, DeserializationOption::Filter(discard))) break;
    c = nextChar(input);
    if (c == '}') {
      if (haveArray && haveDate && haveResult) {
        if (!provincial) { townLat = coords ? sumLat / coords : 0; townLon = coords ? sumLon / coords : 0; }
        return true;
      }
      break;
    }
    if (c != ',') break;
  }
  error = "Respuesta del Ministerio incompleta";
  return false;
}

// Listados: se descargan enteros (40 kB como mucho, Barcelona) porque leerlos letra a letra por TLS
// tardaba ~25 s y bloqueaba la página de ajustes. Si `out` no es nulo, deja ahí los elementos en JSON.
bool readList(const String& province, const String& wanted, String& found, String& error, String* out = nullptr) {
  WiFiClientSecure client;
  HTTPClient http;
  String path = province.isEmpty() ? "/Listados/Provincias/" : "/Listados/MunicipiosPorProvincia/" + province;
  if (!beginApi(http, client, path, error)) { http.end(); return false; }
  String body = http.getString();
  http.end();
  if (body.startsWith("\xEF\xBB\xBF")) body.remove(0, 3);  // BOM UTF-8 opcional.
  const char* idKey = province.isEmpty() ? "IDPovincia" : "IDMunicipio";
  const char* nameKey = province.isEmpty() ? "Provincia" : "Municipio";
  JsonDocument filter, doc;
  filter[0][idKey] = true; filter[0][nameKey] = true;
  bool ok = !deserializeJson(doc, body, DeserializationOption::Filter(filter)) && doc.is<JsonArray>();
  body = String();
  if (ok && out) *out = "";
  for (JsonObject item : doc.as<JsonArray>()) {
    if (!ok) break;
    const char* id = item[idKey] | "", *name = item[nameKey] | "";
    if (!digits(id, province.isEmpty() ? 2 : 6) || !*name || strlen(name) > 99) { ok = false; break; }
    if (wanted == id) found = name;
    if (out) {
      JsonDocument row; row["id"] = id; row["nombre"] = name;
      String one;  // serializeJson sustituye el contenido de un String: se añade aparte.
      serializeJson(row, one);
      if (out->length()) *out += ',';
      *out += one;
    }
  }
  if (!ok) error = "No se pudo leer la lista oficial";
  return ok;
}

// ---------- Francia y Austria: respuestas pequeñas (≤100 estaciones), filtradas enteras ----------
void stampNow(Summary& d) {
  struct tm t;
  if (!now(t)) return;
  d.date = ymdOf(t);
  strftime(d.stamp, sizeof(d.stamp), "%d/%m/%Y %H:%M:%S", &t);
  strftime(d.clock, sizeof(d.clock), "%H:%M", &t);
}

// near: radio de 20 km ordenado por distancia (Francia) o búsqueda por punto (Austria, ~10 más baratas).
String foreignUrl(bool near) {
  char point[64];
  if (country == 1) {
    const char* f = FR_FIELDS[config.fuel];
    String url = String(FR_BASE) + "?limit=100&select=adresse,ville,cp,geom," + f + "&where=";
    if (near) {
      snprintf(point, sizeof(point), "geom%%27POINT(%.5f%%20%.5f)%%27", positionLon, positionLat);
      return url + "within_distance(geom%2C" + point + "%2C20km)%20and%20" + f +
             "%20is%20not%20null&order_by=distance(geom%2C" + point + ")";
    }
    return url + "cp%3D%22" + config.town + "%22%20and%20" + f + "%20is%20not%20null&order_by=" + f;
  }
  String tail = String("&fuelType=") + AT_CODES[config.fuel] + "&includeClosed=false";
  if (!near) return String(AT_BASE) + "/search/gas-stations/by-region?type=PB&code=" + config.town + tail;
  snprintf(point, sizeof(point), "latitude=%.5f&longitude=%.5f", positionLat, positionLon);
  return String(AT_BASE) + "/search/gas-stations/by-address?" + point + tail;
}

bool fetchForeign(Summary& out, String& error, bool near) {
  WiFiClientSecure client;
  HTTPClient http;
  bool ok = openUrl(http, client, country == 1 ? FR_CA : AT_CA, foreignUrl(near), SOURCE[country], error);
  JsonDocument filter, doc;
  if (country == 1) {
    JsonObject f = filter["results"][0].to<JsonObject>();
    f["adresse"] = true; f["ville"] = true; f["cp"] = true; f["geom"] = true; f[FR_FIELDS[config.fuel]] = true;
    filter["total_count"] = true;
  } else {
    JsonObject f = filter[0].to<JsonObject>();
    f["name"] = true; f["location"]["address"] = true; f["location"]["city"] = true;
    f["location"]["latitude"] = true; f["location"]["longitude"] = true; f["prices"][0]["amount"] = true;
  }
  if (ok) {
    BufferedInput input(http.getStream()); input.setTimeout(15000);
    ok = !deserializeJson(doc, input, DeserializationOption::Filter(filter));
  }
  http.end();
  JsonArray list = country == 1 ? doc["results"].as<JsonArray>() : doc.as<JsonArray>();
  if (ok && list.isNull()) ok = false;
  if (!ok) { if (error.isEmpty()) error = "Respuesta de precios no válida"; return false; }
  double sumLat = 0, sumLon = 0;
  int coords = 0;
  for (JsonObject s : list) {
    float p = country == 1 ? s[FR_FIELDS[config.fuel]] | 0.0f : s["prices"][0]["amount"] | 0.0f;
    float lat = country == 1 ? s["geom"]["lat"] | 0.0f : s["location"]["latitude"] | 0.0f;
    float lon = country == 1 ? s["geom"]["lon"] | 0.0f : s["location"]["longitude"] | 0.0f;
    if (!(p > 0.2f && p < 10.0f)) continue;  // null o fuera de rango: sin precio público.
    Station st;
    st.mills = lroundf(p * 1000);
    if (country == 1) {
      compactText(st.brand, sizeof(st.brand), s["adresse"] | "Sin dirección");
      snprintf(st.address, sizeof(st.address), "%s %s", (const char*)(s["cp"] | ""), (const char*)(s["ville"] | ""));
    } else {
      compactText(st.brand, sizeof(st.brand), s["name"] | "Sin nombre");
      snprintf(st.address, sizeof(st.address), "%s, %s", (const char*)(s["location"]["address"] | ""),
               (const char*)(s["location"]["city"] | ""));
    }
    bool hasPos = (lat != 0 || lon != 0) && fabsf(lat) <= 90 && fabsf(lon) <= 180;
    if (near) {
      if (!hasPos || nearCount == nearCapacity) continue;
      NearStation& n = nearStations[nearCount++];
      n.lat = lat; n.lon = lon; n.mills = st.mills;
      compactText(n.brand, sizeof(n.brand), st.brand);
      compactText(n.address, sizeof(n.address), st.address);
    } else {
      addStation(out, st);
      if (hasPos) { sumLat += lat; sumLon += lon; coords++; }
    }
  }
  if (!near) { townLat = coords ? sumLat / coords : 0; townLon = coords ? sumLon / coords : 0; }
  else {
    nearCompleteKm = 1e9f;
    if (country == 1 && (doc["total_count"] | 0) > (int)list.size() && nearCount)
      nearCompleteKm = distanceKm(positionLat, positionLon, nearStations[nearCount - 1].lat, nearStations[nearCount - 1].lon);
  }
  stampNow(out);
  return true;
}

void refreshPrices() {
  refreshWanted = false;
  if (!config.town[0] || busy) return;
  busy = true;
  lastFetchTry = millis();
  Summary candidate;
  String error;
  bool ok;
  if (country) ok = fetchForeign(candidate, error, false);
  else {
    WiFiClientSecure client;
    HTTPClient http;
    String path = "/EstacionesTerrestres/FiltroMunicipioProducto/" + String(config.town) + "/" + PRODUCT_IDS[config.fuel];
    ok = beginApi(http, client, path, error);
    if (ok) { BufferedInput input(http.getStream()); input.setTimeout(15000); ok = readPrices(input, candidate, error); }
    http.end();
  }
  if (ok) {
    data = candidate; fetchedAt = millis(); lastError[0] = 0;
    recordHistory();
  } else strlcpy(lastError, error.isEmpty() ? "Error al leer precios: reintenta" : error.c_str(), sizeof(lastError));
  // Una línea por intento; el error no se confunde con cifras recién descargadas.
  if (country) Serial.printf("GASOLINA pais=%s zona=%s producto=%s ", country == 1 ? "FR" : "AT", config.town,
                            country == 1 ? FR_FIELDS[config.fuel] : AT_CODES[config.fuel]);
  else Serial.printf("GASOLINA municipio=%s producto=%u ", config.town, PRODUCT_IDS[config.fuel]);
  Serial.printf("n=%lu min=%.3f media=%.3f hora=%s estado=%s\n", (unsigned long)(ok ? data.count : 0),
                ok ? data.lo / 1000.0f : 0.0f, ok ? average(data) : 0.0f,
                ok ? data.stamp : "--", ok ? "OK" : "ERROR");
  busy = false;
  applyLed();
}

// ---------- Cerca de mí: sin GPS, sin posición persistente ni datos privados en serie ----------
bool locateByWifi() {
  struct tm t;
  if (WiFi.status() != WL_CONNECTED || !now(t)) return false;
  uint8_t connected[6];
  const uint8_t* current = WiFi.BSSID();
  if (!current) return false;
  memcpy(connected, current, 6);
  struct AP { uint8_t mac[6]; int32_t rssi; } strongest[15];
  int used = 0;
  // Síncrono, incluidas redes ocultas. No desconectar del hotspot para escanear.
  int count = WiFi.scanNetworks(false, true);
  current = WiFi.BSSID();
  for (int i = 0; i < count; i++) {
    const uint8_t* mac = WiFi.BSSID(i);
    if (!usableAP(mac, connected, WiFi.SSID(i)) || (current && !memcmp(mac, current, 6))) continue;
    int32_t rssi = WiFi.RSSI(i);
    int duplicate = -1;
    for (int j = 0; j < used; j++) if (!memcmp(mac, strongest[j].mac, 6)) duplicate = j;
    if (duplicate >= 0) {
      if (strongest[duplicate].rssi >= rssi) continue;
      for (int j = duplicate; j < used - 1; j++) strongest[j] = strongest[j + 1];
      used--;
    }
    int at = 0;
    while (at < used && strongest[at].rssi >= rssi) at++;
    if (at >= 15) continue;
    for (int j = min(used, 14); j > at; j--) strongest[j] = strongest[j - 1];
    memcpy(strongest[at].mac, mac, 6); strongest[at].rssi = rssi;
    used = min(used + 1, 15);
  }
  WiFi.scanDelete();
  if (!used || WiFi.status() != WL_CONNECTED) return false;
  JsonDocument request;
  request["considerIp"] = false;
  JsonArray points = request["wifiAccessPoints"].to<JsonArray>();
  for (int i = 0; i < used; i++) {
    char mac[18];
    const uint8_t* m = strongest[i].mac;
    snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4], m[5]);
    JsonObject ap = points.add<JsonObject>();
    ap["macAddress"] = mac; ap["signalStrength"] = strongest[i].rssi;
  }
  String body; serializeJson(request, body);
  request.clear();
  WiFiClientSecure client;
  HTTPClient http;
  client.setCACert(BEACON_CA);  // X2 + YE reales concatenadas; nunca setInsecure.
  client.setTimeout(15000); client.setHandshakeTimeout(15);
  http.useHTTP10(true); http.setReuse(false);
  http.setTimeout(15000); http.setConnectTimeout(10000);
  if (!http.begin(client, "https://api.beacondb.net/v1/geolocate")) return false;
  http.setUserAgent("Gasolina-MS/1.0 (CYD; github.com/msrobotika/AB-MakerIDE-Projects)");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  const char* headers[] = {"Transfer-Encoding"};
  http.collectHeaders(headers, 1);
  int code = http.POST(body);
  body = String();
  beaconCode = code; beaconNets = used;
  // 404 significa que estas redes no permiten una estimación. No recurrir a la IP.
  if (code != HTTP_CODE_OK || http.header("Transfer-Encoding").length()) { http.end(); return false; }
  JsonDocument filter, response;
  filter["location"]["lat"] = true; filter["location"]["lng"] = true; filter["accuracy"] = true;
  BufferedInput input(http.getStream()); input.setTimeout(15000);
  bool ok = !deserializeJson(response, input, DeserializationOption::Filter(filter)) &&
            response["location"]["lat"].is<float>() && response["location"]["lng"].is<float>() &&
            response["accuracy"].is<float>();
  http.end();
  if (!ok) return false;
  float lat = response["location"]["lat"], lon = response["location"]["lng"], accuracy = response["accuracy"];
  if (!isfinite(lat) || !isfinite(lon) || !isfinite(accuracy) || fabsf(lat) > 90 || fabsf(lon) > 180 ||
      accuracy <= 0 || accuracy > 20000000) return false;
  positionLat = lat; positionLon = lon; positionAccuracy = accuracy;
  return true;
}

void refreshPosition() {
  positionWanted = false; lastPositionTry = millis();
  bool wasKnown = havePosition;
  beaconCode = beaconNets = 0;
  bool located = locateByWifi();
  if (beaconNets) Serial.printf("GASOLINA beacondb codigo=%d redes=%d\n", beaconCode, beaconNets);
  if (located) { positionFailures = 0; positionTown = false; }
  else {
    Serial.println("sin estimacion");
    if (positionFailures < 3) positionFailures++;
    // Sin ubicación Wi-Fi (o la anterior ya caducó): distancias desde el centro del pueblo.
    if ((townLat || townLon) && (!havePosition || positionTown || positionFailures >= 3)) {
      positionLat = townLat; positionLon = townLon; positionAccuracy = 0;
      located = positionTown = true;
    }
  }
  if (located) {
    havePosition = true; positionOld = false;
    uint8_t province = country ? 0 : nearestProvince(positionLat, positionLon);
    if (country && provinceValid && distanceKm(positionLat, positionLon, fetchLat, fetchLon) > 2) provinceWanted = true;
    if (province != positionProvince) {
      positionProvince = province; provinceValid = false; nearCount = 0; provinceWanted = true;
      nearError[0] = 0;
    }
    if (!wasKnown && !positionTown) screen = S_NEAR;
    Serial.printf("GASOLINA posicion ok %s\n", positionTown ? "centro del pueblo" : "por Wi-Fi");
  } else {
    positionOld = havePosition;
    if (positionFailures >= 3) {
      havePosition = false; positionOld = false;
      positionLat = positionLon = positionAccuracy = 0;
      if (screen == S_NEAR) screen = S_NOW;
    }
  }
}

void refreshProvince() {
  provinceWanted = false; lastProvinceTry = millis();
  // La caché única nunca se consulta mientras se rellena o tras una lectura parcial.
  provinceValid = false; nearCount = 0;
  uint16_t want = country ? 100 : MAX_NEAR_STATIONS;
  if (nearStations && nearCapacity != want) { delete[] nearStations; nearStations = nullptr; }
  if (!nearStations) { nearStations = new (std::nothrow) NearStation[want]; nearCapacity = nearStations ? want : 0; }
  if (!nearStations) { strlcpy(nearError, "Sin memoria para la provincia", sizeof(nearError)); return; }
  String error;
  Summary candidate;
  bool ok;
  if (country) {
    ok = fetchForeign(candidate, error, true);
    fetchLat = positionLat; fetchLon = positionLon;
  } else {
    WiFiClientSecure client;
    HTTPClient http;
    char id[3]; snprintf(id, sizeof(id), "%02u", positionProvince);
    String path = "/EstacionesTerrestres/FiltroProvinciaProducto/" + String(id) + "/" + PRODUCT_IDS[config.fuel];
    ok = beginApi(http, client, path, error);
    if (ok) { BufferedInput input(http.getStream()); input.setTimeout(15000); ok = readPrices(input, candidate, error, true); }
    http.end();
  }
  if (ok) {
    provinceMeta = candidate; cachedProvince = positionProvince; cachedFuel = config.fuel;
    provinceFetchedAt = millis(); provinceValid = true; nearError[0] = 0;
  } else {
    nearCount = 0;
    strlcpy(nearError, error.isEmpty() ? "No se pudo leer la provincia" : error.c_str(), sizeof(nearError));
  }
}

NearChoice chooseNear() {
  NearChoice out;
  if (!havePosition || !provinceValid || !nearCount || cachedProvince != positionProvince || cachedFuel != config.fuel) return out;
  float closest = 1e9f;
  for (int i = 0; i < nearCount; i++) {
    float d = distanceKm(positionLat, positionLon, nearStations[i].lat, nearStations[i].lon);
    if (d < closest) { closest = d; out.closest = i; out.closeKm = d; }
  }
  for (int radius : {5, 10, 20}) {
    out.radius = radius;
    for (int i = 0; i < nearCount; i++) {
      float d = distanceKm(positionLat, positionLon, nearStations[i].lat, nearStations[i].lon);
      if (d <= radius && (out.cheapest < 0 || nearStations[i].mills < nearStations[out.cheapest].mills ||
          (nearStations[i].mills == nearStations[out.cheapest].mills && d < out.cheapKm))) {
        out.cheapest = i; out.cheapKm = d;
      }
    }
    if (out.cheapest >= 0) break;
  }
  return out;
}

// ---------- Marco y texto corto ----------
String shortText(const char* src, int columns) {
  String out;
  bool truncated = textWidth(src, 1) / 6 > columns;
  int limit = truncated ? max(0, columns - 3) : columns;
  const uint8_t* p = (const uint8_t*)src;
  int n = 0;
  while (*p && n < limit) {
    const uint8_t* start = p;
    bool euroMark;
    mapUtf8(p, euroMark);
    while (start < p) out += (char)*start++;
    n++;
  }
  if (truncated) out += "...";
  return out;
}

void putShort(int x, int y, const char* s, int columns, uint16_t color, int size = 1) {
  String text = shortText(s, columns);
  put(x, y, text.c_str(), color, size);
}

void wifiIcon(int x, int y) {
  int bars = 0;
  if (WiFi.status() == WL_CONNECTED) {
    long r = WiFi.RSSI();
    bars = r > -60 ? 4 : r > -70 ? 3 : r > -80 ? 2 : 1;
  }
  for (int i = 0; i < 4; i++) {
    int h = 3 + i * 3;
    gfx->fillRect(x + i * 5, y + 12 - h, 3, h, i < bars ? TXT : LINE);
  }
}

void drawHeader() {
  gfx->fillRect(0, 0, W, 26, PANEL);
  gfx->fillRoundRect(8, 5, 13, 16, 2, MID);
  gfx->fillRect(10, 7, 9, 5, BG);  // surtidor
  gfx->drawLine(21, 9, 24, 12, MID); gfx->drawLine(24, 12, 24, 19, MID);
  put(28, 9, "GASOLINA MS", TXT, 1, PANEL);
  put(105, 9, SCREEN_NAMES[screen], ACC, 1, PANEL);
  struct tm t;
  char clock[8] = "--:--";
  if (now(t)) strftime(clock, sizeof(clock), "%H:%M", &t);
  putRight(286, 6, clock, TXT, 2, PANEL);
  wifiIcon(294, 7);
}

void drawFooter() {
  gfx->fillRect(0, 198, W, 42, BG);
  if (!portalMode && nearEnabled && positionTown && screen == S_NEAR)
    putCenter(160, 199, "Sin ubicación Wi-Fi: distancias desde tu pueblo", MID);
  else if (!portalMode && nearEnabled && positionFailures >= 3 && !havePosition)
    putCenter(160, 199, "Sin ubicación: pocas redes Wi-Fi aquí", MID);
  else if (!portalMode && screen == S_NEAR && havePosition) {
    char line[96];
    snprintf(line, sizeof(line), "Ubicación aproximada por redes Wi-Fi (±%.0f m)", positionAccuracy);
    putCenter(160, 199, line, DIM);
  } else if (country) {
    char line[64]; snprintf(line, sizeof(line), "Precios oficiales: %s", SOURCE[country]);
    putCenter(160, 199, line, DIM);
  } else putCenter(160, 199, "Precios oficiales del Ministerio (datos abiertos).", DIM);
  putCenter(160, 209, "Pueden tardar en actualizarse:", DIM);
  putCenter(160, 219, "confirma en el surtidor.", DIM);
  for (int i = 0; i < S_COUNT; i++) {
    int x = 160 - (S_COUNT * 14 - 4) / 2 + i * 14;
    if (i == screen) gfx->fillRoundRect(x, 232, 10, 5, 2, ACC);
    else gfx->fillCircle(x + 5, 234, 2, LINE);
  }
}

void clearBody() { gfx->fillRect(0, 26, W, 172, BG); }
bool inside(const Btn& b, int x, int y) { return x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h; }

void button(const Btn& b, const char* label, uint16_t color = TXT) {
  gfx->fillRoundRect(b.x, b.y, b.w, b.h, 5, PANEL);
  gfx->drawRoundRect(b.x, b.y, b.w, b.h, 5, LINE);
  putCenter(b.x + b.w / 2, b.y + (b.h - 8) / 2, label, color);
}

static int qrX = 0, qrY = 0, qrMax = 130;
void drawQr(esp_qrcode_handle_t q) {
  int size = esp_qrcode_get_size(q);
  int scale = max(1, qrMax / (size + 8));
  int total = (size + 8) * scale;
  gfx->fillRect(qrX, qrY, total, total, TXT);
  for (int y = 0; y < size; y++)
    for (int x = 0; x < size; x++)
      if (esp_qrcode_get_module(q, x, y))
        gfx->fillRect(qrX + (x + 4) * scale, qrY + (y + 4) * scale, scale, scale, INK);
}

void showQr(const String& text, int x, int y, int maxSize) {
  qrX = x; qrY = y; qrMax = maxSize;
  esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
  cfg.display_func = drawQr;
  cfg.max_qrcode_version = 6;
  if (esp_qrcode_generate(&cfg, text.c_str()) != ESP_OK) put(x, y, "QR no disponible", DEAR);
}

void settingsQr(int x, int y, int maxSize) {
  if (WiFi.status() == WL_CONNECTED) showQr(settingsUrl, x, y, maxSize);
  else put(x, y, "Sin Wi-Fi", DEAR);
}

bool showMissing() {
  if (!config.town[0]) {
    putCenter(160, 35, "Elige tu pueblo: escanea el QR", TXT);
    settingsQr(104, 54, 120);
    putCenter(160, 182, "Con el móvil en la misma red Wi-Fi", DIM);
    return true;
  }
  if (lastError[0]) {
    putCenter(160, 57, "No se pudieron actualizar los precios", DEAR);
    putShort(10, 82, lastError, 50, MID);
    putCenter(160, 107, "Comprueba la conexión y reintenta", DIM);
    if (data.count) putCenter(160, 125, "Se conserva la última descarga válida", DIM);
    button(B_RETRY, "Reintentar", ACC);
    return true;
  }
  if (!data.count) {
    putCenter(160, 66, fetchedAt ? "Sin precios públicos para esta selección" : "Buscando precios...", TXT);
    putCenter(160, 96, "Puedes cambiar pueblo o combustible en AJUSTES", DIM);
    button(B_RETRY, "Actualizar ahora", ACC);
    return true;
  }
  return false;
}

void dataTime(int y) {
  char line[80];
  struct tm t;
  bool today = now(t) && data.date == ymdOf(t);
  if (today && country) snprintf(line, sizeof(line), "Datos de %s · %s", SOURCE[country], data.clock);
  else if (today) snprintf(line, sizeof(line), "Datos del Ministerio · %s", data.clock);
  else snprintf(line, sizeof(line), "Datos anteriores · %s", data.stamp);
  putCenter(160, y, line, today ? DIM : MID);
}

// ---------- CERCA ----------
void drawNear() {
  if (!nearEnabled || !havePosition) {
    putCenter(160, 48, nearEnabled ? "Buscando ubicación por Wi-Fi" : "Ubicación: solo mi pueblo", ACC);
    if (positionFailures >= 3 && nearEnabled) {
      putCenter(160, 76, "Sin ubicación:", MID);
      putCenter(160, 91, "pocas redes Wi-Fi aquí", MID);
    } else putCenter(160, 85, "El pueblo sigue en AHORA y TOP 5", DIM);
    putCenter(160, 118, nearEnabled ? "Toca para volver a localizar" : "Activa el modo automático en AJUSTES", DIM);
    if (nearEnabled) button(B_RETRY, "Localizar ahora", ACC);
    return;
  }
  if (!provinceValid || cachedProvince != positionProvince || cachedFuel != config.fuel) {
    putCenter(160, 55, nearError[0] ? (country ? "Sin precios cercanos" : "Sin precios de la provincia")
                                    : "Buscando precios cercanos...", ACC);
    if (nearError[0]) putShort(10, 82, nearError, 50, MID);
    putCenter(160, 110, "Puedes consultar tu pueblo en AHORA", DIM);
    button(B_RETRY, "Actualizar ahora", ACC);
    return;
  }
  NearChoice choice = chooseNear();
  char line[112], v[20], distance[20];
  snprintf(line, sizeof(line), "Más barata · radio %d km%s", choice.radius, choice.radius > 5 ? " (ampliado)" : "");
  put(10, 32, line, ACC);
  if (choice.cheapest >= 0) {
    const NearStation& s = nearStations[choice.cheapest];
    gfx->fillRoundRect(8, 44, 304, 80, 8, PANEL);
    putShort(16, 49, s.brand, 24, TXT, 2);
    putShort(16, 71, s.address, 48, DIM);
    price(v, sizeof(v), s.mills / 1000.0f);
    gfx->setFont(&FreeSansBold24pt7b); gfx->setTextColor(CHEAP); gfx->setCursor(14, 117); gfx->print(v);
    gfx->setFont(); put(190, 99, "€/L", TXT);
    price(distance, sizeof(distance), choice.cheapKm, 1);
    snprintf(line, sizeof(line), "a %s km", distance); putRight(306, 113, line, DIM);
  } else {
    putCenter(160, 63, "Sin estaciones públicas en 20 km", TXT);
    putCenter(160, 90, "Solo se consulta la provincia aproximada", DIM);
  }
  if (choice.closest >= 0) {
    const NearStation& s = nearStations[choice.closest];
    snprintf(line, sizeof(line), "Más cercana: %s", s.brand); putShort(10, 131, line, 50, TXT);
    price(v, sizeof(v), s.mills / 1000.0f); price(distance, sizeof(distance), choice.closeKm, 1);
    snprintf(line, sizeof(line), "%s €/L · a %s km", v, distance); put(10, 144, line, DIM);
    if (choice.cheapest >= 0 && choice.cheapest != choice.closest) {
      price(v, sizeof(v), max(0, (int)s.mills - (int)nearStations[choice.cheapest].mills) * 0.05f, 2);
      snprintf(line, sizeof(line), "Ahorras %s € en 50 L", v); put(10, 159, line, CHEAP);
      put(10, 170, "frente a la más cercana", CHEAP);
    }
  } else putCenter(160, 139, "Sin precios públicos con coordenadas válidas", DIM);
  struct tm t;
  if (positionOld) strlcpy(line, "Posición anterior; reintentando cada 2 min", sizeof(line));
  else if (country == 1 && choice.radius > nearCompleteKm)
    snprintf(line, sizeof(line), "Solo las 100 más cercanas (hasta %.1f km)", nearCompleteKm);
  else if (now(t) && provinceMeta.date == ymdOf(t))
    snprintf(line, sizeof(line), "%s · %s · distancias en línea recta", SOURCE[country], provinceMeta.clock);
  else snprintf(line, sizeof(line), "Datos anteriores · %s", provinceMeta.stamp);
  putShort(10, 188, line, 50, positionOld ? MID : DIM);
}

// ---------- AHORA y TOP 5 ----------
void drawNow() {
  if (showMissing()) return;
  putShort(10, 32, config.name, 50, ACC);
  gfx->fillRoundRect(8, 45, 304, 82, 8, PANEL);
  putShort(18, 51, data.best[0].brand, 24, TXT, 2);
  putShort(18, 72, data.best[0].address, 47, DIM);
  char v[24], line[96];
  price(v, sizeof(v), data.lo / 1000.0f);
  gfx->setFont(&FreeSansBold24pt7b);
  gfx->setTextColor(CHEAP); gfx->setCursor(16, 119); gfx->print(v);
  gfx->setFont();
  put(200, 103, "€/L", TXT, 2);
  put(10, 132, fuelName(), TXT);
  float saving = max(0.0f, average(data) - data.lo / 1000.0f);
  price(v, sizeof(v), saving * 100, 1);
  snprintf(line, sizeof(line), "%s céntimos menos que la media", v);
  put(10, 147, line, CHEAP);
  price(v, sizeof(v), saving * config.litres, 2);
  snprintf(line, sizeof(line), "Ahorras %s € en un depósito de %u L", v, config.litres);
  put(10, 162, line, CHEAP);
  if (country == 2) snprintf(line, sizeof(line), "Media de las %lu más baratas (E-Control)", (unsigned long)data.count);
  else snprintf(line, sizeof(line), "Frente a la media de %lu estaciones públicas", (unsigned long)data.count);
  put(10, 176, line, DIM);
  dataTime(188);
}

uint16_t rankColor(int rank) {
  int r = 5 + rank * 6, g = 52 - rank, b = 10 - rank * 2;
  return (r << 11) | (g << 5) | b;
}

void drawTop() {
  if (showMissing()) return;
  putShort(10, 32, config.name, 30, ACC);
  putRight(310, 32, fuelName(), DIM);
  int n = min((uint32_t)5, data.count);
  for (int i = 0; i < n; i++) {
    int y = 48 + i * 29;
    uint16_t c = rankColor(i);
    gfx->fillRoundRect(8, y, 304, 26, 4, PANEL);
    gfx->fillRect(8, y + 3, 3, 20, c);
    putShort(17, y + 2, data.best[i].brand, 30, c);
    putShort(17, y + 15, data.best[i].address, 36, DIM);
    char v[16]; price(v, sizeof(v), data.best[i].mills / 1000.0f);
    putRight(306, y + 5, v, c, 2);
  }
}

// ---------- TENDENCIA ----------
int dayNumber(int date) {
  struct tm t = {};
  t.tm_year = date / 10000 - 1900; t.tm_mon = (date / 100) % 100 - 1; t.tm_mday = date % 100;
  t.tm_hour = 12; t.tm_isdst = -1;
  return lround((double)mktime(&t) / 86400.0);
}

void drawTrend() {
  if (showMissing()) return;
  putShort(10, 32, config.name, 50, ACC);
  if (history.count < 2) {
    putCenter(160, 82, "Vuelve mañana para ver la tendencia", TXT);
    putCenter(160, 108, "Una muestra al día, después de las 08:00", DIM);
    return;
  }
  int yesterday;
  char line[80], v[16];
  if (yesterdayMin(yesterday)) {
    int diff = data.lo - yesterday;
    put(10, 48, diff < 0 ? "Hoy más barata que ayer" : diff > 0 ? "Hoy más cara que ayer" : "Hoy igual que ayer", TXT);
    price(v, sizeof(v), abs(diff) / 10.0f, 1);
    snprintf(line, sizeof(line), "(%s%s cts)", diff < 0 ? "−" : diff > 0 ? "+" : "", v);
    putRight(310, 63, line, diff <= 0 ? CHEAP : MID);
  } else put(10, 48, "Sin comparación de hoy con ayer", DIM);
  float lo = 100, hi = 0;
  int firstDay = dayNumber(history.days[0].date), lastDay = dayNumber(history.days[history.count - 1].date);
  for (int i = 0; i < history.count; i++) {
    lo = min(lo, history.days[i].lo / 1000.0f);
    hi = max(hi, history.days[i].avg);
  }
  if (hi - lo < 0.01f) { hi += 0.005f; lo -= 0.005f; }
  auto yOf = [&](float p) { return 161 - (int)((p - lo) / (hi - lo) * 75); };
  for (int k = 0; k < 3; k++) {
    float p = lo + (hi - lo) * k / 2;
    int y = yOf(p);
    gfx->drawFastHLine(43, y, 266, PANEL);
    price(v, sizeof(v), p, 2); put(8, y - 3, v, DIM);
  }
  int px = 0, pyLo = 0, pyAvg = 0, previousDay = 0;
  for (int i = 0; i < history.count; i++) {
    int day = dayNumber(history.days[i].date);
    int x = 44 + (day - firstDay) * 260 / max(1, lastDay - firstDay);
    int yl = yOf(history.days[i].lo / 1000.0f), ya = yOf(history.days[i].avg);
    if (i && day == previousDay + 1) {  // No unir días ausentes.
      gfx->drawLine(px, pyLo, x, yl, CHEAP); gfx->drawLine(px, pyAvg, x, ya, ACC);
    }
    gfx->fillCircle(x, yl, 2, CHEAP); gfx->fillCircle(x, ya, 2, ACC);
    px = x; pyLo = yl; pyAvg = ya; previousDay = day;
  }
  snprintf(line, sizeof(line), "%02d/%02d", history.days[0].date % 100, history.days[0].date / 100 % 100);
  put(43, 170, line, DIM);
  int date = history.days[history.count - 1].date;
  snprintf(line, sizeof(line), "%02d/%02d", date % 100, date / 100 % 100);
  putRight(310, 170, line, DIM);
  put(72, 186, "Mínimo", CHEAP); put(145, 186, "Media · €/L", ACC);
}

// ---------- AJUSTES ----------
void drawSettings() {
  putShort(10, 32, config.name[0] ? config.name : "Elige tu pueblo", 50, ACC);
  put(10, 48, fuelName(), TXT);
  char line[48]; snprintf(line, sizeof(line), "Depósito: %u L · %s", config.litres, COUNTRIES[country]);
  put(10, 63, line, DIM);
  put(174, 48, "Escanea y ajusta", TXT);
  settingsQr(174, 65, 132);
  button(B_REFRESH, "Actualizar ahora", ACC);
  button(B_BRIGHT, autoBright ? "Brillo: automático" : "Brillo: fijo", TXT);
  button(B_CAL, "Recalibrar táctil", TXT);
  button(B_WIFI, "Wi-Fi: mantener 2 s", MID);
}

void drawScreen() {
  drawHeader(); clearBody();
  switch (screen) {
    case S_NEAR: drawNear(); break;
    case S_NOW: drawNow(); break;
    case S_TOP: drawTop(); break;
    case S_TREND: drawTrend(); break;
    case S_SETTINGS: drawSettings(); break;
  }
  drawFooter();
}

void slideTo(int next, int dir) {
  for (int i = 0; i < 8; i++) {
    int x = dir > 0 ? W - (i + 1) * 40 : i * 40;
    gfx->fillRect(x, 26, 40, 172, PANEL); delay(8);
  }
  screen = (next + S_COUNT) % S_COUNT;
  drawScreen();
}

// ---------- Web local, sin recursos ni servicios externos ----------
const char PAGE_HEAD[] PROGMEM = R"HTML(<!doctype html><html lang="es"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Gasolina MS</title>
<style>body{font-family:system-ui,sans-serif;background:#0b1220;color:#e8eef7;max-width:460px;margin:auto;padding:20px}
h1{font-size:24px}input,select,button{width:100%;box-sizing:border-box;padding:12px;margin:6px 0 14px;border-radius:10px;
border:1px solid #334;background:#152033;color:#fff;font-size:16px}button{background:#00b4ff;color:#001;font-weight:700}
small{color:#9ab}a{color:#00b4ff}#estado{min-height:2em}</style>)HTML";

const char SETTINGS_PAGE[] PROGMEM = R"HTML(<h1>Gasolina MS</h1>
<p>Elige tu pueblo de respaldo y cómo localizarte.</p><p id="estado" role="status">Cargando ajustes...</p>
<form id="ajustes"><label for="pais">País</label><select id="pais">
<option value="0">España</option><option value="1">Francia</option><option value="2">Austria</option></select>
<div id="es"><label for="provincia">Provincia</label><select id="provincia" disabled></select>
<label for="municipio">Pueblo o ciudad</label><select id="municipio" disabled></select></div>
<div id="fr" hidden><label for="cp">Código postal</label><input id="cp" inputmode="numeric" maxlength="5" pattern="[0-9]{5}"></div>
<div id="at" hidden><label for="distrito">Distrito (Bezirk)</label><select id="distrito" disabled></select>
<small>Austria solo publica las ~10 gasolineras más baratas de cada zona.</small></div>
<label for="ubicacion">Ubicación</label><select id="ubicacion">
<option value="auto">Automática por Wi-Fi</option><option value="town">Solo mi pueblo</option></select>
<small>El modo automático envía MAC y señal de redes cercanas (sin nombres ni tu hotspot) a beaconDB.
La posición aproximada solo permanece en RAM. Sin GPS ni cuenta.</small>
<label for="producto">Combustible</label><select id="producto"></select>
<label for="litros">Litros del depósito</label><input id="litros" type="number" min="1" max="200" step="1" value="50" required>
<button id="guardar" disabled>Guardar</button></form><button id="reintentar" type="button">Recargar listas</button>
<small>Precios oficiales (datos abiertos). Pueden tardar en actualizarse: confirma en el surtidor.
Fuentes: Ministerio para la Transición Ecológica (España), prix-carburants.gouv.fr (Francia), E-Control (Austria).
Aplicación independiente.</small>
<script>
const el=id=>document.getElementById(id);
let token='',generation=0,saved=null;
const FUELS={0:[[1,'Gasolina 95 E5'],[3,'Gasolina 98 E5'],[4,'Gasóleo A'],[5,'Gasóleo Premium'],[17,'GLP']],
1:[[0,'Gazole (diésel)'],[1,'SP95'],[2,'SP95-E10'],[3,'SP98'],[4,'GPLc']],2:[[0,'Diésel'],[1,'Super 95']]};
async function request(url,options){
  const r=await fetch(url,options);
  const value=await r.json();
  if(!r.ok||!value.ok)throw new Error(value.error||'No se pudo cargar. Reintenta.');
  return value;
}
function options(id,items,selected){
  const select=el(id);select.replaceChildren();
  select.add(new Option('Elige una opción',''));
  for(const row of items)select.add(new Option(row.nombre,row.id));
  select.value=selected||'';select.disabled=false;
}
async function towns(selected){
  const mine=++generation,province=el('provincia').value;
  el('municipio').disabled=true;el('municipio').replaceChildren();el('guardar').disabled=true;
  if(!province)return;
  el('estado').textContent='Cargando pueblos...';
  try{
    const data=await request('/municipios?p='+encodeURIComponent(province));
    if(mine!==generation)return;
    options('municipio',data.items,selected);
    el('estado').textContent='Elige el pueblo y guarda.';
    el('guardar').disabled=false;
  }catch(e){if(mine===generation)el('estado').textContent=e.message;}
}
async function country(){
  const c=el('pais').value,mine=saved&&String(saved.pais||0)===c;
  el('es').hidden=c!=='0';el('fr').hidden=c!=='1';el('at').hidden=c!=='2';el('cp').disabled=c!=='1';
  const p=el('producto');p.replaceChildren();
  for(const[v,n]of FUELS[c])p.add(new Option(n,v));
  if(mine)p.value=String(saved.producto);
  el('guardar').disabled=c!=='1';
  if(c==='1'){el('cp').value=mine?saved.municipio:'';el('estado').textContent='Escribe el código postal y guarda.';}
  else if(c==='2'){
    el('estado').textContent='Cargando distritos...';
    const data=await request('/distritos');options('distrito',data.items,mine?saved.municipio:'');
    el('guardar').disabled=false;el('estado').textContent='Elige el distrito y guarda.';
  }else{
    const data=await request('/provincias');options('provincia',data.items,mine?saved.provincia:'');
    if(mine&&saved.provincia)await towns(saved.municipio);
    else el('estado').textContent='Elige la provincia y después el pueblo.';
  }
}
async function load(){
  el('guardar').disabled=true;el('provincia').disabled=true;el('municipio').disabled=true;
  el('estado').textContent='Cargando ajustes...';
  el('reintentar').disabled=true;
  try{
    saved=await request('/config');token=saved.token;
    el('pais').value=String(saved.pais||0);el('litros').value=saved.litros;
    el('ubicacion').value=saved.ubicacion||'auto';
    await country();
  }catch(e){el('estado').textContent=e.message;}
  finally{el('reintentar').disabled=false;}
}
el('pais').addEventListener('change',()=>country().catch(e=>el('estado').textContent=e.message));
el('provincia').addEventListener('change',()=>towns(''));
el('reintentar').addEventListener('click',load);
el('ajustes').addEventListener('submit',async event=>{
  event.preventDefault();el('guardar').disabled=true;el('estado').textContent='Comprobando y guardando...';
  try{
    // Permiso nuevo en cada guardado: si la placa se ha reiniciado, el de la página ya no vale.
    token=(await request('/config')).token;
    const c=el('pais').value;
    const body=new URLSearchParams({pais:c,provincia:c==='0'?el('provincia').value:'',
      municipio:c==='0'?el('municipio').value:c==='1'?el('cp').value.trim():el('distrito').value,
      producto:el('producto').value,litros:el('litros').value,ubicacion:el('ubicacion').value,token});
    await request('/guardar',{method:'POST',body});el('estado').textContent='Guardado. La placa actualizará los precios.';}
  catch(e){el('estado').textContent=e.message;}
  finally{el('guardar').disabled=false;}
});
load();
</script></html>)HTML";

void jsonError(int status, const String& message) {
  JsonDocument doc; doc["ok"] = false; doc["error"] = message;
  String out; serializeJson(doc, out);
  web.send(status, "application/json; charset=utf-8", out);
}

void handleRoot() {
  web.sendHeader("Cache-Control", "no-store");
  String page = FPSTR(PAGE_HEAD);
  if (portalMode) {
    page += "<h1>Gasolina MS · Wi-Fi</h1><p>Conecta la placa a una red de 2,4 GHz. La clave se guarda solo en la placa.</p>"
            "<form method=post action=/wifi><label>Nombre de la red</label>"
            "<input name=s maxlength=32 required autocomplete=off><label>Contraseña</label>"
            "<input name=p type=password maxlength=63 autocomplete=off><input type=hidden name=token value='";
    page += formToken;
    page += "'><button>Guardar y conectar</button></form><small>MS Robotika · AB MakerIDE</small></html>";
  } else page += FPSTR(SETTINGS_PAGE);
  web.send(200, "text/html; charset=utf-8", page);
}

void handleWifiSave() {
  if (!portalMode || web.arg("token") != formToken) { jsonError(403, "Vuelve a abrir el portal"); return; }
  String s = web.arg("s"), p = web.arg("p");
  // No recortar el nombre: los espacios pueden pertenecer a la red.
  if (!s.length() || s.length() > 32 || (p.length() && (p.length() < 8 || p.length() > 63))) {
    web.send(400, "text/html; charset=utf-8", String(FPSTR(PAGE_HEAD)) +
             "<h1>Revisa los datos</h1><p>Escribe el nombre de la red y una clave de 8 a 63 caracteres, o vacía si es abierta.</p><a href=/>Volver</a>");
    return;
  }
  if (prefs.putString("ssid", s) != s.length() ||
      (prefs.putString("pass", p) != p.length())) {
    jsonError(500, "No se pudo guardar la Wi-Fi"); return;
  }
  web.send(200, "text/html; charset=utf-8", String(FPSTR(PAGE_HEAD)) +
           "<h1>Guardado</h1><p>La placa se reinicia. Conecta el móvil a tu red habitual y abre el QR de AJUSTES.</p>");
  delay(1200); ESP.restart();
}

void handleConfig() {
  if (portalMode) { jsonError(403, "Configura primero la Wi-Fi"); return; }
  JsonDocument doc;
  doc["ok"] = true; doc["token"] = formToken;
  doc["provincia"] = config.province; doc["municipio"] = config.town;
  doc["pais"] = country;
  doc["producto"] = country ? config.fuel : PRODUCT_IDS[config.fuel]; doc["litros"] = config.litres;
  doc["ubicacion"] = nearEnabled ? "auto" : "town";
  String out; serializeJson(doc, out);
  web.sendHeader("Cache-Control", "no-store");
  web.send(200, "application/json; charset=utf-8", out);
}

// Última lista enviada a la página (0 = provincias, 1 = pueblos de listKey[1]). Al guardar se valida con
// ella y no se vuelve a pedir al Ministerio, que a veces tarda decenas de segundos en responder.
String listCache[3], listKey[3];  // 2 = distritos de Austria.

String nameFromCache(int kind, const String& key, const String& id) {
  if (listCache[kind].isEmpty() || listKey[kind] != key) return "";
  String mark = "{\"id\":\"" + id + "\",\"nombre\":\"";
  int a = listCache[kind].indexOf(mark);
  if (a < 0) return "";
  a += mark.length();
  int b = listCache[kind].indexOf("\"}", a);
  return b < 0 ? "" : listCache[kind].substring(a, b);
}

void handleList(bool towns) {
  if (portalMode) { jsonError(403, "Configura primero la Wi-Fi"); return; }
  String province = towns ? web.arg("p") : "";
  if (towns && !digits(province, 2)) { jsonError(400, "Provincia no válida"); return; }
  int kind = towns ? 1 : 0;
  if (listCache[kind].isEmpty() || listKey[kind] != province) {
    String found, error, items;
    if (!readList(province, "", found, error, &items)) { jsonError(502, "No se pudo cargar la lista. Reintenta."); return; }
    listCache[kind] = items; listKey[kind] = province;
  }
  web.sendHeader("Cache-Control", "no-store");
  web.send(200, "application/json; charset=utf-8", "{\"items\":[" + listCache[kind] + "],\"ok\":true}");
}

// Distritos (Bezirke) de E-Control: ~40 kB, leídos enteros como los listados del Ministerio.
bool loadDistricts(String& error) {
  if (!listCache[2].isEmpty()) return true;
  freeNear();
  WiFiClientSecure client;
  HTTPClient http;
  bool ok = openUrl(http, client, AT_CA, String(AT_BASE) + "/regions?includeCities=false", "E-Control", error);
  String body = ok ? http.getString() : String();
  http.end();
  if (!ok) return false;
  JsonDocument filter, doc;
  filter[0]["name"] = true; filter[0]["subRegions"][0]["code"] = true; filter[0]["subRegions"][0]["name"] = true;
  if (deserializeJson(doc, body, DeserializationOption::Filter(filter)) || !doc.is<JsonArray>()) {
    error = "No se pudo leer la lista de distritos"; return false;
  }
  body = String();
  String items;
  for (JsonObject state : doc.as<JsonArray>())
    for (JsonObject d : state["subRegions"].as<JsonArray>()) {
      if ((d["code"] | 0) == 900) continue;  // «Wien(Stadt)» no devuelve gasolineras: se usan sus 23 distritos.
      JsonDocument row; row["id"] = String(d["code"] | 0);
      row["nombre"] = String(d["name"] | "") + " · " + (const char*)(state["name"] | "");
      String one;
      serializeJson(row, one);  // serializeJson sobrescribe el String: añadir aparte.
      if (items.length()) items += ',';
      items += one;
    }
  if (items.isEmpty()) { error = "Lista de distritos vacía"; return false; }
  listCache[2] = items; listKey[2] = "";
  return true;
}

void handleDistricts() {
  if (portalMode) { jsonError(403, "Configura primero la Wi-Fi"); return; }
  String error;
  if (!loadDistricts(error)) { jsonError(502, "No se pudo cargar la lista. Reintenta."); return; }
  web.sendHeader("Cache-Control", "no-store");
  web.send(200, "application/json; charset=utf-8", "{\"items\":[" + listCache[2] + "],\"ok\":true}");
}

// Francia: el código postal vale si tiene gasolineras publicadas; el nombre sale de la primera.
bool frPlace(const String& cp, String& name, String& error) {
  WiFiClientSecure client;
  HTTPClient http;
  bool ok = openUrl(http, client, FR_CA, String(FR_BASE) + "?limit=1&select=ville&where=cp%3D%22" + cp + "%22",
                    SOURCE[1], error);
  JsonDocument filter, doc;
  filter["total_count"] = true; filter["results"][0]["ville"] = true;
  if (ok) {
    BufferedInput input(http.getStream()); input.setTimeout(15000);
    ok = !deserializeJson(doc, input, DeserializationOption::Filter(filter));
    if (!ok) error = "Respuesta no válida";
  }
  http.end();
  if (ok && (doc["total_count"] | 0) == 0) { error = "No hay gasolineras con ese código postal"; ok = false; }
  if (ok) name = "CP " + cp + " · " + (const char*)(doc["results"][0]["ville"] | "");
  return ok;
}

void handleSettingsSave() {
  if (portalMode || web.arg("token") != formToken) { jsonError(403, "Vuelve a abrir los ajustes"); return; }
  String province = web.arg("provincia"), town = web.arg("municipio"), litres = web.arg("litros");
  String product = web.arg("producto"), locationMode = web.arg("ubicacion"), countryArg = web.arg("pais");
  int nextCountry = countryArg.isEmpty() ? 0 : countryArg.length() == 1 ? countryArg[0] - '0' : -1;
  int fuel = -1;
  if (nextCountry == 0) { for (int i = 0; i < 5; i++) if (product == String(PRODUCT_IDS[i])) fuel = i; }
  else if (nextCountry > 0 && nextCountry <= 2 && product.length() == 1 && product[0] >= '0' &&
           product[0] < '0' + fuelCount(nextCountry)) fuel = product[0] - '0';
  bool place = nextCountry == 0 ? digits(province, 2) && digits(town, 6)
             : nextCountry == 1 ? town.length() == 5 && digits(town, 5) : digits(town, 4);
  if (nextCountry < 0 || nextCountry > 2 || !place || !digits(litres, 3) || litres.toInt() > 200 || fuel < 0 ||
      (locationMode != "auto" && locationMode != "town")) {
    jsonError(400, "Revisa país, zona, combustible y litros (1 a 200)"); return;
  }
  String officialTown, error;
  if (nextCountry == 0) {
    String officialProvince = nameFromCache(0, "", province);
    officialTown = nameFromCache(1, province, town);
    if ((officialProvince.isEmpty() && (!readList("", province, officialProvince, error) || officialProvince.isEmpty())) ||
        (officialTown.isEmpty() && (!readList(province, town, officialTown, error) || officialTown.isEmpty()))) {
      jsonError(400, error.isEmpty() ? "El pueblo no pertenece a esa provincia" : error); return;
    }
  } else if (nextCountry == 1) {
    if (!frPlace(town, officialTown, error)) { jsonError(400, error); return; }
  } else {
    if (loadDistricts(error)) officialTown = nameFromCache(2, "", town);
    if (officialTown.isEmpty()) { jsonError(400, error.isEmpty() ? "Distrito no válido" : error); return; }
  }
  Config next;
  if (nextCountry == 0) strlcpy(next.province, province.c_str(), sizeof(next.province));
  strlcpy(next.town, town.c_str(), sizeof(next.town));
  strlcpy(next.name, officialTown.c_str(), sizeof(next.name));
  next.fuel = fuel; next.litres = litres.toInt();
  if (prefs.putBytes("config", &next, sizeof(next)) != sizeof(next)) { jsonError(500, "No se pudo guardar"); return; }
  bool nextNear = locationMode == "auto";
  if (prefs.putBool("near", nextNear) != 1 || prefs.putUChar("pais", nextCountry) != 1) {
    prefs.putBytes("config", &config, sizeof(config)); prefs.putUChar("pais", country); prefs.putBool("near", nearEnabled);
    jsonError(500, "No se pudo guardar la ubicación"); return;
  }
  bool changed = strcmp(config.town, next.town) || config.fuel != next.fuel || country != nextCountry;
  if (config.fuel != next.fuel || country != nextCountry) { provinceValid = false; nearCount = 0; }
  if (country != nextCountry) {
    // Otro país: olvidar el centro del pueblo y la provincia española; se vuelve a localizar.
    townLat = townLon = 0; positionTown = havePosition = positionOld = false; positionFailures = 0; positionProvince = 0;
  }
  country = nextCountry;
  config = next;
  nearEnabled = nextNear;
  positionWanted = provinceWanted = nearEnabled;
  if (!nearEnabled) {
    havePosition = positionOld = provinceValid = positionTown = false;
    positionLat = positionLon = positionAccuracy = 0; positionFailures = 0; positionProvince = 0;
    nearCount = 0; nearError[0] = 0;
    delete[] nearStations; nearStations = nullptr;
    if (screen == S_NEAR) screen = S_NOW;
  }
  if (changed) {
    data = Summary(); fetchedAt = 0; lastError[0] = 0;
    resetHistory();
    if (!saveHistory()) strlcpy(lastError, "No se pudo guardar la tendencia", sizeof(lastError));
  }
  refreshWanted = true;
  web.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
  drawScreen();
}

void startWeb() {
  web.on("/", HTTP_GET, handleRoot);
  web.on("/wifi", HTTP_POST, handleWifiSave);
  web.on("/config", HTTP_GET, handleConfig);
  web.on("/provincias", HTTP_GET, [] { handleList(false); });
  web.on("/municipios", HTTP_GET, [] { handleList(true); });
  web.on("/distritos", HTTP_GET, handleDistricts);
  web.on("/guardar", HTTP_POST, handleSettingsSave);
  web.onNotFound([] {
    if (portalMode) { web.sendHeader("Location", "http://192.168.4.1/"); web.send(302, "text/plain", ""); }
    else jsonError(404, "Página no encontrada");
  });
  web.begin();
}

void startPortal() {
  portalMode = true;
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  char pass[9]; snprintf(pass, sizeof(pass), "%08lu", (unsigned long)(esp_random() % 100000000UL));
  apPass = pass;
  WiFi.softAP("Gasolina-MS", pass);
  dns.start(53, "*", WiFi.softAPIP());
  gfx->fillScreen(BG);
  gfx->fillRect(0, 0, W, 26, PANEL);
  put(10, 9, "GASOLINA MS · CONFIGURAR WI-FI", ACC, 1, PANEL);
  showQr("WIFI:T:WPA;S:Gasolina-MS;P:" + apPass + ";;", 8, 40, 150);
  put(170, 43, "1. Escanea el QR", TXT);
  put(170, 58, "   con el móvil", TXT);
  put(170, 79, "2. Abre el portal", TXT);
  put(170, 94, "   192.168.4.1", DIM);
  put(170, 115, "3. Escribe tu red", TXT);
  put(170, 130, "   y su clave", TXT);
  put(170, 153, "Red: Gasolina-MS", ACC);
  put(170, 169, "Clave:", ACC); put(210, 166, pass, MID, 2);
  drawFooter();
  setLed(false, false, true);
}

bool connectWifi() {
  String s = prefs.getString("ssid", ""), p = prefs.getString("pass", "");
  if (s.isEmpty()) return false;
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("gasolina-ms");
  WiFi.setAutoReconnect(true);
  WiFi.begin(s.c_str(), p.c_str());
  gfx->fillScreen(BG);
  putCenter(160, 96, "GASOLINA MS", MID, 3);
  putCenter(160, 132, "Conectando a la Wi-Fi...", TXT);
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {
    gfx->fillCircle(130 + (i % 5) * 15, 160, 4, ACC);
    if (i % 5 == 4) gfx->fillRect(120, 154, 80, 12, BG);
    delay(500);
  }
  return WiFi.status() == WL_CONNECTED;
}

// ---------- Gestos de Kilovatio ----------
bool touching = false, longFired = false;
int downX = 0, downY = 0, lastX = 0, lastY = 0;
uint32_t downAt = 0, upAt = 0;

void onTap(int x, int y) {
  if (screen == S_SETTINGS) {
    if (inside(B_REFRESH, x, y)) { refreshWanted = true; positionWanted = provinceWanted = nearEnabled; }
    else if (inside(B_BRIGHT, x, y)) { autoBright = !autoBright; prefs.putBool("auto", autoBright); updateBrightness(); }
    else if (inside(B_CAL, x, y)) calibrate();
    else return;
    drawScreen();
  } else if (screen == S_NEAR && nearEnabled) { positionWanted = provinceWanted = true; }
  else if (inside(B_RETRY, x, y) && (lastError[0] || !data.count)) refreshWanted = true;
}

void onLongPress(int x, int y) {
  if (screen == S_SETTINGS && inside(B_WIFI, x, y)) startPortal();
}

void handleTouch() {
  int rx, ry;
  uint32_t t = millis();
  if (sampleRaw(rx, ry)) {
    int x, y; toScreen(rx, ry, x, y);
    if (!touching) { touching = true; longFired = false; downX = x; downY = y; downAt = t; }
    lastX = x; lastY = y; upAt = t;
    if (!longFired && t - downAt > 2000 && abs(x - downX) < 25 && abs(y - downY) < 25) {
      longFired = true; onLongPress(downX, downY);
    }
    return;
  }
  if (touching && t - upAt > 60) {
    touching = false;
    if (longFired || portalMode) return;
    int dx = lastX - downX, dy = lastY - downY;
    if (abs(dx) > 50 && abs(dx) > abs(dy)) slideTo(screen + (dx < 0 ? 1 : -1), dx < 0 ? 1 : -1);
    else if (abs(dx) < 15 && abs(dy) < 15) onTap(downX, downY);
  }
}

void loadPreferences() {
  nearEnabled = prefs.getBool("near", true);
  country = prefs.getUChar("pais", 0);
  if (country > 2) country = 0;
  if (prefs.getBytesLength("config") == sizeof(config)) {
    Config saved;
    prefs.getBytes("config", &saved, sizeof(saved));
    saved.province[sizeof(saved.province) - 1] = 0; saved.town[sizeof(saved.town) - 1] = 0;
    saved.name[sizeof(saved.name) - 1] = 0;
    bool place = country == 0 ? digits(saved.province, 2) && digits(saved.town, 6)
               : country == 1 ? strlen(saved.town) == 5 && digits(saved.town, 5) : digits(saved.town, 4);
    if (saved.version == 1 && saved.fuel < fuelCount(country) && saved.litres >= 1 && saved.litres <= 200 &&
        place && saved.name[0]) config = saved;
  }
  resetHistory();
  if (prefs.getBytesLength("history") == sizeof(history)) {
    History saved;
    prefs.getBytes("history", &saved, sizeof(saved)); saved.town[sizeof(saved.town) - 1] = 0;
    bool valid = saved.version == 1 && saved.count <= 14 && saved.fuel == config.fuel && !strcmp(saved.town, config.town);
    for (int i = 0; i < saved.count && valid; i++)
      valid = saved.days[i].date >= 20200101 && saved.days[i].date <= 20991231 && saved.days[i].lo > 0 &&
              isfinite(saved.days[i].avg) && saved.days[i].avg >= saved.days[i].lo / 1000.0f - 0.0001f &&
              (!i || saved.days[i].date > saved.days[i - 1].date);
    if (valid) history = saved;
  }
}

// ---------- Programa ----------
void setup() {
  Serial.begin(115200); delay(200);
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
  setLed(false, false, false);
  pinMode(5, OUTPUT); digitalWrite(5, HIGH);  // microSD sin usar.
  ledcAttach(PIN_BL, 5000, 8); ledcWrite(PIN_BL, 230);
  analogSetPinAttenuation(PIN_LDR, ADC_0db);
  lcdBus.begin(14, 12, 13, 15); lcd.begin(40000000); lcd.setRotation(1); gfx->cp437(true);
  gfx->setTextWrap(false);
  touchBus.begin(25, 39, 32, 33); touch.begin(touchBus); touch.setRotation(1);
  prefs.begin("gasolina-ms", false);
  loadPreferences(); autoBright = prefs.getBool("auto", true);
  if (prefs.getBool("cal", false)) {
    ax = prefs.getFloat("ax"); bx = prefs.getFloat("bx"); cx = prefs.getFloat("cx");
    ay = prefs.getFloat("ay"); by = prefs.getFloat("by"); cy = prefs.getFloat("cy");
  } else calibrate();
  updateBrightness();
  char token[17]; snprintf(token, sizeof(token), "%08lx%08lx", (unsigned long)esp_random(), (unsigned long)esp_random());
  formToken = token;
  if (!connectWifi()) { startPortal(); startWeb(); return; }
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "es.pool.ntp.org", "pool.ntp.org", "time.google.com");
  settingsUrl = "http://" + WiFi.localIP().toString() + "/";
  Serial.printf("GASOLINA ajustes=%s\n", settingsUrl.c_str());  // Única IP en serie.
  startWeb();
  putCenter(160, 180, "Sincronizando hora...", DIM);
  struct tm t;
  for (int i = 0; i < 60 && !now(t); i++) { web.handleClient(); delay(100); }
  drawScreen();
}

// Capturas por USB para la documentación y los vídeos (no hace falta tocar la pantalla):
// "MSR_TAB n" cambia de pantalla y "MSR_SHOT" manda la pantalla en RGB565, franja a franja.
class Strip : public Adafruit_GFX {
 public:
  uint16_t buf[320 * 24];  // 15 kB: cabe aunque la provincia ocupe ~86 kB.
  int y0 = 0;
  Strip() : Adafruit_GFX(320, 240) {}
  void drawPixel(int16_t x, int16_t y, uint16_t c) override {
    y -= y0;
    if (x >= 0 && x < 320 && y >= 0 && y < 24) buf[y * 320 + x] = c;
  }
};
void sendScreenshot() {
  Strip* st = new (std::nothrow) Strip();
  if (!st) { Serial.printf("MSR_SHOT_ERROR heap=%u\n", (unsigned)ESP.getFreeHeap()); return; }
  st->cp437(true);
  Serial.printf("MSR_SHOT_BEGIN %d %d\n", 320, 240);
  for (st->y0 = 0; st->y0 < 240; st->y0 += 24) {
    memset(st->buf, 0, sizeof(st->buf));
    gfx = st;
    drawScreen();
    gfx = &lcd;
    Serial.write((const uint8_t*)st->buf, sizeof(st->buf));
  }
  Serial.println();
  Serial.println("MSR_SHOT_END");
  delete st;
}
void pollSerial() {
  static char buf[32];
  static uint8_t len = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c != '\n' && c != '\r') { if (len < sizeof(buf) - 1) buf[len++] = c; continue; }
    buf[len] = 0; len = 0;
    int a;
    if (sscanf(buf, "MSR_TAB %d", &a) == 1 && a >= 0 && a < S_COUNT) { screen = a; drawScreen(); Serial.println("MSR_CMD_OK"); }
    else if (!strcmp(buf, "MSR_SHOT")) sendScreenshot();
  }
}

void loop() {
  web.handleClient();
  pollSerial();
  if (portalMode) { dns.processNextRequest(); delay(2); return; }
  handleTouch();
  if (portalMode) return;
  uint32_t ms = millis();
  if (ms - lastBright > 1500) { lastBright = ms; updateBrightness(); }
  if (WiFi.status() == WL_CONNECTED) {
    String url = "http://" + WiFi.localIP().toString() + "/";
    if (url != settingsUrl) { settingsUrl = url; drawScreen(); }
  } else if (ms - lastReconnect > 30000) { lastReconnect = ms; WiFi.reconnect(); }
  if (nearEnabled && (positionWanted || ms - lastPositionTry >= 120000UL)) {
    refreshPosition(); drawScreen(); ms = millis();
  }
  if (nearEnabled && havePosition &&
      (provinceWanted || (provinceValid ? ms - provinceFetchedAt >= 3600000UL : ms - lastProvinceTry >= 120000UL))) {
    refreshProvince(); drawScreen(); ms = millis();
  }
  uint32_t interval = lastError[0] ? 120000UL : 1800000UL;
  if (config.town[0] && (refreshWanted || ms - lastFetchTry >= interval)) {
    putCenter(160, 188, "Actualizando precios...", ACC, 1, BG);
    refreshPrices(); drawScreen();
    // Al arrancar la posición se pide antes que los precios: reintentar ya con el centro del pueblo.
    if (nearEnabled && !havePosition && (townLat || townLon)) positionWanted = true;
  }
  if (ms - lastTick > 1000) {
    lastTick = ms;
    static int lastDate = 0, lastMinute = -1;
    struct tm t;
    if (now(t)) {
      if (lastDate && ymdOf(t) != lastDate) { refreshWanted = true; drawScreen(); }
      lastDate = ymdOf(t);
      if (lastMinute != t.tm_min) { lastMinute = t.tm_min; drawHeader(); }
    }
  }
  applyLed(); delay(5);
}

