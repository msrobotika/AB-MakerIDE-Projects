<div align="center">

# AB MakerIDE · Proyectos
### Mira el vídeo. Descarga el proyecto. Hazlo tuyo.

Proyectos open source de **MS Robotika**, creados con **AB MakerIDE SUPER** para makers con Arduino y ESP32.

**[Conseguir el IDE](https://github.com/msrobotika/AB-MakerIDE-SUPER)** · [Instalación y descargas](https://github.com/msrobotika/AB-MakerIDE-SUPER/blob/main/docs/DESCARGAS_Y_VERSIONES.md) · [Laboratorio de pantallas](https://github.com/msrobotika/AB-Displays-Lab)

</div>

## ¿Vienes de un vídeo? Empieza aquí

### 01 · Radar WiFi CYD

Un radar naval verde en una pantalla CYD: contactos por intensidad, lista táctil de redes y detalle por punto de acceso. Se ejecuta en la placa, con WiFi de 2,4 GHz.

<div align="center">
<img src="https://raw.githubusercontent.com/msrobotika/AB-Displays-Lab/main/docs/images/radar-wifi-cyd/0_radar_home.png" width="290" alt="Radar WiFi en LCD físico; fotograma real del vídeo">
<img src="https://raw.githubusercontent.com/msrobotika/AB-Displays-Lab/main/docs/images/radar-wifi-cyd/2_lista_redes.png" width="290" alt="Lista real con nombres de red ocultos">
</div>

### **[⬇ Descargar Radar WiFi CYD en ZIP](https://raw.githubusercontent.com/msrobotika/AB-Displays-Lab/main/downloads/Radar-WiFi-CYD.zip)**

[Código y guía](https://github.com/msrobotika/AB-Displays-Lab/tree/main/examples/radar-wifi-cyd) · [Galería real](https://github.com/msrobotika/AB-Displays-Lab/blob/main/docs/GALERIA.md) · [Pines y placa](https://github.com/msrobotika/AB-Displays-Lab/tree/main/examples/radar-wifi-cyd#hardware-y-pines) · [SHA-256](https://github.com/msrobotika/AB-Displays-Lab/blob/main/downloads/Radar-WiFi-CYD.zip.sha256)

| Necesitas | Detalle |
|---|---|
| Placa declarada | CYD ESP32-2432S028R, ILI9341 240 × 320 y táctil XPT2046 |
| Proyecto | Arduino C++ con dos cabeceras; fuentes y documentación bajo MIT |
| Preparación | Extraer ZIP, abrir `RadarWiFiCYD.ino` y preparar núcleo/bibliotecas |
| Evidencia | Compilación completa y vídeo aportado del LCD físico; límites documentados |

El radio representa señal relativa; el ángulo es decorativo. No mide distancia ni dirección. Las capturas proceden del vídeo original y ocultan los identificadores de redes. [Qué se ha probado](https://github.com/msrobotika/AB-Displays-Lab/blob/main/docs/EVIDENCIA.md).

## De la descarga a tu placa

1. **Descarga** el ZIP del proyecto y extráelo completo.
2. **Abre** `RadarWiFiCYD/RadarWiFiCYD.ino` en [AB MakerIDE SUPER](https://github.com/msrobotika/AB-MakerIDE-SUPER). Mantén las cabeceras junto al sketch.
3. **Prepara y compila** con el núcleo y bibliotecas de la guía incluida. Confirma el modelo y revisión de tu placa antes de cargar.
4. **Explora y comparte** tus cambios. Oculta nombres de redes y BSSID al grabar tus propias capturas.

La descarga contiene código fuente; Arduino CLI, núcleos y bibliotecas se instalan por separado. [Guía de uso](docs/EMPEZAR.md).

## Tres sitios, un recorrido

| Buscas | Abre |
|---|---|
| Crear, editar y compilar tus proyectos | [AB MakerIDE SUPER](https://github.com/msrobotika/AB-MakerIDE-SUPER) |
| Elegir un proyecto y descargarlo | **Este catálogo** |
| Fuentes, pantallas, pines y evidencias CYD | [AB Displays Lab](https://github.com/msrobotika/AB-Displays-Lab) |

El firmware vive en su laboratorio de origen; este catálogo lo reúne mediante enlaces para mantener una sola fuente. Por ahora hay **un proyecto disponible**. [Índice de proyectos](projects.json).

## Construyámoslo entre makers

¿Has probado otra revisión, encontrado un fallo o mejorado un proyecto? [Abre una incidencia](https://github.com/msrobotika/AB-MakerIDE-Projects/issues) o consulta [cómo contribuir](CONTRIBUTING.md). Indica qué has probado y con qué placa. No compartas claves ni datos privados.

[MIT](LICENSE) para contenido propio · [Dependencias y atribuciones](THIRD_PARTY_NOTICES.md). Sin promesas de compatibilidad universal ni pruebas físicas que no se hayan realizado.
