# Especificación · CazaCámaras MS

Fecha: 2026-11-09 · Estado: implementada; compilación y pruebas físicas pendientes por bloqueo de entorno · Sin rama nueva.

## Historias y aceptación

### US1 (P1): revisar una habitación

- A1: escucha WiFi en canales 1–13, unos 250 ms por canal; tabla de 64 transmisores con OUI, RSSI último/mejor/suavizado, canal, fechas y tramas/bytes de una ventana de 10 s. Cola acotada y protegida; nunca procesar pantalla ni clasificar en el callback WiFi.
- A2: ALTA siempre cuenta; MEDIA con vídeo o más de 30 s de presencia; BAJA solo con vídeo. Vídeo significa subida observada por encima de 20 000 B/s, mayoría de tramas de datos y evidencia sostenida durante más de 10 s. Los OUI desconocidos también pueden aportar vídeo. No confundir tráfico de bajada del punto de acceso con subida.
- A3: cada unos 20 s, suspender escucha promiscua y hacer escaneo pasivo de redes. Los 20 patrones solicitados se comparan sin distinguir mayúsculas. `V380-TEST` produce solo «Red de cámara tipo V380». Ignorar redes restantes. BLE pasivo aporta pistas por cam/IPC/V380/spy sin enseñar nombres.
- A4: ESCANEO muestra verde sin sospechosos, ámbar con posibles y rojo con ALTA o vídeo. Recuentos WiFi/redes/Bluetooth y progreso de canales. Errores de radio visibles, sin presentar un fallo como tranquilidad.

Prueba independiente: hotspot de prueba, dispositivo de OUI conocido y muestras sintéticas de subida/bajada.

### US2 (P1): encontrar la señal

- A5: seis pantallas por deslizamiento: ESCANEO, RADAR, LISTA, BUSCAR, CONSEJOS, AJUSTES. Radar RSSI con ángulo decorativo. LISTA enseña hasta seis, numeración estable durante su estancia, marca/confianza/vídeo/barras; toque abre BUSCAR.
- A6: BUSCAR fija el canal WiFi elegido, suspende escaneo de redes, suaviza RSSI y muestra FRÍO/TEMPLADO/¡CALIENTE!, barra grande y pitidos más rápidos. Señal caducada deja de pitar. Ignorar afecta al veredicto hasta reiniciar; salir reanuda salto. BLE no tiene canal WiFi que fijar.

Prueba independiente: mover una fuente dentro de la habitación; probar navegación, señal perdida e ignorar.

### US3 (P2): ajustar y entender límites

- A7: mismo cableado, buses, CP437, calibración de tres puntos, LDR, PWM, LED activo bajo y altavoz de Vigía. Ajustes persistentes y recalibración. Ninguna detección ni marca de ignorado persiste.
- A8: consejos completos en español, README con límites honestos, prueba V380, pines, dependencias, compilación y evidencia pendiente de placa; LICENSE copiado exactamente.
- A9: ejecutar el comando de compilación pedido; corregir errores propios si puede ejecutarse. Si el sandbox bloquea, registrar bloqueo y no inventar flash/RAM; eliminar `.build` al terminar.

## Decisiones y casos límite

- OUI indica fabricante, no modelo. Marcas que usan OEM pueden aparecer con otro nombre. No inventar prefijos para cubrir una marca.
- Redes y BLE son pistas independientes: se cuentan como posibles aunque no superen el filtro OUI WiFi; no prueban vídeo ni elevan a rojo por sí solas.
- Una única radio solo escucha un canal cada vez. Ventana de 10 s conserva bytes realmente recibidos; el umbral de vídeo se calcula por tiempo efectivamente escuchado en ese canal. Se necesitan muestras repetidas que abarquen más de 10 s, sin huecos de más de 6 s entre muestras positivas. El resultado es una estimación, sin inspección de contenido cifrado.
- Retención solo en RAM, presencia de 60 s y caducidad de 3 min; BLE general se cuenta durante 30 s. Tabla llena: sustituir solo entradas caducadas, conservar seleccionada/ignoradas y avisar de saturación. Pausas, pérdida de cola o cambio de canal del transmisor invalidan continuidad de vídeo.
- BUSCAR no da metros ni dirección. Si la señal desaparece, mostrar «Sin señal reciente». Los canales se fijan conforme al dominio ES (1–13).

## Resultado medible

Fuentes y licencia solo en el destino autorizado; filtros y límites verificables por pruebas de lógica; compilación o bloqueo real documentados; pruebas físicas expresamente pendientes.
