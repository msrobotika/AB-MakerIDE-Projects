# Plan: importación y documentación
**Fecha**: 2026-10-30 | **Especificación**: [spec.md](spec.md)

## Contexto y diseño
Firmware Arduino C++ existente, ESP32/CYD con ILI9341 y XPT2046. Herramientas de PC para compilación y extracción PNG; no conectan ni graban la placa. Archivos en examples/radar-wifi-cyd/, imágenes en docs/images/radar-wifi-cyd/, documentación en docs/ y proceso en specs/001-import-radar/.

## Secuencia
1. Respaldar seis originales y registrar base Git vacía.
2. Revisar especificación, constitución, checklist y coherencia antes de copiar contenido.
3. Copiar originales, añadir utilidad de compilación portable basada en la original, documentar hardware y procedencia.
4. Inspeccionar vídeo, elegir ocho cuadros, anonimizar identificadores y revisar cada PNG. Mantener originales privados fuera de Git.
5. Compilar con herramientas existentes, ejecutar autopruebas solo si hay entorno de host adecuado, validar todos los criterios e inventario antes del push.
6. Push a main sin forzar, verificar SHA/árbol/privacidad/releases mediante GitHub.

## Puerta constitucional
Revisada antes del diseño y antes de implementación: sin conflictos. La exportación histórica es evidencia declarada, no test repetido. No se ejecutan scripts originales de monitorización ni instrucciones históricas de carga. No hay costes, hardware ni publicación pública.
