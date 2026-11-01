# Verificación y convergencia de la incorporación

> Informe histórico de la importación inicial privada. La publicación pública y el ZIP posterior se documentan en [la verificación de publicación](../002-public-catalog/verification.md).

Fecha: 2026-10-30. Resultado: **Convergente para el alcance de importación y documentación**. Las pruebas físicas no repetidas siguen pendientes y no forman parte de esta declaración de convergencia.

## Entrega remota comprobada

Repositorio preexistente: [msrobotika/AB-Displays-Lab](https://github.com/msrobotika/AB-Displays-Lab), creado el 2026-10-30. Identificado como el único repositorio de laboratorio de pantallas entre los 20 accesibles del propietario. Se encontraba vacío y privado; no se ha creado un duplicado.

[Commit de incorporación a7e389f73a4e658d96392e92bbbe6fc5e515e3ee](https://github.com/msrobotika/AB-Displays-Lab/commit/a7e389f73a4e658d96392e92bbbe6fc5e515e3ee), rama `main`: **36 rutas de archivo y 36 SHA de blob contrastados entre la copia local y el árbol remoto** mediante la conexión de GitHub. Incluye los seis originales, ocho PNG y documentación. La verificación se refiere al contenido de ese commit; este informe se añade después.

La consulta remota posterior al push confirma `private: true`, `visibility: private` y lista de releases vacía. Sin tags, releases ni cambios a otros repositorios. En la enumeración de la cuenta, únicamente AB-MakerIDE-SUPER es público.

## Criterios y evidencia

| Criterio | Evidencia | Evaluación |
|---|---|---|
| AC1 | Metadatos GitHub antes/después, repositorio existente, visibilidad privada y cero releases. | Cumplido. |
| AC2 | Seis originales byte a byte y SHA-256; docs/source-manifest.json; SHA de blob remoto coincidente. | Cumplido. |
| AC3 | README y guía con pines, dependencias, controles y enlaces al IDE. Compilación completa con utilidad incluida. | Cumplido. |
| AC4 | Ocho PNG, nuevo muestreo de vídeo; manifiesto por cuadro y prueba de píxeles contra vídeo. | Cumplido con extracción nueva expresamente documentada. |
| AC5 | Revisión visual de ocho imágenes y revisión de inventario; SSID/BSSID tapados, monitor fuera del recorte; sin vídeo, binarios, secretos conocidos ni copias de dispositivo. | Cumplido en el alcance revisado. |
| AC6 | docs/EVIDENCIA.md y docs/VALIDACION.md separan vídeo, metadatos históricos, compilación y autopruebas no ejecutadas. | Cumplido; pruebas físicas pendientes conservadas. |
| AC7 | Enlaces relativos y manifiestos comprobados, compilación exitosa, 36 rutas y blobs remotos coincidentes con el commit de incorporación. | Cumplido. |

## Proceso y límites

Spec Kit aplicado manualmente con las cinco plantillas oficiales locales 1.2.3, porque la CLI no está instalada. Base Git vacía registrada en `0abd383`; constitución, especificación, plan, tareas y checklist revisados antes de importar en `92397c6`. Los originales se respaldan localmente antes de copiarlos. Sin agentes ni extensiones adicionales.

La compilación final devuelve salida 0: programa 938.224 bytes, RAM global 50.192 bytes. No se ejecutan autopruebas de host ni de placa, no se graba hardware y no se presenta el vídeo como prueba del binario exacto. El pack anterior de PNG no estaba disponible: se extraen ocho cuadros nuevos del mismo vídeo y se publican sus derivados anonimizados.
