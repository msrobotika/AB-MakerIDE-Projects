# Checklist de calidad de requisitos

2026-11-09 · Revisión previa a implementación; las marcas no significan pruebas físicas.

- [x] CHK001 Alcance y prohibiciones expresas recogidos; base Git identificada sin cambios previos en destino.
- [x] CHK002 Cada historia tiene criterios observables A1–A9 y prueba independiente.
- [x] CHK003 Escaneo de redes y BLE explícitamente pasivos; sin reconexión ni persistencia de resultados.
- [x] CHK004 OUI, redes, BLE y vídeo separados con confianza y límites; sin prefijos inventados.
- [x] CHK005 Salto de canales, tiempo escuchado y continuidad de vídeo definidos sin prometer tráfico completo.
- [x] CHK006 Saturación, señal perdida, ignorado, errores de radio y calibración contemplados.
- [x] CHK007 Pantallas y README españoles; privacidad de nombres/direcciones comprobable.
- [x] CHK008 Plan y tareas cubren todos los criterios; compilación bloqueada y prueba física no se confunden con éxito.

Coherencia previa: sin contradicciones abiertas. La descarga HTTP de IEEE falla en PowerShell; HTTPS desde Node funciona. CLI Arduino informa acceso denegado a su configuración. Ambas observaciones condicionan herramientas, no alteran la pasividad del firmware.
