# Constitución · CazaCámaras MS

Versión 1.0.0 · 2026-11-09.

1. Solo crear o cambiar archivos dentro de `examples/cazacamaras-ms-cyd/`. Sin commits, publicación ni carga en placa.
2. Escucha pasiva: sin conexión, sondas activas, desautenticación ni transmisión deliberada. WiFi sin credenciales ni reconexión automática; escaneo de redes pasivo.
3. Datos efímeros: direcciones solo en RAM para agrupar señales. Nunca mostrar MAC, IP, SSID ni nombres BLE. NVS solo para sonido, brillo y calibración.
4. Reutilizar hardware y ayudantes de Vigía. OUI reales contrastados con IEEE; no atribuir prefijos compartidos a una marca sin respaldo.
5. Evidencia honesta: distinguir revisión, pruebas de lógica, compilación y prueba física. Una señal indica un posible dispositivo, no demuestra una cámara oculta.

## Proceso y preservación

Spec Kit aplicado manualmente: `specify` no está en PATH. Se han leído las plantillas oficiales de constitución, especificación, plan, tareas y checklist de `C:/Users/abeli/Desktop/GitHub-Spec-Kit/templates/`. No se inicializa en la raíz para respetar el alcance del usuario.

Base existente: commit `8bce168bdc3a741f4ef9321db0132ec19ffeb8c4`. La carpeta de destino no existía. Los originales de Vigía son solo lectura y siguen respaldados por Git; los archivos ajenos sin seguimiento se preservan. No hace falta sobrescribir ni respaldar un archivo de destino anterior.

Esta constitución concreta la del repositorio; la prohibición actual de publicar y grabar prevalece sobre autorizaciones históricas.
