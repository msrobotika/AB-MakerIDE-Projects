# Especificación · Gasolina MS

Fecha: 9 nov 2026. Estado: preparada para implementar.

## Historias y criterios de aceptación

### US1 · Configurar (P1)
- A1: sin credenciales, calibrar y abrir AP Gasolina-MS con QR y portal solo Wi-Fi. La red doméstica nunca aparece en LCD ni serie; únicamente en el formulario necesario para elegirla.
- A2: conectado, servir web LAN con provincias, municipios dependientes, cinco combustibles y depósito de 50 L por defecto; validar y persistir. Mostrar QR en AJUSTES y, sin pueblo, «Elige tu pueblo: escanea el QR».
- A3: cambiar municipio o combustible limpia datos e histórico para impedir mezclas. Cambiar litros conserva histórico.

### US2 · Comparar precios (P1)
- A4: enviar Accept application/json, validar HTTPS con DigiCert Global Root G2 y analizar flujo con ArduinoJson 7 Filter. Solo venta P, precio decimal válido y no vacío. Mantener top 5, recuento, mínimo, máximo y media aritmética de todas las estaciones válidas.
- A5: AHORA muestra marca, dirección, precio, combustible, diferencia frente a media, ahorro para litros elegidos y hora oficial. TOP 5 ordenado y coloreado de verde a ámbar; menos de cinco no crea filas ficticias.
- A6: actualizar cada 30 min o a petición. Error visible con reintento, sin sustituir datos válidos por una respuesta parcial. LED rojo intermitente en error. Una línea serie por intento con municipio, producto, recuento, mínimo, media y hora; sin IP salvo URL de ajustes una vez al arrancar.

### US3 · Tendencia y uso (P2)
- A7: guardar hasta 14 días naturales, mínimo y media por día, después de las 08:00; reemplazar por la última descarga válida del día. Sobrevivir al reinicio, no rellenar días ausentes. Comparar exclusivamente hoy con ayer; con menos de dos días pedir volver mañana. LED verde si baja/iguala, ámbar si sube; sin referencia verde significa solo descarga correcta.
- A8: cuatro pantallas por deslizamiento, QR LAN, brillo y recalibración táctil; mantener pines, buses, calibración afín y aspecto de Kilovatio.
- A9: pie visible «Precios oficiales del Ministerio (datos abiertos). Pueden tardar en actualizarse: confirma en el surtidor.» Sin garantía de precio, apertura actual ni seguimiento de ubicación.
- A10: README español, MIT copiada, compilación solicitada o bloqueo documentado, `.build` borrado al terminar; placa explícitamente pendiente.

## Decisiones y casos límite

No hay ambigüedades que requieran detener el trabajo. «Última del día» elige reemplazo de muestra tras las 08:00, según fecha local Europe/Madrid. Solo se registra si la fecha oficial corresponde a hoy: una respuesta antigua no crea historia de hoy. Antes de las 08:00 no se registra. Ayer significa día natural anterior, incluidos cambio de mes/año y horario de verano. No afirmar «hoy» con datos oficiales antiguos. Sin estaciones públicas con precio: estado informativo, sin ahorro inventado. Certificado/hora/JSON/red fallidos: error y reintento. Formularios LAN locales, sin servicios web externos.

## Entidades

Configuración NVS; estación (marca, dirección, precio; horario/ID solo si necesarios); resumen (cinco estaciones, cuenta, mínimo/media/máximo, fecha oficial); muestra diaria (fecha, mínimo, media), asociada a municipio y producto.

## Resultados medibles

Un único .ino; máximo cinco estaciones retenidas; catorce muestras como máximo y una por fecha; cálculos verificables con datos sintéticos; compilación real separada de validación física.
