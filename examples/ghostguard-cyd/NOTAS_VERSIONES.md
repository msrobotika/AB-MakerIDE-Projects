# AB CYD GhostGuard LIVE v1.0 — MS ROBOTIKA

Firmware de visualización para **ESP32-2432S028R** y agente de telemetría de **Windows 11**. Comunicación **USB serie a 115200**. La CYD muestra **datos reales del PC** cuando el agente está conectado. No es un antivirus ni sustituye Microsoft Defender.

## Pantallas

- **HOME**: PC conectado, estado reportado de Defender, CPU y RAM del PC, tarjetas de colores y dial.
- **RED**: subida/descarga total del PC en KiB/s, gráfico temporal, conexiones TCP establecidas.
- **EVENT**: recuento de determinados eventos de Defender de las últimas 24 h y último identificador/hora.
- **ESP32**: memoria libre y CPU del ESP32 (reales), CPU y disco del PC, tiempo encendido.

**Seguridad visual:** cuando no llegan datos en 6,5 segundos, aparece OFFLINE y no se muestran métricas antiguas como actuales. El estado Defender DESCONOCIDO no equivale a protegido. Los eventos son una selección de IDs de Defender, no un análisis exhaustivo de amenazas.

## Instalación paso a paso

1. En **AB MakerIDE SUPER**, abre `GhostGuard_CYD/GhostGuard_CYD.ino`.
2. Selecciona la placa ESP32 y el puerto real de tu CYD. Librerías: `Adafruit GFX`, `Adafruit ILI9341`, `Adafruit BusIO`, `XPT2046_Touchscreen`. Core ESP32 3.3.12 probado por el usuario con la versión anterior.
3. Compila y graba. La pantalla debe mostrar **OFFLINE** hasta iniciar el agente.
4. **Cierra el monitor serie del IDE**: Windows solo permite normalmente una aplicación por puerto COM.
5. En Windows instala **Python 3**, ejecuta `windows_agent/instalar.bat` y luego `windows_agent/iniciar_COM4.bat` (si la CYD está en otro puerto, edita COM4).
6. Tras el reinicio automático al abrir COM, espera ~3 segundos. La CYD debe cambiar a **PC LIVE** y mostrar CPU, RAM y Defender si Windows permite consultarlo.
7. Pulsa las pestañas táctiles HOME, RED, EVENT, ESP32. Para detener el agente, Ctrl+C. Al desconectarlo, el estado pasa a OFFLINE.

## Comprobaciones

- `python tests/test_protocol.py` (requiere `psutil` y `pyserial` instalados).
- `python windows_agent/ghostguard_agent.py --dry-run` en Windows: genera telemetría sin conectar la CYD.
- `arduino-cli compile --fqbn esp32:esp32:esp32 GhostGuard_CYD` (requiere core y bibliotecas Arduino).

## Arquitectura y límites

- La CYD **no se conecta a Internet** ni crea puntos de acceso.
- El agente consulta **psutil** y dos comandos de PowerShell de solo lectura: `Get-MpComputerStatus` y `Get-WinEvent`.
- No requiere administrador para su uso básico, pero algunos eventos o conexiones pueden no ser accesibles; se indicará valor desconocido.
- No envía nombres de procesos, IP de conexiones ni datos privados al exterior. Solo transmite por USB local el nombre del PC, métricas agregadas y resumen del evento.
- No se ejecutan comandos recibidos desde la CYD. No se modifica firewall, Defender ni configuraciones de Windows.
- **No conectar simultáneamente Monitor Serie y agente**. Abrir el puerto puede reiniciar el ESP32.
- El indicador `DEFENDER ACTIVADO` significa que el sistema reporta antivirus y protección en tiempo real habilitados; **no garantiza que el equipo esté libre de malware**.
- La pantalla de 240x320 tiene limitaciones de espacio. El diseño se implementa con primitivas GFX y no es idéntico píxel a píxel al concepto visual generado anteriormente.
- El táctil utiliza el mapeo de la versión GhostGuard v0.2 probada en el mismo hardware; si hay desfase, será necesaria calibración.

## Estado de verificación

**Código generado y pruebas Python ejecutables.** No se afirma compilación Arduino ni grabación física en esta sesión; la versión v0.2 sí fue fotografiada funcionando en la misma placa. Las imágenes conceptuales previas no son capturas de este firmware.

MIT License. Copyright (c) 2026 MS Robotika.


## v1.1: pantalla sin parpadeo
Se usa un framebuffer RGB565 de 240x320 (~150 KiB de heap) para dibujar la interfaz completa fuera de pantalla y transferirla a la TFT sin mostrar un borrado intermedio. Si no se puede reservar memoria, se utiliza el renderizador anterior (mensaje `GHOSTGUARD_FRAMEBUFFER=FALLBACK` en monitor serie). HOME y EVENTOS solo se redibujan cuando llegan datos; RED y ESP32 actualizan gráficas/uptime. La frecuencia de actualizacion depende de la telemetria del agente.

**Prueba:** cargar el nuevo `.ino`, cerrar monitor serie antes de ejecutar el agente COM4, comprobar que aparece `GHOSTGUARD_FRAMEBUFFER=OK` y observar estabilidad visual. No se ha probado esta revision en la placa fisica.


## v1.2: calibración táctil
En el primer arranque aparece una calibración de tres cruces (arriba izquierda, arriba derecha, abajo izquierda). Toca cada cruz con precisión y espera a que desaparezca. Las coordenadas se guardan en NVS, así que no hay que repetirlo en cada encendido. Las pestañas se activan al **soltar** el dedo, evitando pulsaciones duplicadas.

Para repetir la calibración, borra la NVS desde Arduino IDE (Erase All Flash) y vuelve a grabar. Esto también elimina otras preferencias almacenadas. No hace falta repetirlo si el táctil funciona.

**Estado:** parche revisado estáticamente; no compilado ni probado en CYD física en esta sesión.


## v1.3 Incremental refresh
Telemetry redraws only live data fields and the graph interior; full-page draw occurs only on boot and navigation. Touch calibration remains stored in NVS. The serial agent and scripts are unchanged from v1.2. Physical build and hardware QA are pending.


## v1.4 CYBER GREEN — 2026-11-01
- Identidad visual negra y verde neón; páginas HOME, NETWORK, SECURITY EVENTS y SYSTEM CORE con acentos distintos.
- Renderizado incremental conservado: cabeceras y tarjetas solo en navegación; campos de datos se actualizan por zonas.
- Desactivado framebuffer completo para evitar transferencias de pantalla entera y consumo de 150 KiB.
- Se conserva calibración táctil NVS y agente Windows sin cambios.
- Compilación Arduino y prueba física **pendientes**. La compatibilidad visual exacta debe validarse en CYD.
- El agente sigue siendo de solo lectura; no sustituye antivirus ni detecta amenazas por sí mismo.
