# Implementation Plan: Cerca de mí

Fecha: 2026-11-09. Spec: [spec.md](spec.md). Proceso manual con plantillas oficiales locales; CLI ausente, sin extensiones.

## Summary / Technical Context

Único .ino Arduino C++ ESP32 3.x / ArduinoJson 7. Escaneo síncrono cada 120 s, selección ordenada acotada a 15 y TLS beaconDB. Respuestas pequeñas de posición filtradas y acotadas; provincias por streaming compartiendo parser municipal y BufferedInput. Array provincial dinámico único (~82 kB máximo) liberado al desactivar automático. Sin nuevo almacenamiento de posición ni bibliotecas.

Modelo RAM: Position (lat/lon, accuracy, fallos), NearStation (float lat/lon, uint32_t mills, marca[21], dirección[35]; 68 bytes con alineación de 4), caché (provincia/producto/fecha oficial/edad/error). Tabla constante de 52 capitales; Haversine para provincia, radio y selección. Config binaria previa intacta y preferencia `near` separada. Evitar dos copias provinciales: invalidar caché al iniciar descarga. Liberar la caché antes de las listas completas de configuración para dejar RAM al flujo ya probado; recuperarla después.

Contratos: POST https://api.beacondb.net/v1/geolocate, considerIp=false, wifiAccessPoints MAC/señal, User-Agent solicitado. Ministerio GET FiltroProvinciaProducto, Accept JSON, HTTP/1.0 sin chunked. Error y respuesta incompleta nunca implican datos actuales.

## Constitution Check

Pasa: alcance, respaldo ZIP y hashes, un .ino, privacidad explícita, sin placa/publicación ni agentes. Revisar CA obtenidas de letsencrypt.org y huellas CP/CPS. Mantener evidencia física anterior.

## Secuencia y validación

1. Preservar base y revisar requisitos/checklist antes del código.
2. Obtener CA oficiales, cotejar huellas y firmas; comprobar TLS de escritorio sin escanear redes reales.
3. Implementar escaneo, parser provincial, caché y selección; pantalla y ajustes.
4. Pruebas de escritorio sobre JS real y funciones C++ extraídas del .ino (compilador local si disponible), fixtures y errores. Revisar diferencias con base.
5. Comando Arduino exacto; si excede flash probar huge_app; si sandbox bloquea, registrar bloqueo sin simular compilación. Limpiar .build dentro del ejemplo.
6. README y matriz de evidencia; RF/placa pendiente.
