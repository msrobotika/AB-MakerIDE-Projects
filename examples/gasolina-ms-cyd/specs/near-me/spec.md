# Feature Specification: Cerca de mí

Creada: 2026-11-09. Sin rama ni commit por instrucción del usuario. Especificación revisada antes de implementar.

## User Scenarios & Testing

### US1 (P1): conducir con el hotspot del móvil
Con conexión activa, localizarse cada 120 s o al actualizar. Mostrar la estación pública más barata en 5 km y la más cercana; ampliar a 10/20 km si hace falta. Prueba: escaneo y trayecto en placa, pendiente.

### US2 (P1): seguir usando mi pueblo
Elegir ubicación automática o solo pueblo en la web manteniendo municipio, combustible, litros e histórico. Tras tres intentos fallidos volver a AHORA y avisar. Prueba: ajustes reales en navegador de escritorio y posteriormente placa.

### US3 (P2): datos verificables y privados
HTTPS validado con CA reales. Nunca imprimir redes ni coordenadas. Distancias en línea recta y precisión explícita. Precios orientativos.

## Requirements / criterios de aceptación

- A1: `WiFi.scanNetworks(false, true)` sin desconectar; hasta 15 AP distintos más fuertes tras excluir `_nomap`, MAC local/multicast/nula y BSSID conectado. POST beaconDB con `considerIp:false`, solo MAC/señal, User-Agent exacto; respuesta validada y 404 sin estimación.
- A2: raíces X2 y YE oficiales concatenadas en `setCACert`, huellas verificadas; hora NTP obligatoria. Posición solo RAM, serie de posición limitada a éxito/precisión o `sin estimacion`.
- A3: tres fallos consecutivos invalidan posición y muestran «Sin ubicación: pocas redes Wi-Fi aquí», vuelven a pueblo; recuperación automática en siguientes intentos. Un fallo intermedio marca posición anterior.
- A4: tabla de 52 capitales con ID INE 01..52; capital más cercana determina provincia. Cambio de provincia/producto invalida caché; precios cada 60 min o a demanda. Reintento de error a 2 min.
- A5: respuesta provincial leída por `BufferedInput`/ArduinoJson Filter, estación a estación; máximo 1200, lat/lon float, milésimas, marca 20 bytes útiles y dirección 34. Solo venta P, precio y coordenadas válidos. Truncamiento/capacidad excedida rechazan toda descarga; nunca presentar un subconjunto como completo.
- A6: CERCA=0, AHORA=1, TOP 5=2, TENDENCIA=3, AJUSTES=4; puntos, gestos, `MSR_TAB 0..4`, capturas por `gfx`. Primer éxito abre CERCA; no roba pantalla en cada actualización. Ahorro fijo 50 L frente a la cercana si son distintas.
- A7: preservar `readList/getString`, `listCache/nameFromCache`, `BufferedInput`, token fresco por guardado, `gfx`, Strip y comandos. Mantener formato binario Config antiguo; nuevo modo en clave NVS separada.
- A8: README con precisión orientativa 50–200 m, ausencia de redes, privacidad, consumo, crédito, límites, evidencia pendiente. Compilar con comando solicitado; huge_app solo si no cabe. Borrar .build. Solo cambios dentro del ejemplo, sin commit/push/upload.

## Aclaraciones y casos límite

No hay ambigüedades materiales pendientes. Automático por defecto; pueblo guardado permanece como respaldo. «Actualizar ahora» refresca posición, provincia y municipio. Distancia geodésica, sin rutas. La provincia por capital es una aproximación y puede equivocarse en límites; no se añaden provincias vecinas. Sin posición al arrancar se muestra pueblo; CERCA sigue accesible con estado y reintento. Tras tres fallos se borra la posición previa. La caché provincial es única para acotar RAM: un fallo de lectura la invalida de forma visible. Con más de 1200 estaciones se informa del límite, no se da un mínimo parcial. No se promete que un proveedor externo no mantenga logs HTTP.

## Success Criteria

A1–A8 contrastados en verification.md con evidencia separada de revisión, escritorio, compilación y placa. Compilar no acredita RF, TLS ESP32, memoria libre ni legibilidad física.
