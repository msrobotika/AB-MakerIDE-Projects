# GhostGuard CYD v1.4 · Cyber Green

**MS Robotika · AB MakerIDE SUPER** — monitor de telemetría del PC en una pantalla táctil ESP32 CYD, conectado por USB serie.

## Descarga

[Descargar paquete completo ZIP](../../downloads/GhostGuard_CYD_v1_4_LISTO_PARA_GITHUB.zip)

El ZIP contiene el sketch `GhostGuard_CYD/GhostGuard_CYD.ino`, el agente Windows `windows_agent/ghostguard_agent.py`, scripts de instalación/arranque, dependencias, pruebas y documentación. [SHA-256 del ZIP](../../downloads/GhostGuard_CYD_v1_4_LISTO_PARA_GITHUB.zip.sha256).

Los mismos archivos están en esta carpeta para leerlos sin descargar:

| Archivo | Qué es |
|---|---|
| [`GhostGuard_CYD/GhostGuard_CYD.ino`](GhostGuard_CYD/GhostGuard_CYD.ino) | Firmware de la CYD |
| [`windows_agent/ghostguard_agent.py`](windows_agent/ghostguard_agent.py) | Agente de telemetría para Windows |
| [`windows_agent/instalar.bat`](windows_agent/instalar.bat) · [`iniciar_COM4.bat`](windows_agent/iniciar_COM4.bat) | Instalación de dependencias y arranque en COM4 |
| [`tests/test_protocol.py`](tests/test_protocol.py) | Prueba del protocolo serie |
| [`NOTAS_VERSIONES.md`](NOTAS_VERSIONES.md) | README original del paquete con los cambios v1.0 → v1.4 |

## Vídeo

[▶ Ver GhostGuard v1.4 funcionando en la CYD (79 s)](../../docs/media/GhostGuard-CYD-v1.4.mp4)

## Hardware

- ESP32-2432S028R (CYD), ILI9341 240×320, XPT2046.
- Conexión USB de datos al ordenador Windows.
- Núcleo ESP32 Arduino y bibliotecas Adafruit GFX, Adafruit ILI9341, XPT2046_Touchscreen.
- Configuración de placa utilizada: `esp32:esp32:jczn_2432s028r` (confirmar compatibilidad con la revisión física).
- Pantalla: HSPI `SCK=14, MISO=12, MOSI=13, CS=15, DC=2`. Luz GPIO21. Táctil: `SCK=25, MISO=39, MOSI=32, CS=33, IRQ=36`.

## Instalación y arranque

1. Descarga y descomprime el ZIP.
2. Abre `GhostGuard_CYD/GhostGuard_CYD.ino` en AB MakerIDE SUPER o Arduino IDE.
3. Instala las bibliotecas necesarias, selecciona la placa y puerto correctos, compila y graba.
4. En Windows, instala Python 3 y ejecuta `windows_agent/instalar.bat`, o `python -m pip install -r windows_agent/requirements.txt`.
5. Identifica el puerto COM de la CYD; el script `windows_agent/iniciar_COM4.bat` presupone COM4. Ajusta el puerto si es distinto.
6. Ejecuta el agente con la placa conectada. **Cierra antes el monitor serie**, porque el puerto no puede estar ocupado por dos aplicaciones.
7. Verifica que la pantalla muestra `PC LIVE` y datos actualizados. La calibración táctil de tres puntos se guarda en memoria de la placa.

## Funciones y límites

Interfaz Cyber Green con páginas HOME, RED, EVENTOS y ESP32; datos de carga y red desde el PC, y lectura informativa de eventos/estado de Microsoft Defender según permisos y disponibilidad.

**No es un antivirus, IDS/IPS ni protección contra phishing o intrusiones.** No bloquea ataques, no inspecciona paquetes y no garantiza detectar malware. No sustituye a Microsoft Defender, las actualizaciones ni a las copias de seguridad. La lectura de eventos puede ser incompleta según la configuración de Windows.

## Evidencia

El autor aportó [vídeo de ejecución física de la CYD](../../docs/media/GhostGuard-CYD-v1.4.mp4) (1 de noviembre de 2026, 79 s). Muestra datos reales del PC en HOME (Defender activado, CPU y RAM), RED (subida/descarga y gráfico en vivo), EVENTOS y SYSTEM CORE, con navegación táctil entre pantallas. Las notas del paquete marcan la prueba física como pendiente porque se escribieron antes de grabar este vídeo. Esta evidencia **no equivale a una auditoría de seguridad ni a una prueba automatizada exhaustiva**. No se incluyen capturas sintéticas.

## Licencia

Consulta `LICENSE` en el ZIP; las dependencias conservan sus licencias.
