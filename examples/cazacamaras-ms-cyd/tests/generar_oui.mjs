// Herramienta de desarrollo: descarga IEEE, valida titulares y genera la tabla.
// Ejecutar desde la raíz del repositorio con Node 22 o posterior.
import fs from 'node:fs';
import path from 'node:path';
const root = path.resolve(import.meta.dirname, '..');
const url = 'https://standards-oui.ieee.org/oui/oui.csv';
const response = await fetch(url);
if (!response.ok) throw new Error(`IEEE: ${response.status}`);
const csv = await response.text();
const registry = csv.split(/\r?\n/).flatMap(line => {
  const m = line.match(/^MA-L,([A-F0-9]{6}),("(?:[^"]|"")*"|[^,]*),/);
  return m ? [{ oui: m[1], titular: m[2].replace(/^"|"$/g, '').replace(/""/g, '"') }] : [];
});
// Nombre de pantalla, confianza, titular IEEE exacto o parcial y máximo de entradas.
const groups = [
  ['Hikvision','ALTA',/Hikvision/i,8], ['Dahua','ALTA',/Dahua/i,6],
  ['EZVIZ','ALTA',/Ezviz/i,5], ['Imou','ALTA',/Huacheng Network/i,6],
  ['Uniview','ALTA',/Uniview/i,5], ['Reolink','ALTA',/^Reolink/i,4],
  ['Amcrest','ALTA',/^Amcrest/i,4], ['Arlo','ALTA',/^Arlo Technology$/i,4],
  ['Wyze','ALTA',/^Wyze Labs/i,5], ['Hanwha/Wisenet','ALTA',/Hanwha Vision/i,4],
  ['Axis','ALTA',/^Axis Communications/i,4], ['Lorex','ALTA',/^Lorex/i,3],
  ['Swann','ALTA',/^Swann communications/i,3], ['YI/Kami','ALTA',/^Shanghai Xiaoyi/i,3],
  ['Xiaomi/Imilab','MEDIA',/^Shanghai Imilab/i,5], ['Xiaomi/Imilab','MEDIA',/^Xiaomi |^Beijing Xiaomi|^XIAOMI Electronics/i,5],
  ['TP-Link/Tapo','MEDIA',/^TP-LINK /i,6], ['Amazon/Ring','MEDIA',/^Ring LLC$/i,4],
  ['Amazon/Blink','MEDIA',/^Blink by Amazon/i,4], ['Amazon','MEDIA',/^Amazon Technologies/i,4],
  ['Anker/Eufy','MEDIA',/^Smart Innovation LLC$/i,1],
  ['Google/Nest','MEDIA',/^Nest Labs/i,3], ['Google/Nest','MEDIA',/^Google,/i,4],
  ['Tuya','MEDIA',/^Tuya Smart/i,5], ['Espressif','BAJA',/^Espressif/i,10],
  ['Beken','BAJA',/^Beken Corporation$/i,4], ['Realtek','BAJA',/^REALTEK SEMICONDUCTOR/i,3],
  ['Ingenic','BAJA',/^Ingenic Semiconductor/i,2], ['Hisilicon','BAJA',/^Hisilicon Technologies/i,2],
  ['MediaTek','BAJA',/^MediaTek Inc$/i,4],
  ['AltoBeam','BAJA',/^AltoBeam Inc\.$/i,2], ['FN-Link','BAJA',/^FN-LINK TECHNOLOGY/i,2]
];
const entries = [];
for (const [vendor, confianza, re, max] of groups) {
  const rows = registry.filter(r => re.test(r.titular));
  if (!rows.length) throw new Error(`Sin asignación: ${vendor}`);
  for (const r of rows.slice(0,max)) entries.push({...r,vendor,confianza});
}
// Prefijos habituales y chips vistos en cámaras del estudio CICIoT2023.
for (const oui of ['24A160','246F28','30AEA4','A4CF12','AC67B2','7CDFA1','ECFABC','DC4F22','847AB6','2CD26B','C056E3','BCAD28']) {
  if (entries.some(e=>e.oui===oui)) continue;
  const r = registry.find(e=>e.oui===oui);
  if (!r) throw new Error(`Prefijo sin registro IEEE: ${oui}`);
  const g = groups.find(g=>g[2].test(r.titular));
  if (!g) throw new Error(`Titular inesperado: ${oui} ${r.titular}`);
  entries.push({...r,vendor:g[0],confianza:g[1]});
}
if (new Set(entries.map(e=>e.oui)).size !== entries.length) throw new Error('OUI duplicado');
fs.writeFileSync(path.join(root,'spec/oui-ieee.json'), JSON.stringify({fuente:url,fecha:new Date().toISOString().slice(0,10),entries},null,2)+'\n');
const header = `#pragma once
#include "Reglas.h"

// ---------- Tabla OUI ampliable: asignaciones MA-L de IEEE ----------
// Fuente y titulares completos: spec/oui-ieee.json y OUI.md.
// No confundir marca con chip. Foscam y Anyka: sin prefijo verificado.
// Generar y validar con tests/generar_oui.mjs; después revisar los cambios.
struct OuiEntry { uint32_t prefix; const char* vendor; Confidence confidence; };
static const OuiEntry OUI_TABLE[] = {
${entries.map(e=>`  {0x${e.oui}, "${e.vendor}", ${e.confianza}},  // ${e.titular}`).join('\n')}
};
inline const OuiEntry* lookupOui(const uint8_t* mac) {
  if (mac[0] & 0x03) return nullptr;  // local o multidifusión: no atribuir fabricante
  uint32_t prefix = ((uint32_t)mac[0] << 16) | ((uint32_t)mac[1] << 8) | mac[2];
  for (const auto& e : OUI_TABLE) if (e.prefix == prefix) return &e;
  return nullptr;
}
`;
fs.writeFileSync(path.join(root,'CazaCamarasMS/TablaOui.h'),header);
const lines = ['# Tabla OUI · CazaCámaras MS','',`Extracto de [IEEE MA-L](${url}), consultado el ${new Date().toISOString().slice(0,10)}. ${entries.length} prefijos de 24 bits. La selección no es exhaustiva. Titulares exactos en [spec/oui-ieee.json](spec/oui-ieee.json).`,'','| Marca o chip | Confianza | OUI |','|---|---|---|'];
for (const vendor of new Set(entries.map(e=>e.vendor))) {
  const rows=entries.filter(e=>e.vendor===vendor);
  lines.push(`| ${vendor} | ${rows[0].confianza} | ${rows.map(e=>e.oui.match(/../g).join(':')).join(', ')} |`);
}
lines.push('','Imou se vincula a Huacheng mediante su [página oficial de contacto](https://new.imoulife.com/eu/support/contact-us). El prefijo 8C:85:80 figura como Smart Innovation LLC en IEEE; la asociación con Eufy se apoya en la HomeBase 2 del [estudio CICIoT2023, tabla de dispositivos](https://www.mdpi.com/1424-8220/23/13/5941). Es una asociación de producto, no una garantía de que todo el bloque sea Eufy.','',
'Foscam y Anyka: no se han encontrado asignaciones bajo esos nombres en MA-L, MA-M ni MA-S en esta consulta. Se omiten para no inventar OUI. Esto no impide detectar sus redes por patrón o tráfico. Anker no se atribuye a ANKER-EAST, cuyo nombre por sí solo no demuestra relación con Eufy.','',
'AltoBeam y FN-Link se tratan como chips genéricos, nunca como YI aunque algunos modelos los usen. No se incluyen prefijos populares sin registro IEEE (por ejemplo 00:62:6E), ni se atribuye C4:D6:55 a Foscam: IEEE lo asigna a Tercel. Los nombres ALTA/MEDIA/BAJA son reglas del proyecto, no porcentajes certificados.','');
fs.writeFileSync(path.join(root,'OUI.md'),lines.join('\n'));
console.log(`Tabla generada: ${entries.length} OUI verificados en IEEE.`);
