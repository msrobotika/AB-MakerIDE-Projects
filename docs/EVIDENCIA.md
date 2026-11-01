# Evidencias y límites

## Observación física aportada
El vídeo original muestra una CYD con radar y barrido, lista de redes, pantalla de detalle y el uso de un lápiz táctil. Las capturas muestran el LCD físico; no son renderizados ni un simulador. No se dispone de una asociación criptográfica entre ese vídeo y el binario exacto del sketch; se documentan como evidencia visual del proyecto aportado.

Las imágenes aisladas no demuestran por sí solas respuesta táctil, continuidad de animación, exactitud de RSSI, seguridad RF ni persistencia tras reiniciar. El vídeo aporta contexto de navegación y variación de pantalla; no se ha repetido una prueba física completa.

## Declaraciones históricas
La exportación original declara `flash_reported_verified`, pero también `upload_requested: false`. Se conservan ambos campos sin reinterpretarlos como una nueva carga. El puerto y FQBN son declarativos. El README original menciona registros de compilación/carga y copias del dispositivo que no se adjuntan aquí; no se presentan como verificaciones repetidas.

## Verificación actual
Se comprueban integridad de las fuentes, PNG, referencias, inventario y compilación completa si el entorno disponible la permite. El informe [VALIDACION.md](VALIDACION.md) registra los resultados realmente obtenidos.

No se abre puerto serie, no se lee ni escribe flash, no se ejecutan autopruebas en placa ni se conecta a redes. La presencia de `radar_selftest.h` y su compilación no equivalen a un PASS de ejecución. No hay releases.

## Comprobaciones físicas pendientes
Calibración desde cero y tras reiniciar, precisión táctil en toda la superficie, todas las páginas y detalles, redes ocultas/duplicadas, 48 AP, ausencia de redes, fallos de radio y pruebas prolongadas. Ninguna se marca superada por el vídeo ni por compilar.
