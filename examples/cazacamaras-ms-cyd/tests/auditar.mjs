// Auditoría estática reproducible. No ejecuta el firmware ni simula la placa.
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
const root = path.resolve(import.meta.dirname,'..');
const repo = path.resolve(root,'../..');
const read = p => fs.readFileSync(path.join(root,p),'utf8');
const ino = read('CazaCamarasMS/CazaCamarasMS.ino');
const rules = read('CazaCamarasMS/Reglas.h');
const oui = read('CazaCamarasMS/TablaOui.h');
const source = fs.readFileSync(path.join(repo,'examples/vigia-ms-cyd/VigiaMS/VigiaMS.ino'),'utf8');
let count = 0;
const check = (name, fn) => { fn(); ++count; console.log(`OK · ${name}`); };
// Se eliminan cadenas y comentarios antes de revisar llaves o llamadas.
const codeOnly = s => s.replace(/\/\*[\s\S]*?\*\/|\/\/[^\n]*|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'/g, m=>m.replace(/[^\n]/g,' '));
function body(s,name) {
  const clean=codeOnly(s);
  const re=new RegExp(`\\b${name}\\([^;{}]*\\)[^{;]*\\{`,'g');
  const m=re.exec(clean); assert(m, `Función no encontrada: ${name}`);
  const start=clean.indexOf('{',m.index); let depth=1,i=start+1;
  while (i<clean.length && depth) { if(clean[i]==='{')++depth; if(clean[i]==='}')--depth; ++i; }
  assert.equal(depth,0); return s.slice(start,i).replace(/\r\n/g,'\n');
}
check('Vigía original intacto respecto a Git',()=>{
  const baseline=execFileSync('git',['show','HEAD:examples/vigia-ms-cyd/VigiaMS/VigiaMS.ino'],{cwd:repo});
  assert.equal(source.replace(/\r\n/g,'\n'),baseline.toString('utf8').replace(/\r\n/g,'\n'));
});
check('LICENSE copiado byte a byte',()=>assert.deepEqual(fs.readFileSync(path.join(root,'LICENSE')),fs.readFileSync(path.join(repo,'examples/vigia-ms-cyd/LICENSE'))));
check('Ayudantes originales de texto, táctil, LED, brillo y sonido',()=>{
  for(const name of ['mapUtf8','textWidth','put','putCenter','putRight','sampleRaw','toScreen','setLed','updateBrightness','beep']) assert.equal(body(ino,name),body(source,name),name);
});
check('Mismos pines, buses y calibración de tres puntos',()=>{
  for(const text of ['SPIClass lcdBus(HSPI), touchBus(VSPI);','Adafruit_ILI9341 lcd(&lcdBus, 2, 15, -1);','XPT2046_Touchscreen touch(33, 36);','LED_R = 4, LED_G = 16, LED_B = 17','PIN_BL = 21, PIN_LDR = 34, PIN_SPK = 26','lcdBus.begin(14, 12, 13, 15)','touchBus.begin(25, 39, 32, 33)','const int sx[3] = {24, 296, 24}, sy[3] = {24, 24, 216};']) assert(ino.includes(text),text);
  for(const line of body(source,'calibrate').split('\n').filter(l=>/^    (ax|bx|cx|ay|by|cy) =/.test(l))) assert(ino.includes(line));
});
check('Llaves, paréntesis y corchetes equilibrados en C++',()=>{
  for(const file of ['CazaCamarasMS/CazaCamarasMS.ino','CazaCamarasMS/Reglas.h','CazaCamarasMS/TablaOui.h','tests/reglas_test.cpp']) {
    const stack=[]; for(const c of codeOnly(read(file))) {
      if('({['.includes(c)) stack.push(c);
      if(')}]'.includes(c)) assert.equal(stack.pop(),'({['[')}]'.indexOf(c)],file);
    } assert.equal(stack.length,0,file);
  }
});
check('OUI reales: tabla idéntica al extracto IEEE sin duplicados',()=>{
  const data=JSON.parse(read('spec/oui-ieee.json'));
  assert.equal(data.fuente,'https://standards-oui.ieee.org/oui/oui.csv');
  const entries=[...oui.matchAll(/\{0x([A-F0-9]{6}), "([^"]+)", (ALTA|MEDIA|BAJA)\},  \/\/ (.+)/g)];
  assert.equal(entries.length,data.entries.length); assert(entries.length>=100);
  assert.equal(new Set(entries.map(m=>m[1])).size,entries.length);
  for(const m of entries) {
    assert.equal(parseInt(m[1].slice(0,2),16)&3,0);
    assert(data.entries.some(e=>e.oui===m[1]&&e.vendor===m[2]&&e.confianza===m[3]&&e.titular===m[4]));
  }
});
check('Todos los patrones SSID pedidos y solo etiquetas de patrón',()=>{
  const expected=['IPC','IPCAM','HIK','EZVIZ','DAHUA','IMOU','REOLINK','V380','WIFICAM','HDWIFICAM','LOOKCAM','IPC365','YI-','TAPO_CAM','CAMERA','CAM-','JXL'];
  const patterns=[...rules.match(/NET_PATTERNS\[\] = \{([\s\S]*?)\}/)[1].matchAll(/"([^"]+)"/g)].map(m=>m[1]);
  assert.deepEqual([...patterns].sort(),expected.sort());
  assert(ino.includes('cameraPattern(WiFi.SSID(i).c_str())'));
  assert(ino.includes('NET_PATTERNS[t.pattern]'));
});
check('WiFi y BLE pasivos; no conexión ni transmisión',()=>{
  const code=codeOnly(ino);
  for(const name of ['esp_wifi_connect','esp_wifi_80211_tx','esp_wifi_deauth_sta','esp_ble_gap_start_advertising']) assert(!new RegExp(`\\b${name}\\s*\\(`).test(code),name);
  assert(!/WiFi\.(begin|softAP|reconnect|disconnect)\s*\(/.test(code));
  assert(!/createClient|createServer|getClient|setActiveScan\(true/.test(code));
  assert(ino.includes('WiFi.scanNetworks(true, true, true, 120, 0)'));
  assert(ino.includes('scan->setActiveScan(false)'));
  assert(ino.includes('WiFi.setAutoReconnect(false)'));
  assert(ino.includes('WiFi.persistent(false)'));
  assert(ino.includes('esp_wifi_set_storage(WIFI_STORAGE_RAM)'));
});
check('Callback WiFi acotado y protegido',()=>{
  const b=codeOnly(body(ino,'wifiRx'));
  assert(!/Serial|lcd|prefs|lookupOui|String|new\s|malloc|delay/.test(b));
  assert(b.includes('portENTER_CRITICAL')); assert(b.includes('portEXIT_CRITICAL'));
  assert(b.includes('ringHead')); assert(b.includes('wifiDrops')); assert(b.includes('h + 10'));
});
check('Serie limitada a fabricante, RSSI y confianza',()=>{
  const calls=[...ino.matchAll(/Serial\.(printf|print|println|write)\s*\(/g)];
  assert.equal(calls.length,1); assert.equal(calls[0][1],'printf');
  assert(ino.includes('Serial.printf("%s | %d dBm | %s\\n"'));
  assert(!body(ino,'reportNew').match(/t\.addr|SSID|\.getName/));
});
check('NVS solo para ajustes y calibración',()=>{
  const keys=[...ino.matchAll(/prefs\.put\w+\("([^"]+)"/g)].map(m=>m[1]);
  assert.deepEqual([...new Set(keys)].sort(),['auto','ax','ay','bx','by','cal','cx','cy','sound']);
  assert(!/File\s|SPIFFS|LittleFS|SD\.begin|EEPROM/.test(codeOnly(ino)));
});
check('BUSCAR fija canal, evita redes y pausa al calibrar',()=>{
  const b=body(ino,'radioTick');
  assert(b.includes('screen == 3')); assert(b.includes('targets[selected].channel'));
  assert(b.includes('findMode && scanning')); assert(b.includes('!findMode && now - lastScan'));
  assert(body(ino,'onTap').includes('pauseRadios(); noTone(PIN_SPK); calibrate();'));
});
check('Continuidad de vídeo se corta al perder muestras o pausar',()=>{
  assert(body(ino,'radioTick').includes('drops != lastDrops) { resetTraffic();'));
  assert(body(ino,'cancelNetworkScan').includes('resetTraffic()'));
  assert(body(ino,'pauseRadios').includes('resetTraffic()'));
  assert(body(ino,'ingest').includes('t.run.reset()'));
  assert(rules.includes('now - since > 10000'));
  assert(rules.includes('now - last > 6000'));
});
check('Textos literales dentro de 320×240',()=>{
  for(const m of ino.matchAll(/\b(put|putCenter|putRight)\((\d+),\s*(\d+),\s*"([^"]*)",\s*\w+(?:,\s*(\d+))?/g)) {
    const size=+(m[5]||1),width=[...m[4]].length*6*size;
    const x=+m[2],y=+m[3]; const left=m[1]==='putCenter'?x-width/2:m[1]==='putRight'?x-width:x;
    assert(left>=0 && left+width<=320 && y>=0 && y+8*size<=240,`${m[4]}: ${left}, ${y}, ${width}`);
  }
});
check('Seis pantallas, castellano y límites documentados',()=>{
  for(const label of ['ESCANEO','RADAR','LISTA','BUSCAR','CONSEJOS','AJUSTES','FRÍO','TEMPLADO','¡CALIENTE!','Es mío / ignorar']) assert(ino.includes(`"${label}"`));
  const readme=read('README.md');
  for(const text of ['V380-TEST','solo graban en SD','cámaras por cable','vecino','OUI','Evidencia: pendiente de prueba en la placa','huge_app']) assert(readme.includes(text),text);
});
console.log(`\n${count} comprobaciones estáticas superadas. No equivalen a compilación ni prueba física.`);
