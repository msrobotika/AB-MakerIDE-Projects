# Reloj Despertador MS · CYD

[← Catálogo](../../README.md) · [AB MakerIDE SUPER](https://github.com/msrobotika/AB-MakerIDE-SUPER)

Reloj despertador con Wi-Fi para la **CYD ESP32-2432S028R** (ILI9341 horizontal 320 × 240 y táctil resistivo XPT2046). Se pone en hora solo por internet (NTP), cambia solo al horario de verano y tiene una alarma que se ajusta tocando la pantalla o desde el monitor serie.

Hecho con AB MakerIDE. Es el proyecto del **tutorial en vídeo** del IDE: crear el proyecto, elegir la placa, librerías, compilar, grabar y usar el monitor serie.

## Cómo se usa

| Gesto | Qué hace |
|---|---|
| Botón **H+** | Sube la hora de la alarma (de 00 a 23 y vuelta a empezar). |
| Botón **M+** | Sube los minutos de 5 en 5. |
| Botón **ON/OFF** | Activa o desactiva la alarma (en naranja = activa). |
| Al sonar: **POSPONER** | Vuelve a sonar en 5 minutos. |
| Al sonar: **PARAR** | La apaga hasta el día siguiente. |
| Mantener pulsado 3 s | Abre la configuración de la Wi-Fi. |

- Pantalla: hora en dígitos de 7 segmentos, barra de segundos, fecha en español y la tarjeta de la alarma.
- Al sonar, la pantalla parpadea en rojo, el LED RGB se pone rojo y suena un bip-bip por el conector **SPEAK** (GPIO 26) si le conectas un altavoz pequeño. Sin altavoz, el aviso es solo visual.
- Si nadie la para, deja de sonar sola a los 5 minutos.
- La alarma y su estado se guardan en la placa y se mantienen al reiniciar.

### Monitor serie (115200 baudios)

| Comando | Qué hace |
|---|---|
| `alarma 07:30` | Pone la alarma a esa hora y la activa. |
| `alarma on` / `alarma off` | Activa o desactiva la alarma (también la para si está sonando). |
| `hora` | Responde con la hora de la placa. |

## Primera puesta en marcha

1. Si la placa ya tenía Kilovatio MS o Radar Aviones MS, reutiliza su calibración del táctil y su Wi-Fi. Si no, toca el centro de las tres cruces para calibrar.
2. Wi-Fi: puedes escribirla al principio del código (`WIFI_SSID` y `WIFI_PASS`). Si lo dejas vacío y no hay ninguna guardada, la placa crea la red **Reloj-MS** y muestra un **QR** con una clave nueva: escanéalo con el móvil y elige tu Wi-Fi de 2,4 GHz (si no se abre la página, entra en `192.168.4.1`).
3. En unos segundos aparece la hora. Mientras no la tiene, muestra guiones y «Sincronizando la hora...».

La Wi-Fi se guarda solo en la memoria de la placa. Si grabas la pantalla del IDE, deja `WIFI_SSID` vacío para no enseñar tu red.

## Ajustes al principio del código

| Ajuste | Por defecto |
|---|---|
| `ZONA_HORARIA` | España peninsular con cambio de hora automático (`CET-1CEST,M3.5.0,M10.5.0/3`). Canarias: `WET0WEST,M3.5.0/1,M10.5.0`. |
| `ALARMA_POR_DEFECTO` | 07:30 (desactivada hasta que la actives). |
| `POSPONER_MIN` | 5 minutos. |
| `ALARMA_MAX_MS` | Deja de sonar sola a los 5 minutos. |

## Límites

- Necesita Wi-Fi para coger la hora al arrancar. Si se corta la Wi-Fi después, el reloj sigue en hora con el reloj interno del ESP32 y vuelve a sincronizar al reconectar.
- Si la placa se queda sin corriente, la alarma no suena: no tiene batería. No lo uses para avisos importantes (medicación, seguridad).
- La alarma suena todos los días a la misma hora; no hay días de la semana ni varias alarmas.

## Hardware y pines

| Elemento | Pines GPIO |
|---|---|
| Pantalla ILI9341 (HSPI) | SCLK 14, MISO 12, MOSI 13, CS 15, DC 2, retroiluminación 21 |
| Táctil XPT2046 (VSPI) | CLK 25, MISO 39, MOSI 32, CS 33, IRQ 36 |
| LED RGB (activo a nivel bajo) | R 4, G 16, B 17 |
| Altavoz (conector SPEAK) | 26 |
| microSD | CS 5 se deja en alto; no se monta |

## Compilar y cargar

1. Abre `RelojDespertadorMS/RelojDespertadorMS.ino` en AB MakerIDE SUPER (o pega el código en un proyecto nuevo).
2. Elige la placa **ESP32 Dev Module** (`esp32:esp32:esp32`, núcleo esp32 3.x) y el puerto de tu CYD.
3. Instala las bibliotecas **Adafruit GFX**, **Adafruit ILI9341** y **XPT2046_Touchscreen** (Paul Stoffregen). En el IDE, **Librerías → Analizar el código actual** te las indica.
4. Compila y graba. Cargar reemplaza el programa que tenga la placa.

## Evidencia

- 3 nov 2026: compilado con el núcleo esp32 3.3.12 (1 020 601 bytes de programa, 77 %; 50 468 bytes de RAM global, 15 %).
- Grabado en una CYD con ESP32-D0WD-V3 por COM4, con escritura verificada por hash, desde Arduino CLI y desde AB MakerIDE (Compilar y grabar a 921600 baudios, «Hash of data verified»).
- Al arrancar responde `MSR_RELOJ_BOOT selftest=OK` (autoprueba del lector de horas, de la suma de minutos y de cuándo debe sonar), se conecta con la Wi-Fi reutilizada de Kilovatio MS (`MSR_RELOJ_WIFI_OK`) y coge la hora por NTP (`MSR_RELOJ_NTP_OK`).
- Alarma comprobada por el monitor serie: `alarma HH:MM` → `MSR_RELOJ_ALARM ring` en el minuto programado; `alarma off` → `MSR_RELOJ_ALARM stop`. Un comando mal escrito (`alarma 25:00`) da error y no cambia la alarma.
- El autor grabó la pantalla real: hora, fecha, tarjeta de la alarma y la pantalla «¡ARRIBA!» con POSPONER y PARAR. [Capturas](../../docs/images/reloj-despertador-ms-cyd/) ([origen](../../docs/images/reloj-despertador-ms-cyd/capturas-manifest.json)).
- Sin comprobar: los botones táctiles H+, M+, ON/OFF, POSPONER y PARAR en la placa (se probaron la lógica y los comandos serie), el portal Wi-Fi con QR de este proyecto y el sonido con un altavoz en SPEAK.

No es una validación de todas las revisiones de la CYD: confirma la serigrafía y la revisión de tu placa antes de grabar.

## Licencia

MIT, como el resto de AB MakerIDE.
