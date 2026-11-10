# Gasolina MS · CYD

[← Catálogo](../../README.md) · [AB MakerIDE SUPER](https://github.com/msrobotika/AB-MakerIDE-SUPER)

Las **gasolineras públicas más baratas de tu pueblo o cerca de ti**, con precios oficiales, para la **CYD ESP32-2432S028R** (ILI9341 horizontal 320 × 240 y táctil resistivo XPT2046). Funciona en **España, Francia y Austria**. Sin cuenta ni clave de API. Elige país, zona, combustible y ubicación automática por Wi-Fi o solo tu zona.

Hecho con AB MakerIDE. Un único archivo: [GasolinaMS/GasolinaMS.ino](GasolinaMS/GasolinaMS.ino). Pantalla, pines, calibración, colores, brillo y gestos adaptados de Kilovatio MS. No se necesita tarjeta SD ni PSRAM.

## Pantallas

Desliza a izquierda o derecha. Los puntos inferiores indican la pantalla.

| Pantalla | Qué muestra |
|---|---|
| CERCA (0) | Con posición conocida, la más barata en 5 km y la más cercana de la provincia consultada. Amplía a 10 y 20 km si hace falta, indicando el radio; precio, dirección, distancia y ahorro en 50 L frente a la cercana. |
| AHORA | Marca y dirección de la estación pública más barata, precio en €/L, combustible, céntimos por debajo de la media y ahorro orientativo para los litros elegidos. Hora oficial del dato; si corresponde a otra fecha, indica «Datos anteriores» con fecha y hora. |
| TOP 5 | Hasta cinco estaciones ordenadas de menor a mayor precio, de verde a ámbar. Solo aparecen las que tienen precio válido y venta pública. |
| TENDENCIA | Mínimo y media durante los últimos 14 días naturales. Compara hoy con ayer; si falta ayer no usa otro día. Con menos de dos muestras pide volver mañana. Los huecos no se rellenan ni se unen en la gráfica. |
| AJUSTES | Combustible y litros actuales, actualizar ahora, QR de la web local, brillo automático o fijo y recalibración táctil. Mantén 2 s el botón Wi-Fi para abrir el portal. |

La actualización es cada **30 minutos** y al tocar **Actualizar ahora**. Si falla, aparece un error con **Reintentar**; además reintenta a los 2 minutos. Una respuesta incompleta no sustituye la última descarga válida. Sin estaciones públicas con precio, lo indica sin inventar un ahorro.

Ese intervalo corresponde al pueblo. CERCA renueva la posición cada 2 minutos y los precios provinciales cada 60 minutos; su caché se invalida si una descarga queda incompleta.

## Países

| País | Fuente oficial (datos abiertos) | Clave | Zona de AHORA / TOP 5 / TENDENCIA | CERCA |
|---|---|---|---|---|
| España | [Ministerio para la Transición Ecológica](https://sedeaplicaciones.minetur.gob.es/ServiciosRESTCarburantes/PreciosCarburantes/) | No | Municipio (provincia → pueblo) | Provincia aproximada, radio 5/10/20 km |
| Francia | [prix-carburants.gouv.fr](https://www.prix-carburants.gouv.fr/) vía [data.economie.gouv.fr](https://data.economie.gouv.fr/explore/dataset/prix-des-carburants-en-france-flux-instantane-v2/) | No | Código postal | Las 100 más cercanas en 20 km |
| Austria | [E-Control Spritpreisrechner](https://www.e-control.at/spritpreisrechner) | No | Distrito (Bezirk) | Búsqueda por punto de E-Control |

El país se elige en la web de AJUSTES (primer campo). **España es el valor por defecto**: una placa que ya funcionaba sigue igual. El país se guarda en su propia clave de Preferences (`pais`); el formato de la configuración no cambia. Al cambiar de país se reinicia la tendencia y se vuelve a localizar.

- **Combustibles:** España: Gasolina 95 E5, 98 E5, Gasóleo A, Gasóleo Premium y GLP. Francia: Gazole, SP95, SP95-E10, SP98 y GPLc. Austria: Diésel, Super 95 y GNC.
- **Francia:** escribe un código postal de 5 cifras; la placa comprueba que tiene gasolineras publicadas y toma el nombre de la ciudad. Cada consulta pide como mucho 100 estaciones: AHORA/TOP 5 usan las del código postal y CERCA las 100 más cercanas en 20 km, ordenadas por distancia. Las direcciones aparecen en lugar de la marca, porque el conjunto de datos no la incluye.
- **Austria:** elige el distrito en la lista de E-Control (117 distritos; «Wien(Stadt)» se omite porque no devuelve gasolineras: usa los 23 distritos de Viena). **Por ley, E-Control solo publica las ~10 gasolineras más baratas de cada consulta**: la media de AHORA es la de esas más baratas y la pantalla lo indica.
- **Mismas reglas en los tres países:** precios orientativos (confirma en el surtidor), distancias en línea recta, sin horarios, y CERCA usa el centro de tu zona si las redes Wi-Fi no dan la ubicación.
- **TLS:** cada servicio con sus raíces reales, nunca `setInsecure`. Francia: **USERTrust RSA Certification Authority**, SHA-256 `E793C9B02FD8AA13E21C31228ACCB08119643B749C898964B1746D46C3D4CBD2`. Austria: **HARICA TLS RSA Root CA 2021**, `D95D0E8EDA79525BF9BEB11B14D2100D3294985F0C62D9FABD9CD999ECCB7B1D`, y **Hellenic Academic and Research Institutions RootCA 2015** (la firma cruzada que envía el servidor), `A040929A02CE53B4ACF4F2FFC6981CE4496F755E6D45FE0B2A692BCD52523F36`. Ambas cadenas se verificaron con OpenSSL contra los servidores reales (`Verify return code: 0`) y desde la placa.

## Cerca de mí (ubicación por Wi-Fi)

Pensado para llevar la CYD en el coche conectada al hotspot del móvil. En **Ubicación** puedes elegir **Automática por Wi-Fi** (por defecto) o **Solo mi pueblo**. El pueblo elegido se conserva como respaldo y sigue disponible en AHORA/TOP 5; TENDENCIA sigue siendo la del pueblo. El primer éxito de ubicación abre CERCA. Toca CERCA o **Actualizar ahora** en AJUSTES para localizar de nuevo; ese botón también actualiza precios de provincia y pueblo.

Cada **2 minutos**, sin desconectarse, escanea y toma hasta **15 redes más fuertes**. Excluye el punto de acceso conectado (el hotspot se mueve contigo), redes cuyo nombre termina en `_nomap` y MAC locales/aleatorias. Envía MAC y potencia a [beaconDB](https://beacondb.net/), servicio comunitario abierto compatible con Mozilla Location Service, gratuito y sin clave. No usa GPS ni ubicación por IP (`considerIp:false`). Un 404 significa que no hay estimación. Si beaconDB no conoce las redes de la zona (pasa en muchos pueblos), CERCA mide las distancias **desde el centro de tu pueblo** (media de las coordenadas de sus gasolineras) y lo indica con **«Sin ubicación Wi-Fi: distancias desde tu pueblo»**. Sigue intentándolo cada 2 minutos y cambia a la ubicación por Wi-Fi en cuanto la consigue. Con una posición Wi-Fi anterior, la mantiene dos fallos (señalada como tal) antes de pasar al centro del pueblo. Si el pueblo no tiene gasolineras con coordenadas, muestra **«Sin ubicación: pocas redes Wi-Fi aquí»**.

La precisión orientativa es **≈50–200 m en pueblos y ciudades**, según cobertura; se muestra la incertidumbre devuelta por el servicio. **En autovías o campo sin Wi-Fi alrededor puede no saber dónde estás.** La tabla integrada de las 52 capitales (IDs INE 01..52, incluidas Ceuta y Melilla) elige la más cercana como provincia aproximada. Puede equivocarse cerca de límites provinciales; solo se comparan estaciones de esa provincia, sin consultar las vecinas. Distancias en línea recta, no por carretera: no incluyen desvíos, gasto del viaje ni apertura actual.

La placa descarga la provincia y el producto con `Accept: application/json`, usando el lector TLS de 1 kB y ArduinoJson Filter **estación a estación**, sin guardar el cuerpo completo. Mantiene hasta **1200 estaciones compactas** en RAM (~82 kB): latitud/longitud, precio en milésimas, rótulo y dirección corta. Solo venta pública `P` con precio y coordenadas válidos. Si supera el límite o se corta la respuesta, avisa y descarta la descarga; no presenta una lista parcial como completa. Al cargar listas de configuración se libera temporalmente esta caché para dejar RAM a los listados ya probados y se recupera después. Desactivar automático libera la caché y borra la posición en RAM.

**Privacidad:** envía la lista de redes Wi-Fi cercanas —**MAC y señal, sin nombres**— a beaconDB; no envía tu nombre, cuenta, credenciales, el BSSID conectado ni un identificador de esta placa. Las redes observadas y la posición se usan en RAM: no se guardan en Preferences ni aparecen en pantalla o serie. En serie, la posición solo genera `GASOLINA beacondb codigo=NNN redes=N`, `GASOLINA posicion ok por Wi-Fi`, `GASOLINA posicion ok centro del pueblo` o `sin estimacion`; se mantienen los mensajes municipales y los comandos de captura. [beaconDB declara que no almacena las MAC consultadas ni la posición devuelta](https://beacondb.net/privacy/); como servidor de Internet sí ve la IP pública y conserva logs HTTP de IP/fecha/URL/User-Agent durante 28 días. La elección de modo se guarda en la placa junto a las preferencias habituales.

**Consumo orientativo:** ≈**320 kB por provincia y hora** + **~1 kB por posición**, además de TLS/cabeceras, las consultas de pueblo y los listados. Cambiar de provincia, combustible, pulsar actualizar o recargar la caché tras configurar puede generar descargas adicionales. Crédito a **beaconDB** y a su comunidad por el servicio de localización.

TLS de beaconDB usa las raíces **ISRG Root X2 y Root YE reales concatenadas**, obtenidas de [Let's Encrypt](https://letsencrypt.org/certificates/). Huellas SHA-256 verificadas contra [ISRG CP/CPS](https://letsencrypt.org/documents/isrg-cp-cps-v6.1/):

- X2: `69729B8E15A86EFC177A57AFB7171DFC64ADD28C2FCA8CF1507E34453CCB1470`
- YE: `E14FFCAD5B0025731006CAA43A121A22D8E9700F4FB9CF852F02A708AA5D5666`

Se verificó además la firma de YE cruzada por X2 y la conexión TLS real de escritorio a `api.beacondb.net` con estas CA. Sin marcadores ni TLS inseguro. Probada también en el ESP32 el 10 nov 2026 (respuesta HTTP real de beaconDB).

Capturas USB: `MSR_TAB 0` CERCA, `1` AHORA, `2` TOP 5, `3` TENDENCIA, `4` AJUSTES; responde `MSR_CMD_OK`. `MSR_SHOT` conserva el envío RGB565 por franjas. Todos los dibujos pasan por `gfx`.

| LED RGB | Significado |
|---|---|
| Verde | Descarga correcta; mínimo de hoy más barato o igual que ayer. Si todavía no hay comparación válida, solo indica descarga correcta. |
| Ámbar | Mínimo de hoy más caro que el de ayer. |
| Rojo intermitente | Error de actualización o de guardado del histórico. |
| Azul | Portal de configuración Wi-Fi. |
| Apagado | Sin selección o sin precios públicos disponibles. |

El histórico es propio de esta placa: **no descarga precios históricos del Ministerio**. Guarda una muestra por día, después de las 08:00 (hora peninsular), y la reemplaza con la última descarga válida del día. Solo registra datos cuya fecha oficial sea la de hoy. Se conserva en NVS al reiniciar. Cambiar de pueblo o combustible reinicia el histórico; cambiar solo los litros lo conserva.

## Primera puesta en marcha y configuración

1. Toca el centro de las tres cruces para calibrar. Se guarda en NVS y se puede repetir desde AJUSTES.
2. Sin credenciales guardadas, la placa crea **Gasolina-MS**, con contraseña nueva, y muestra un QR para conectarte desde el móvil. El LED está azul.
3. Abre el portal que ofrece el móvil; si no aparece, entra en `http://192.168.4.1/`. Escribe el nombre de tu red de **2,4 GHz** y su clave. El portal solo configura Wi-Fi. La placa se reinicia y sincroniza su reloj por NTP.
4. Conecta el móvil a tu red habitual. La placa muestra **«Elige tu pueblo: escanea el QR»**. Ese QR y el de AJUSTES abren la web de la placa en su IP local. No confundirlo con el QR para conectarse al AP.
5. Elige el **país** y tu zona (España: provincia y municipio; Francia: código postal; Austria: distrito), modo de ubicación, combustible y litros del depósito (**50 L** por defecto; de 1 a 200 L). La placa obtiene los listados oficiales y comprueba la pertenencia del municipio antes de guardar. Si falla una lista, usa **Recargar listas**.
6. Pulsa Guardar. La selección queda en Preferences y se solicita una descarga nueva. Los cambios de litros se reflejan en el ahorro.

El móvil y la placa deben estar en la misma red; el aislamiento de clientes o una red de invitados pueden impedir acceder. La URL de ajustes se imprime **una sola vez al arrancar conectado**, a 115200 baudios; el QR se adapta si el router cambia la IP. No necesita Internet el navegador: las peticiones al Ministerio las hace la placa. Sí necesita salida a Internet la CYD y hora NTP válida para verificar HTTPS.

Si no conecta tras unos 20 segundos, abre de nuevo el portal. Para cambiar de Wi-Fi, mantén 2 s su botón en AJUSTES. La nueva red solo se escribe al enviar el formulario; el dispositivo conserva las credenciales anteriores hasta entonces.

## Datos y licencia de los datos

Fuente: **Ministerio para la Transición Ecológica y el Reto Demográfico**, Servicios REST de Carburantes, difundidos como datos abiertos. Consulta también [datos.gob.es: reutilización de información de carburantes](https://datos.gob.es/es/noticias/reutilizacion-de-informacion-del-sector-publico-apps-de-combustible).

Base: `https://sedeaplicaciones.minetur.gob.es/ServiciosRESTCarburantes/PreciosCarburantes`.

| Consulta | Ruta |
|---|---|
| Provincias | `/Listados/Provincias/` (campo oficial `IDPovincia`, con esa errata) |
| Municipios de provincia | `/Listados/MunicipiosPorProvincia/{IDProvincia}` |
| Productos | `/Listados/ProductosPetroliferos/` |
| Precios del municipio y producto | `/EstacionesTerrestres/FiltroMunicipioProducto/{IDMunicipio}/{IDProducto}` |
| Precios de provincia y producto | `/EstacionesTerrestres/FiltroProvinciaProducto/{IDProvincia}/{IDProducto}` |

Todas las peticiones envían `Accept: application/json`. Los precios usan coma decimal. Se excluyen venta restringida (`R`), venta distinta de `P`, precios vacíos y precios mal formados o no positivos. La media es aritmética de **todas** las estaciones públicas con precio válido, no solo de las cinco primeras. El ahorro es `(media − mínimo) × litros`, sin sumar desplazamiento ni descuentos particulares.

| ID verificado el 9 nov 2026 | Combustible |
|---|---|
| 1 | Gasolina 95 E5 |
| 3 | Gasolina 98 E5 |
| 4 | Gasóleo A (nombre oficial: Gasóleo A habitual) |
| 5 | Gasóleo Premium |
| 17 | GLP (nombre oficial: Gases licuados del petróleo) |

IDs confirmados contra el [listado oficial de productos](https://sedeaplicaciones.minetur.gob.es/ServiciosRESTCarburantes/PreciosCarburantes/Listados/ProductosPetroliferos/) con cabecera JSON. La respuesta de Madrid se comprobó por TLS y HTTP/1.0 sin codificación chunked. El sketch analiza los precios con **ArduinoJson 7 Filter por estación**, sin cargar la respuesta completa. El modo de pueblo mantiene cinco estaciones, recuento, suma, mínimo y máximo; CERCA añade el array provincial en RAM. No conserva horarios: una estación publicada puede estar cerrada a esa hora.

HTTPS valida nombre de servidor, fechas y cadena con `setCACert`. Incluye la **DigiCert Global Root G2 real**, obtenida del almacén de confianza distribuido con Node, y contrastada con la [huella publicada por DigiCert](https://knowledge.digicert.com/general-information/digicert-trusted-root-authority-certificates):

`CB3CCBB76031E5E0138F8DD39A23F9DE47FFC35E43C1144CEA27D46A5AB1CB5F`

No hay marcador pendiente ni conexión insegura. La conexión de escritorio al servicio con **solo esta CA** funcionó. Si el Ministerio cambia la cadena, habrá que revisar la CA del sketch.

La reutilización de los datos está permitida **citando la fuente**, indicando su fecha de actualización, sin desnaturalizarlos ni sugerir patrocinio o respaldo del organismo. Consulta las condiciones originales del Ministerio y [datos.gob.es](https://datos.gob.es/es/noticias/reutilizacion-de-informacion-del-sector-publico-apps-de-combustible). La licencia MIT del programa no cambia las condiciones de los datos.

## Privacidad y límites

- Solo mi pueblo usa el municipio elegido; automático añade la localización Wi-Fi descrita arriba. Sin GPS ni ubicación del móvil. El Ministerio recibe las consultas de provincia, municipio y producto desde tu conexión.
- Pantalla y serie no muestran el nombre de tu Wi-Fi. El formulario de alta necesita que lo escribas, pero no lo devuelve en la confirmación ni lo copia a los ajustes LAN.
- Credenciales, calibración, selección y tendencia se guardan en la placa (NVS). Este sketch no cifra NVS. La web de configuración usa HTTP local, sin recursos externos; úsala en una red de confianza. El AP lleva clave aleatoria.
- Una línea serie por intento: municipio, producto, recuento, mínimo, media, fecha/hora oficial y estado. En error usa cifras cero y hora `--`; no presenta las cifras anteriores como recién descargadas.
- La fecha de la API corresponde al conjunto publicado; no acredita cuándo cambió cada surtidor. La renovación de la placa no acelera la declaración de precios por las estaciones.
- El pie recuerda: **«Precios oficiales del Ministerio (datos abiertos). Pueden tardar en actualizarse: confirma en el surtidor.»** No se garantiza un precio, apertura actual, disponibilidad ni condiciones de pago.
- La zona horaria es Europe/Madrid, como Kilovatio: en Canarias la hora mostrada es peninsular. Si no hay hora válida, no se omite la validación TLS: aparece un error y se reintenta.
- Mientras se realiza una consulta HTTPS, el servidor y el táctil pueden esperar al tiempo de red (15 s de espera por lectura). Rendimiento, consumo real de memoria y lectura de QR pendientes en placa.

## Hardware y pines

| Elemento | Pines GPIO |
|---|---|
| Pantalla ILI9341 (HSPI) | SCLK 14, MISO 12, MOSI 13, CS 15, DC 2, retroiluminación 21 (PWM) |
| Táctil XPT2046 (VSPI) | CLK 25, MISO 39, MOSI 32, CS 33, IRQ 36 |
| LED RGB, activo a nivel bajo | R 4, G 16, B 17 |
| Sensor de luz (LDR) | 34, brillo automático |
| Altavoz | 26, reservado; no se usa |
| microSD | CS 5 en alto; no se monta |

ILI9341 a 40 MHz, rotación 1 en LCD y táctil. Calibración afín de tres puntos, igual que Kilovatio. Brillo fijo 230/255 o automático con LDR (mínimo 35/255).

Referencia: [ESP32-Cheap-Yellow-Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md). Otras revisiones, pantallas ST7789 o modelos capacitivos necesitan adaptación.

## Compilar y cargar

1. Abre `GasolinaMS/GasolinaMS.ino` en AB MakerIDE SUPER.
2. Selecciona **ESP32 Dev Module** (`esp32:esp32:esp32`, núcleo esp32 **3.x**).
3. Instala **Adafruit GFX**, **Adafruit ILI9341**, **XPT2046_Touchscreen** (Paul Stoffregen) y **ArduinoJson 7**. WiFi, WebServer, DNSServer, Preferences, HTTPClient, WiFiClientSecure y el QR se incluyen con esp32. No requiere pestañas `.h` propias.
4. Compila con el esquema de particiones por defecto: ocupa el **96 %** (1 264 047 bytes, núcleo esp32 3.3.12). No hace falta Huge APP.
5. Selecciona tu puerto y carga. Cargar sustituye el firmware de la placa.

Desde la raíz del repositorio, PowerShell:

```powershell
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' compile --fqbn esp32:esp32:esp32 --build-path examples/gasolina-ms-cyd/.build examples/gasolina-ms-cyd/GasolinaMS
```

Solo si informa de falta de espacio, cambia el FQBN a `esp32:esp32:esp32:PartitionScheme=huge_app`. Borra la carpeta `.build` de este proyecto al terminar.

Comprobaciones de escritorio, con Node 24:

```powershell
node examples/gasolina-ms-cyd/tests/verificar.mjs
# Añadir --red verifica también los cuatro servicios y TLS reales.
node examples/gasolina-ms-cyd/tests/cerca.mjs --red
# CA, regresiones, lógica traducida a JS, TLS/404 beaconDB sin redes reales y provincia Madrid.
```

## Evidencia

- **10 nov 2026, en la placa real (COM4):** España (Madrid, Gasolina 95 E5): AHORA 232 gasolineras, mínimo 1,499 €/L; CERCA sin ubicación Wi-Fi (beaconDB respondió 404 con 15 redes en esa zona) usando el centro del pueblo: la más barata Alcampo 1,529 €/L a 2,7 km y la más cercana Repsol 1,814 €/L a 0,5 km. Francia (75015, Gazole): 6 gasolineras, mínimo 2,250 €/L; CERCA con las de alrededor de París (2,129 €/L a 4,9 km). Austria (Wien 10., Diésel): 9 gasolineras, mínimo 2,082 €/L; acentos alemanes (ö, ß) correctos. Portal de ajustes con los tres países probado desde la red local.
- Arreglos encontrados al probar en la placa: la pila de `loop()` (8 kB) se desbordaba con TLS + JSON de Francia → 16 kB (`SET_LOOP_TASK_STACK_SIZE`); CERCA reservaba 1 200 estaciones también en Francia/Austria → 100; la captura por USB usa franjas de 24 líneas (15 kB) para caber con la provincia cargada; la lista de distritos salía con un solo elemento (`serializeJson` sobrescribe el String).
- Pruebas de escritorio del 10 nov: `verificar.mjs` (5 escenarios de la página de ajustes, incluidos Francia y Austria) y `cerca.mjs` (6 raíces CA, regresiones y la lógica del centro del pueblo) pasan. Las huellas de `drawNow`, `drawTop`, `sendScreenshot` y `Strip` se actualizaron tras revisar sus cambios.
- 9 nov 2026, ampliación: ajustes JS reales comprobados (incluye token fresco y ambos modos); diez bloques del sketch comparados por SHA-256 con la base respaldada. Pruebas de lógica traducida a JavaScript para distancias, radios, selección, capitales y tres fallos; no equivalen a ejecutar C++/ArduinoJson. TLS real con las CA embebidas, beaconDB 404 sin redes enviadas y provincia Madrid: 859 estaciones públicas, 321 529 bytes. [Evidencia y límites](specs/near-me/verification.md). Compilación nueva bloqueada por sandbox; sin carga en placa.
- 9 nov 2026: compilado con el núcleo esp32 3.3.12 y la partición por defecto (1 219 339 bytes de programa, 93 %) y grabado en una CYD por COM4 con escritura verificada por hash.
- En la placa real, con Madrid y Gasolina 95 E5: 233 gasolineras públicas descargadas del Ministerio, mínimo 1,529 €/L y media 1,717 €/L (datos de las 18:28 y 18:45). Portal Wi-Fi, página de ajustes desde el móvil, pantallas AHORA y TOP 5 y capturas por USB comprobados.
- Tras la primera prueba se corrigieron cuatro cosas: la lista de pueblos llegaba rota y tardaba ~25 s (ahora se descarga entera, menos de 1 s); la página de ajustes no respondía mientras se leían los precios byte a byte (ahora se leen a trozos de 1 kB); al guardar se volvía a pedir la lista al Ministerio (ahora se usa la copia recién enviada); y si la placa se reiniciaba, guardar decía «Vuelve a abrir los ajustes» (ahora pide un permiso nuevo en cada guardado).
- Detalle de la red verificada desde el escritorio en [specs/red-verificada.json](specs/red-verificada.json) y Spec Kit en [specs/](specs/).

**Pendiente de comprobar:** la pantalla TENDENCIA con varios días de datos, la estabilidad durante días y la ubicación por Wi-Fi en una zona que beaconDB conozca (en la zona de prueba no tenía datos).

## Licencia

MIT, copia exacta de [LICENSE](LICENSE) de Kilovatio MS. Ayudantes de pantalla y calibración de AB MakerIDE / MS Robotika, también MIT. Las bibliotecas conservan sus licencias. Los precios abiertos pertenecen a la fuente oficial y se reutilizan con atribución; el Ministerio no participa ni respalda este proyecto.
