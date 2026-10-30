# Verificación · acceso QR

2026-10-30. Cambio especificado y comprometido antes de implementar, sobre base db2d23d. Primera compilación detectó colisión de cabecera con el SDK: resuelta con encoder MIT aislado y hashes de procedencia, sin cambiar el SDK ni instalar bibliotecas.

| Criterio | Resultado comprobado |
|---|---|
| AC1 | QR WiFi de sesión generado en el dispositivo; pantalla de arranque de 180×180 píxeles, margen de 4 módulos y módulos de 4 px. STATUS confirma encoder WiFi correcto. |
| AC2 | Bootstrap QR intercambia el fragmento en cabecera y lo borra del historial; fallo/expiración impiden peticiones privadas. Tres pruebas nuevas del JS real correctas, con dobles de DOM/red. Reglas de caducidad, consumo, rechazo y replay verificadas en compilación C++; cookie HttpOnly/SameSite Strict y Basic alternativo implementados. |
| AC3/4 | Controles QR WIFI / QR WEB / ESTADO implementados, calibración persistente confirmada en STATUS. Las ocho pruebas JS anteriores siguen pasando. Las transferencias físicas de la versión anterior figuran como evidencia histórica en specs/001-mini-nas, sin afirmar que se repitieron en este nuevo binario. |
| AC5 | Compilación completa salida 0: 1,071,346 bytes de programa y 50,372 de RAM estática. 47 aserciones de reglas más una de geometría. Carga COM4 correcta, arranque, AP y SD consultados por USB. Dos matrices QR con credenciales de demostración generadas por el encoder real del ESP32: decodificación OpenCV exacta a 180×180. |

El autor confirmó el acceso al WiFi y la apertura del panel con los QR en el móvil.

El autor confirmó que los botones de QR responden después de la corrección. [Diagnóstico del toque](bug-001-touch.md).

No se extrajeron ni guardaron las matrices QR reales de acceso. QRTEST por USB usa únicamente SSID/clave/token ficticios públicos. No se publican contraseñas, cookies o códigos vigentes. La prueba del encoder no acredita por sí misma enfoque, moiré o compatibilidad de cámaras concretas. Se conservan pendientes de estabilidad prolongada, Android/iOS distintos y casos físicos no ejecutados del proyecto base.

[Resultados](results.json). La cookie es de sesión y se invalida al reiniciar el NAS. El QR WiFi cambia con el arranque; el QR web se consume/caduca y se renueva al pulsar QR WEB. No hay claves en enlaces Basic ni secreto de acceso en las peticiones GET del servidor.
