# Empezar con un proyecto del catálogo

[← Catálogo](../README.md) · [IDE](https://github.com/msrobotika/AB-MakerIDE-SUPER)

1. Usa el enlace ZIP del proyecto; el NAS también tiene sus fuentes dentro de este repositorio.
2. Extrae todos los archivos. Para el radar, la carpeta `RadarWiFiCYD` contiene `RadarWiFiCYD.ino`, `radar_logic.h` y `radar_selftest.h`.
3. Abre el principal en AB MakerIDE SUPER. Instala por separado Arduino CLI, el núcleo y las bibliotecas que pide el proyecto.
4. Compila para el perfil indicado. Una compilación correcta no comprueba físicamente pantalla, táctil ni radio.
5. Si decides grabarlo, comprueba tu placa/revisión y puerto; conserva tu trabajo previo. Esta descarga no graba nada automáticamente.

[Guía completa del radar](../examples/radar-wifi-cyd/) · [Descargar ZIP](../downloads/Radar-WiFi-CYD.zip) · [Pines CYD](../examples/radar-wifi-cyd/README.md#hardware-y-pines).

## Estación meteorológica WiFi CYD
[Guía](../examples/weather-station-cyd/) · [ZIP](../downloads/Estacion-Meteo-CYD.zip) · [Galería y pruebas](WEATHER_STATION.md).

Las capturas publicadas se han anonimizado. El firmware puede mostrar los nombres y BSSID reales de redes cercanas; revisa tus propios vídeos antes de publicarlos.


## Mini NAS CYD
[Guía](../examples/mini-nas-cyd/) · [ZIP completo](../downloads/Mini-NAS-CYD.zip) · [Galería y pruebas](MINI_NAS.md). Conserva MiniNasCYD.ino, cinco cabeceras y src/ricmoo. Prepara una microSD FAT32 con la alimentación desconectada; el firmware no formatea.
