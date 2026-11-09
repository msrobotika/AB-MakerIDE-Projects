# Tabla OUI · CazaCámaras MS

Extracto de [IEEE MA-L](https://standards-oui.ieee.org/oui/oui.csv), consultado el 2026-11-09. 128 prefijos de 24 bits. La selección no es exhaustiva. Titulares exactos en [spec/oui-ieee.json](spec/oui-ieee.json).

| Marca o chip | Confianza | OUI |
|---|---|---|
| Hikvision | ALTA | 0C:75:D2, 54:8C:81, 24:48:45, EC:C8:9C, 8C:E7:48, 24:28:FD, AC:B9:2F, D4:E8:53, C0:56:E3, BC:AD:28 |
| Dahua | ALTA | 74:C9:29, 6C:1C:71, 08:ED:ED, E0:2E:FE, 40:7A:A4, 5C:F5:1A |
| EZVIZ | ALTA | 64:F2:FB, 94:EC:13, EC:97:E0, F4:70:18, FC:24:22 |
| Imou | ALTA | A8:31:62, 30:24:50, 90:6A:94, 00:D9:8B, AC:3D:FA, 1C:4D:89 |
| Uniview | ALTA | 88:26:3F, 14:BA:88, 6C:F1:7E, 48:EA:63, C4:79:05 |
| Reolink | ALTA | EC:71:DB |
| Amcrest | ALTA | 00:65:1E, 9C:8E:CD, A0:60:32 |
| Arlo | ALTA | FC:9C:98, 48:62:64, A4:11:62 |
| Wyze | ALTA | D0:3F:27, F0:C8:8B, 80:48:2C, 7C:78:B2, 2C:AA:8E |
| Hanwha/Wisenet | ALTA | 44:B4:23, E4:30:22 |
| Axis | ALTA | B8:A4:4F, 00:40:8C, E8:27:25, AC:CC:8E |
| Lorex | ALTA | 00:1F:54 |
| Swann | ALTA | BC:51:FE |
| YI/Kami | ALTA | 58:70:C6 |
| Xiaomi/Imilab | MEDIA | 60:7E:A4, B4:10:1C, 78:DF:72, 94:F8:27, B8:88:80, CC:EB:5E, B8:EA:98, 8C:D0:B2, F4:1A:9C, DC:6A:E7 |
| TP-Link/Tapo | MEDIA | 34:F7:16, 54:A7:03, B0:BE:76, 34:E8:94, 94:D9:B3, B0:95:8E |
| Amazon/Ring | MEDIA | 54:E0:19, AC:9F:C3, CC:3B:FB, 5C:47:5E |
| Amazon/Blink | MEDIA | 70:AD:43, 74:13:48, F0:74:C1, C8:19:D8 |
| Amazon | MEDIA | 84:28:59, 28:73:F6, E0:CB:1D, FC:D7:49 |
| Anker/Eufy | MEDIA | 8C:85:80 |
| Google/Nest | MEDIA | 64:16:66, 18:B4:30, 60:70:6C, C8:2A:DD, 24:29:34, 08:B4:B1 |
| Tuya | MEDIA | 1C:90:FF, FC:3C:D7, E4:AE:E4, FC:67:1F, CC:8C:BF |
| Espressif | BAJA | D4:8A:FC, E4:65:B8, B4:8A:0A, 94:E6:86, 34:B4:72, 34:AB:95, 3C:61:05, 80:7D:3A, 60:01:94, 90:97:D5, 24:A1:60, 24:6F:28, 30:AE:A4, A4:CF:12, AC:67:B2, 7C:DF:A1, EC:FA:BC, DC:4F:22 |
| Beken | BAJA | C8:47:8C, 70:87:9E, 80:6D:DE |
| Realtek | BAJA | 00:E0:4C, FC:93:4E |
| Ingenic | BAJA | D0:31:10 |
| Hisilicon | BAJA | 84:8A:59 |
| MediaTek | BAJA | 00:0C:43, 00:0C:E7, 00:17:A5, 00:0A:00 |
| AltoBeam | BAJA | 44:72:AC, 38:54:F5, 84:7A:B6 |
| FN-Link | BAJA | 48:D8:90, AC:64:CF, 2C:D2:6B |

Imou se vincula a Huacheng mediante su [página oficial de contacto](https://new.imoulife.com/eu/support/contact-us). El prefijo 8C:85:80 figura como Smart Innovation LLC en IEEE; la asociación con Eufy se apoya en la HomeBase 2 del [estudio CICIoT2023, tabla de dispositivos](https://www.mdpi.com/1424-8220/23/13/5941). Es una asociación de producto, no una garantía de que todo el bloque sea Eufy.

Foscam y Anyka: no se han encontrado asignaciones bajo esos nombres en MA-L, MA-M ni MA-S en esta consulta. Se omiten para no inventar OUI. Esto no impide detectar sus redes por patrón o tráfico. Anker no se atribuye a ANKER-EAST, cuyo nombre por sí solo no demuestra relación con Eufy.

AltoBeam y FN-Link se tratan como chips genéricos, nunca como YI aunque algunos modelos los usen. No se incluyen prefijos populares sin registro IEEE (por ejemplo 00:62:6E), ni se atribuye C4:D6:55 a Foscam: IEEE lo asigna a Tercel. Los nombres ALTA/MEDIA/BAJA son reglas del proyecto, no porcentajes certificados.
