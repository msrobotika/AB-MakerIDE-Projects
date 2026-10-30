# Fallo comunicado después de incorporar QR

El autor informa «los botones que no van». STATUS indica calibración guardada, pero eso no demuestra que cada toque se atienda. El código actual lee coordenadas una sola vez en el flanco de IRQ; si el primer muestreo es transitorio/no válido, marca la pulsación como anterior y no vuelve a muestrear hasta soltar. Es una debilidad verificable del flujo, sin atribuir una causa física aún no medida.

Corrección prevista: esperar un breve asentamiento y repetir muestreo mientras el dedo siga apoyado. Consumir la pulsación solo cuando exista coordenada válida y ejecutar como máximo una acción por apoyo. Mantener protección durante transferencias. Registrar por USB contadores y última posición de toque, sin credenciales. Añadir comprobaciones del estado de pulsación real utilizado por el firmware, compilar/cargar y pedir confirmación del síntoma sobre la CYD. Conservar calibración existente salvo que el diagnóstico exija recalibrar.

Aceptación: un primer muestreo fallido no impide un segundo válido en la misma pulsación; mantener pulsado no repite acciones; soltar permite una nueva acción. Confirmación física requerida antes de declarar reparado el síntoma.

## Verificación de la reparación

Corrección implementada en 08fc17b y cargada en COM4. Se espera un asentamiento de 20 ms y se reintenta el muestreo hasta obtener coordenadas válidas; una pulsación se consume una sola vez. Las comprobaciones C++ cubren fallo inicial seguido de muestra válida, una acción por apoyo y nueva acción después de soltar.

Con el binario corregido, STATUS registró 183 intentos de muestreo, 3 muestras válidas y 3 acciones de botones. El autor respondió «Sí, cambia» a la comprobación de que QR WEB abre «2. ENTRA AL NAS». Se da por confirmado ese síntoma reparado en esta placa. Esta confirmación no acredita aún los dos escaneos con un móvil ni compatibilidad con otras pantallas.
