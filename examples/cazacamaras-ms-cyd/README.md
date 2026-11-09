# CazaCámaras MS · CYD

**[Abrir el sketch Arduino](CazaCamarasMS/CazaCamarasMS.ino)**

[← Catálogo](../../README.md) · [AB MakerIDE SUPER](https://github.com/msrobotika/AB-MakerIDE-SUPER)

Detector pasivo de **posibles cámaras** para habitaciones de hotel y alojamientos de vacaciones. Usa la **CYD ESP32-2432S028R**, con ILI9341 horizontal de 320 × 240 y táctil resistivo XPT2046. Escucha señales WiFi de 2,4 GHz y anuncios Bluetooth; busca fabricantes, tráfico compatible con vídeo y nombres típicos de redes de cámara. Después ayuda a localizar la señal con «frío / caliente».

Hecho con AB MakerIDE. Solo necesita la placa: sin cuentas, conexión a Internet ni aplicación móvil.

## Qué detecta

| Pista | Cómo la trata |
|---|---|
| OUI de fabricante de cámaras, confianza ALTA | Cuenta desde la primera señal y muestra «¡Cámara probable!». |
| OUI de fabricante con productos variados, confianza MEDIA | Cuenta con vídeo o tras más de 30 s de presencia continuada. |
| OUI de chip genérico, confianza BAJA | Solo cuenta si hay tráfico compatible con vídeo. Un ESP32 normal no basta. |
| Tráfico de vídeo | Subida estimada de más de 20 000 bytes/s, más del 70 % de tramas de datos y muestras positivas durante más de 10 s. Es una heurística, no identifica el contenido. También se aplica a OUI desconocidos. |
| Red con nombre típico de cámara | Cada unos 20 s hace un barrido pasivo. Muestra únicamente una etiqueta, por ejemplo «Red de cámara tipo V380». Es una pista MEDIA; no provoca rojo por sí sola. |
| Nombre Bluetooth con cam, IPC, V380 o spy | Escaneo BLE pasivo. Pista BAJA para cam/spy y MEDIA para IPC/V380. No muestra ni conserva el nombre. |

La [tabla OUI completa](OUI.md) contiene **128 prefijos reales IEEE**, con ALTA para Hikvision, Dahua, EZVIZ, Imou, Uniview, Reolink, Amcrest, Arlo, Wyze, Hanwha/Wisenet, Axis, Lorex, Swann y YI/Kami. MEDIA incluye Xiaomi/Imilab, TP-Link/Tapo, Amazon/Ring/Blink, Anker/Eufy, Google/Nest y Tuya. BAJA incluye Espressif, Beken, Realtek, Ingenic, Hisilicon y MediaTek, además de AltoBeam y FN-Link.

**Cobertura pendiente:** no se incluyen OUI atribuidos a Foscam ni Anyka sin asignación IEEE verificada. Pueden aparecer mediante las otras pistas. Eufy usa aquí 8C:85:80, asignado a Smart Innovation LLC y observado en una HomeBase 2 en un estudio; no garantiza toda la gama Anker/Eufy. Las asociaciones y sus fuentes están en [OUI.md](OUI.md).

Redes que crean las cámaras mientras se configuran. Se mira solo **el principio del nombre**, sin distinguir mayúsculas, para que redes de casa como «MOVISTAR_A9F0» no cuenten:

- Con nombre de marca o de app de cámaras (confianza **ALTA**): `EZVIZ`, `HIK`, `DAHUA`, `IMOU`, `REOLINK`, `V380`, `IPCAM`, `IPC365`, `LOOKCAM`, `WIFICAM`, `HDWIFICAM`, `TAPO_CAM` y `YI-`.
- Genéricos (confianza **MEDIA**): `CAMERA`, `CAM-`, `IPC` y `JXL`.

Se ignoran las demás redes. No se incluye `SMARTLIFE` porque también lo usan enchufes y bombillas inteligentes.

Con muchas redes de vecinos, la tabla de 64 aparatos WiFi se llena: entonces se olvida el aparato corriente visto hace más tiempo, nunca un sospechoso ni el que estás buscando.

## Pantallas

Desliza a izquierda o derecha, igual que en Vigía MS.

| Pantalla | Qué muestra |
|---|---|
| ESCANEO | «Nada sospechoso» verde, número de posibles cámaras ámbar o «¡Cámara probable!» rojo si hay ALTA o vídeo. Recuentos de dispositivos WiFi, redes de cámara y Bluetooth; progreso de canales. |
| RADAR | Puntos numerados: una señal más fuerte queda más cerca del centro. El ángulo es decorativo; una antena no permite conocer la dirección. |
| LISTA | Hasta seis pistas, primero las más relevantes: #n, fabricante o «Chip genérico», ALTA/MEDIA/BAJA, etiqueta «vídeo» y barras de señal. Toca una para buscarla. Los ignorados quedan al final. |
| BUSCAR | «FRÍO / TEMPLADO / ¡CALIENTE!», barra grande y pitidos que se aceleran con el RSSI suavizado. Fija el canal de la pista WiFi y suspende los barridos de redes. Muestra tramas y kB realmente recibidos en los últimos 10 s. BLE no tiene canal WiFi que fijar. |
| CONSEJOS | Linterna del móvil para buscar reflejos, lugares habituales donde mirar y qué hacer si encuentras una cámara. |
| AJUSTES | Sonido, brillo automático mediante LDR y recalibración táctil. Estado de radios, pérdidas de cola y tabla llena. |

«Es mío / ignorar» excluye esa pista del veredicto hasta reiniciar. Puedes volver a activarla con el mismo botón. Sin señal reciente, BUSCAR deja de pitar y muestra un aviso. Una pista WiFi y su red se fusionan si comparten dirección; WiFi y BLE de un mismo equipo pueden contar por separado.

| LED | Significado |
|---|---|
| Latido verde | Sin sospechosos detectados |
| Latido azul | Hay posibles cámaras |
| Rojo parpadeando | Hay ALTA o tráfico compatible con vídeo |
| Amarillo intermitente | Fallo de radio: revisar AJUSTES |

## Privacidad y límites

- **Solo escucha:** WiFi promiscuo, barridos de redes pasivos y BLE pasivo. Nunca se conecta, pide respuestas de escaneo, envía desautenticaciones ni intenta entrar en un equipo.
- **Datos solo en RAM:** direcciones usadas para agrupar señales, horas relativas, RSSI y contadores. No guarda paquetes ni contenido. No muestra MAC, IP, SSID ni nombres Bluetooth; cada pista recibe un número. La salida serie contiene solo fabricante, RSSI y confianza. El barrido libera sus resultados al terminar.
- **Solo persisten ajustes:** sonido, brillo y calibración en Preferences, en un espacio separado de Vigía. Las detecciones y marcas de ignorado desaparecen al reiniciar.
- **No detecta cámaras que solo graban en SD ni cámaras por cable.** Tampoco WiFi de 5 GHz, radios apagadas o dispositivos que no estén emitiendo mientras se escucha su canal. Una cámara que usa un chip desconocido puede pasar inadvertida.
- **La lista OUI no es exhaustiva.** OUI identifica un titular, no un modelo. Las marcas usan chips y fabricantes externos; las direcciones locales o aleatorias no se atribuyen a una marca. ALTA no es una prueba de espionaje y MEDIA incluye móviles, routers, altavoces y otros equipos normales.
- **Una cámara del vecino también aparece.** Usa BUSCAR para comprobar si la señal crece dentro de la habitación. Paredes, reflejos y el cuerpo alteran la señal; no da metros ni una posición real.
- **El vídeo es solo una estimación.** Salta por los canales 1–13 unos 250 ms por canal. Calcula la tasa con el tiempo escuchado en cada canal, no con toda la vuelta; los contadores de 10 s muestran solo lo recibido, con resolución de un segundo. Excluye del cálculo de subida la bajada del AP, los reintentos y las tramas nulas. Una subida normal también puede parecer vídeo. No descifra ni reconoce imágenes. Las pausas de barrido, calibración, pérdida de cola o cambio de canal invalidan la continuidad; puede tardar más de una vuelta en volver a detectar vídeo. WiFi y BLE comparten radio, así que pueden perderse señales incluso sin desbordar la cola.
- Tablas acotadas: 64 transmisores/redes WiFi, 16 pistas BLE y 96 direcciones BLE para el recuento general. Presencia durante 60 s (BLE general: 30 s); las entradas no ignoradas ni seleccionadas se pueden sustituir tras 3 min sin señal. Ignorados y seleccionado se conservan hasta reiniciar. Si se llena una tabla de pistas, AJUSTES lo avisa; puede faltar alguna señal. Una pista MEDIA pierde continuidad si pasan más de 6 s sin oírla.
- Apaga la luz y busca reflejos de lentes con la linterna del móvil. Mira detectores de humo, cargadores USB, relojes, enchufes y espejos. Si encuentras una cámara, **no la toques**: haz una foto y avisa a la plataforma y a la policía. Para ayuda urgente en España: **112**.

## Probarlo sin una cámara

1. En un móvil, crea un punto de acceso de **2,4 GHz** llamado **`V380-TEST`**. La CYD no necesita su contraseña y no se conectará.
2. Deja ESCANEO abierto durante unos 25 s. Debe subir «redes cámara» y aparecer una pista MEDIA con la etiqueta «Red de cámara tipo V380». Nunca debe salir el nombre completo de la red ni ninguna dirección.
3. Toca la pista en LISTA. BUSCAR debe fijar su canal; acerca y aleja el móvil para ver cómo cambia la barra y el ritmo de pitidos. Paredes y reflejos pueden alterar el resultado.
4. Toca «Es mío / ignorar»: esa pista deja de contar. Vuelve a activarla con el botón; reiniciar debe borrar el ignorado y conservar sonido, brillo y calibración.
5. Apaga el punto de acceso: BUSCAR debe indicar «Sin señal reciente» tras unos 6 s y dejar de pitar; los recuentos pueden tardar hasta 60 s en caducar.

Esta prueba comprueba la pista de **nombre de red**, no el reconocimiento de una cámara real ni el tráfico de vídeo. Para BLE se puede anunciar un nombre como `V380-TEST` con una herramienta de pruebas; el detector nunca envía una solicitud de respuesta. Para vídeo, hace falta una fuente propia con subida sostenida, escuchar su canal durante más de 10 s y comparar también con una descarga normal. Estas pruebas físicas siguen pendientes.

## Hardware y pines

Mismo montaje y ayudantes de texto, táctil, LED, brillo y sonido que [Vigía MS](../vigia-ms-cyd/README.md).

| Elemento | Pines GPIO |
|---|---|
| Pantalla ILI9341 (HSPI) | SCLK 14, MISO 12, MOSI 13, CS 15, DC 2, retroiluminación 21 (PWM) |
| Táctil XPT2046 (VSPI) | CLK 25, MISO 39, MOSI 32, CS 33, IRQ 36 |
| LED RGB (activo a nivel bajo) | R 4, G 16, B 17 |
| Sensor de luz (LDR) | 34 |
| Altavoz | 26 |
| microSD | CS 5 en alto; no se monta |

Referencia: [ESP32-Cheap-Yellow-Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md). Dominio WiFi **ES**, canales 1–13: este montaje está pensado para España.

## Compilar y cargar

1. Abre `CazaCamarasMS/CazaCamarasMS.ino` en AB MakerIDE SUPER o Arduino IDE.
2. Elige **ESP32 Dev Module**, núcleo esp32 **3.x**, partición **Huge APP / huge_app** para una CYD de 4 MB.
3. Instala **Adafruit GFX**, **Adafruit ILI9341**, **XPT2046_Touchscreen** (Paul Stoffregen) y **NimBLE-Arduino 2.x** (h2zero). SPI, Preferences, WiFi y esp_wifi pertenecen al núcleo. La API BLE se ha revisado con los encabezados locales de NimBLE 2.3.7; la compilación con el núcleo sigue pendiente.
4. Compila. La carga en la placa es un paso manual posterior: **en este trabajo no se ha grabado la placa**. El primer arranque pide tocar tres cruces; la calibración queda guardada.

Desde la raíz del repositorio, PowerShell:

```powershell
& "C:\Program Files\Arduino CLI\arduino-cli.exe" compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app --build-path examples/cazacamaras-ms-cyd/.build examples/cazacamaras-ms-cyd/CazaCamarasMS
```

Conserva `btInUse()` para que el núcleo 3.x no libere la memoria que usa NimBLE al arrancar. No añadas `WiFi.begin()`, escaneo activo ni impresión de resultados de redes.

La auditoría estática se puede repetir con Node 22 o posterior:

```powershell
node examples/cazacamaras-ms-cyd/tests/auditar.mjs
```

Las pruebas C++ de las reglas reales están en `tests/reglas_test.cpp`; requieren un compilador C++17 de escritorio y **no se han ejecutado en este entorno**. La auditoría estática no las sustituye. El generador `tests/generar_oui.mjs` descarga el registro público IEEE y reescribe la tabla, su extracto y OUI.md; hay que revisar los cambios antes de usarlos.

## Evidencia

- 9 nov 2026: OUI contrastados con el registro IEEE MA-L; nombres Foscam/Anyka revisados también en MA-M y MA-S sin asignación verificable bajo esos nombres. Revisión estática de pines, prefijos, patrones, privacidad y límites de textos en [spec/evidence.md](spec/evidence.md).
- 9 nov 2026: compilado con el núcleo esp32 3.3.12 y esquema **Huge APP** (1 224 411 bytes de programa, 38 %; 79 160 bytes de RAM, 24 %) y grabado en una CYD por COM4 con escritura verificada por hash.
- Prueba real en casa del autor con una **cámara EZVIZ** propia: LISTA mostró «Red de cámara tipo EZVIZ · ALTA», ESCANEO «¡Cámara probable!» con el LED rojo, y BUSCAR pasó de TEMPLADO (−60 dBm) a «¡CALIENTE!» (−51 dBm, muy cerca) al acercarse. La placa veía a la vez 24 aparatos WiFi y 6-7 Bluetooth de alrededor. Calibración táctil, LED y altavoz funcionando.
- Tras esa prueba se corrigieron tres cosas: la tabla de aparatos ya no se llena con los vecinos, «SMARTLIFE» dejó de contar como cámara y los nombres de red se comparan solo por el principio. También la confianza ALTA de la red ya no se pierde entre escaneos.

**Pendiente de comprobar:** la detección de una cámara solo por su fabricante y su tráfico de vídeo, sin la red de configuración (la EZVIZ de la prueba la mantenía activa). No es una validación con todas las marcas ni modelos.

## Licencia

MIT, ver [LICENSE](LICENSE), copiado del proyecto Vigía. Pantalla, calibración, colores y gestos proceden de sus ayudantes. Las bibliotecas se instalan aparte y conservan sus licencias. Los nombres de fabricantes y productos son marcas de sus propietarios; este proyecto no está afiliado a ellos.
