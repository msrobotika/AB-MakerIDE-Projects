// Escritorio: criptografía, regresiones de código y lógica traducida mecánicamente a JS.
// NO compila C++, NO ejecuta el parser ArduinoJson, NO simula RF/ESP32.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import tls from 'node:tls';
import https from 'node:https';
import { X509Certificate, createHash } from 'node:crypto';
const root = new URL('../', import.meta.url);
const source = fs.readFileSync(new URL('GasolinaMS/GasolinaMS.ino', root), 'utf8').replaceAll('\r\n', '\n');
const functionText = name => {
  const match = source.match(new RegExp('^(?:bool|String|void|float|uint8_t|NearChoice) '+name+'\\([^]*?^}', 'm'));
  assert.ok(match, name); return match[0];
};
const protectedHashes = JSON.parse(fs.readFileSync(new URL('specs/near-me/protected-sha256.json', root)));
for (const [name, hash] of Object.entries(protectedHashes)) {
  const text = ['BufferedInput', 'Strip', 'Config'].includes(name)
    ? source.match(new RegExp('^(?:class|struct) '+name+'[^]*?^};', 'm'))[0] : functionText(name);
  assert.equal(createHash('sha256').update(text).digest('hex'), hash, 'Regresión en '+name);
}
const certs = [...source.matchAll(/-----BEGIN CERTIFICATE-----[^]*?-----END CERTIFICATE-----/g)]
  .map(m => new X509Certificate(m[0]));
assert.equal(certs.length, 6);  // Ministerio, beaconDB (X2 + YE), Francia (USERTrust), Austria (HARICA 2015 + 2021).
assert.equal(certs[3].fingerprint256.replaceAll(':',''), 'E793C9B02FD8AA13E21C31228ACCB08119643B749C898964B1746D46C3D4CBD2');
assert.equal(certs[4].fingerprint256.replaceAll(':',''), 'A040929A02CE53B4ACF4F2FFC6981CE4496F755E6D45FE0B2A692BCD52523F36');
assert.equal(certs[5].fingerprint256.replaceAll(':',''), 'D95D0E8EDA79525BF9BEB11B14D2100D3294985F0C62D9FABD9CD999ECCB7B1D');
assert.match(source, /setCACert\(ca\)/);
assert.equal(certs[1].fingerprint256.replaceAll(':',''), '69729B8E15A86EFC177A57AFB7171DFC64ADD28C2FCA8CF1507E34453CCB1470');
assert.equal(certs[2].fingerprint256.replaceAll(':',''), 'E14FFCAD5B0025731006CAA43A121A22D8E9700F4FB9CF852F02A708AA5D5666');
for (const c of certs) assert.ok(c.ca && c.verify(c.publicKey));
assert.match(source, /client.setCACert\(BEACON_CA\)/);
assert.doesNotMatch(source, /setInsecure\s*\(/);
assert.match(source, /WiFi.scanNetworks\(false, true\)/);
assert.match(source, /request\["considerIp"\] = false/);
assert.match(source, /Gasolina-MS\/1.0 \(CYD; github.com\/msrobotika\/AB-MakerIDE-Projects\)/);
assert.match(functionText('refreshProvince'), /BufferedInput input\(http.getStream\(\)\)/);
assert.doesNotMatch(functionText('refreshProvince'), /getString/);
assert.match(functionText('readPrices'), /nearCount == nearCapacity/);
assert.match(functionText('readPrices'), /DeserializationOption::Filter\(filter\)/);
assert.match(functionText('readPrices'), /parseCoordinate\(item\["Longitud \(WGS84\)"\]/);
assert.match(source, /enum Screen \{ S_NEAR, S_NOW, S_TOP, S_TREND, S_SETTINGS, S_COUNT \}/);
assert.doesNotMatch(source, /lcd\.(?!begin\(|setRotation\()/);
assert.doesNotMatch(source, /prefs\.put\w+\([^\n]*(?:positionLat|positionLon|positionAccuracy)/);
const locate = functionText('locateByWifi');
assert.doesNotMatch(locate, /Serial\.|\bput\(|gfx->|WiFi.disconnect/);
assert.match(functionText('refreshPosition'), /Serial.println\("sin estimacion"\)/);

// Un subconjunto puro del C++ real, traducido a JS para comprobar casos límite del algoritmo.
// Es prueba de lógica; no sustituye la compilación ni valida float32/ArduinoJson.
const capitals = [...source.match(/const Capital CAPITALS\[\] = \{([^]*?)\n};/)[1]
  .matchAll(/\{(\d+),([\d.-]+)f,([\d.-]+)f\}/g)]
  .map(m => ({ id: +m[1], lat: +m[2], lon: +m[3] }));
assert.deepEqual(capitals.map(c => c.id), Array.from({length:52}, (_,i) => i+1));
function translate(name) {
  return functionText(name)
    .replace(/^(?:float|uint8_t|NearChoice|void) (\w+)\(([^)]*)\)/, (_, n, args) =>
      'function '+n+'('+args.replace(/float /g,'')+')')
    .replace(/constexpr float |float |uint8_t |bool /g, 'let ')
    .replace(/(\d+(?:\.\d+)?)f\b/g, '$1')
    .replace(/for \(const Capital& c : CAPITALS\)/g, 'for (const c of CAPITALS)')
    .replace('NearChoice out;', 'let out = {cheapest:-1,closest:-1,radius:5,cheapKm:0,closeKm:0};')
    .replace(/for \(int radius : \{5, 10, 20\}\)/g, 'for (const radius of [5,10,20])')
    .replace(/\bint /g, 'let ');
}
let nextLocate = true, clock = 0;
const context = vm.createContext({ CAPITALS: capitals, sinf: Math.sin, cosf: Math.cos, asinf: Math.asin,
  sqrtf: Math.sqrt, constrain: (v,a,b)=>Math.min(b,Math.max(a,v)),
  havePosition: true, provinceValid: true, nearCount: 0, nearStations: [], cachedProvince:28, positionProvince:28,
  cachedFuel:0, config:{fuel:0}, positionLat:40.4168, positionLon:-3.7038, positionAccuracy:100,
  positionWanted:true, lastPositionTry:0, positionOld:false, positionFailures:0, provinceWanted:false,
  nearError:[''], screen:1, townLat:0, townLon:0, positionTown:false, country:0, fetchLat:0, fetchLon:0,
  beaconCode:0, beaconNets:0, S_NEAR:0, S_NOW:1, millis:()=>++clock, locateByWifi:()=>nextLocate,
  Serial:{printf(){},println(){}} });
for (const name of ['distanceKm','nearestProvince','chooseNear','refreshPosition']) vm.runInContext(translate(name), context);
const run = code => vm.runInContext(code, context);
assert.equal(run('distanceKm(40,-3,40,-3)'), 0);
assert.ok(Math.abs(run('distanceKm(0,0,0,1)')-111.195)<0.01);
for (const c of capitals) assert.equal(run(`nearestProvince(${c.lat},${c.lon})`), c.id);
const station = (km,mills) => ({ lat:40.4168+km/111.195, lon:-3.7038, mills });
function choose(items) {
  context.nearStations=items; context.nearCount=items.length;
  return run('chooseNear()');
}
assert.equal(choose([]).closest,-1);
let result = choose([station(1,1800),station(4,1500),station(7,1400)]);
assert.equal(result.cheapest,1); assert.equal(result.closest,0); assert.equal(result.radius,5);
assert.equal((1800-1500)*0.05,15); // 50 L, no litros configurables.
assert.equal(choose([station(7,1400)]).radius,10);
assert.equal(choose([station(15,1400)]).radius,20);
result=choose([station(21,1400)]); assert.equal(result.cheapest,-1); assert.equal(result.closest,0);
result=choose([station(4,1500),station(1,1500)]); assert.equal(result.cheapest,1); assert.equal(result.closest,1);
context.cachedFuel=1; assert.equal(choose([station(1,1000)]).closest,-1); context.cachedFuel=0;
context.cachedProvince=45; assert.equal(choose([station(1,1000)]).cheapest,-1); context.cachedProvince=28;
nextLocate=false; context.screen=0;
run('refreshPosition()'); assert.equal(context.positionFailures,1); assert.equal(context.havePosition,true); assert.equal(context.positionOld,true);
run('refreshPosition()'); assert.equal(context.havePosition,true);
run('refreshPosition()'); assert.equal(context.havePosition,false); assert.equal(context.screen,1); assert.equal(context.positionLat,0);
nextLocate=true; context.positionLat=40.4168; context.positionLon=-3.7038;
run('refreshPosition()'); assert.equal(context.havePosition,true); assert.equal(context.positionFailures,0); assert.equal(context.screen,0);
context.screen=4; run('refreshPosition()'); assert.equal(context.screen,4);
context.positionLat=39.8628; context.positionLon=-4.0273;
run('refreshPosition()'); assert.equal(context.positionProvince,45); assert.equal(context.provinceValid,false); assert.equal(context.provinceWanted,true);
// Sin Wi-Fi: dos fallos mantienen la posición anterior; el tercero pasa al centro del pueblo y el Wi-Fi lo sustituye.
context.townLat=40.2; context.townLon=-3.9; nextLocate=false;
run('refreshPosition()'); run('refreshPosition()'); assert.equal(context.positionTown,false); assert.equal(context.positionOld,true);
run('refreshPosition()'); assert.equal(context.positionTown,true); assert.equal(context.havePosition,true); assert.equal(context.positionLat,40.2);
run('refreshPosition()'); assert.equal(context.positionTown,true); assert.equal(context.positionOld,false);
nextLocate=true; context.positionLat=40.4168; context.positionLon=-3.7038;
run('refreshPosition()'); assert.equal(context.positionTown,false); assert.equal(context.positionFailures,0);
// Filtrado y ordenación del bloque real de escaneo, también traducido, con redes sintéticas.
context.memcmp = (a,b,n) => a.slice(0,n).every((v,i)=>v===b[i]) ? 0 : 1;
context.memcpy = (a,b,n) => a.splice(0,n,...b.slice(0,n));
context.min = Math.min;
vm.runInContext(functionText('usableAP').replace(/^bool usableAP\([^]*?\) \{/,
  'function usableAP(mac, connected, ssid) {').replace(/uint8_t /g,'let ').replace(/\bint /g,'let '),context);
const connected = [0,17,34,51,68,255]; context.connected=connected;
for (const first of [2,6,10,14,0x12,0x26,0x3a,0x4e,1,3])
  assert.equal(run(`usableAP([${first},1,2,3,4,5],connected,'red')`),false);
assert.equal(run("usableAP([0,0,0,0,0,0],connected,'red')"),false);
assert.equal(run("usableAP(connected,connected,'hotspot')"),false);
assert.equal(run("usableAP([0,1,2,3,4,5],connected,'privada_nomap')"),false);
assert.equal(run("usableAP([0,1,2,3,4,5],connected,'')"),true);
const networks=Array.from({length:30},(_,i)=>({mac:[0,17,34,51,68,i],rssi:-90+i,ssid:'sintetica'}));
networks.push({mac:connected,rssi:-1,ssid:'hotspot'}, {mac:[2,1,2,3,4,5],rssi:-2,ssid:'local'},
  {mac:[0,1,2,3,4,5],rssi:-3,ssid:'oculta_nomap'}, {...networks[29],rssi:-40});
context.WiFi={scanNetworks:(asyncFlag,hidden)=>{assert.equal(asyncFlag,false);assert.equal(hidden,true);return networks.length;},
  BSSID:i=>i===undefined?connected:networks[i].mac, SSID:i=>networks[i].ssid,RSSI:i=>networks[i].rssi,scanDelete(){}};
let selection=locate.slice(locate.indexOf('  int used = 0;'),locate.indexOf('  if (!used ||'));
selection=selection.replace(/const uint8_t\* /g,'const ').replace(/\bint32_t |\bint /g,'let ')
  .replace(/strongest\[j\] = strongest\[j ([+-]) 1\]/g,'strongest[j] = structuredClone(strongest[j $1 1])');
context.structuredClone=structuredClone;
const selected=run('(function(){const strongest=Array.from({length:15},()=>({mac:Array(6).fill(0),rssi:0}));let current;'+selection+'return strongest.slice(0,used);})()');
assert.equal(selected.length,15); assert.equal(selected[0].rssi,-40);
assert.equal(new Set(selected.map(s=>s.mac.join(':'))).size,15);
assert.ok(selected.every((s,i)=>i===0||s.rssi<=selected[i-1].rssi));
assert.equal(selected.at(-1).rssi,-75);
console.log('OK: 10 bloques protegidos, 6 raíces CA, invariantes y lógica traducida de AP/distancia/radios/provincia/fallos/centro del pueblo. No compila C++.');

if (process.argv.includes('--red')) {
  const ca=certs.slice(1).map(c=>c.toString()).join('\n');
  const beacon=await new Promise((resolve,reject)=>{
    const body=JSON.stringify({considerIp:false,wifiAccessPoints:[]}); // Ninguna red real ni posición enviada.
    const req=https.request('https://api.beacondb.net/v1/geolocate',{method:'POST',ca,
      headers:{'User-Agent':'Gasolina-MS/1.0 (CYD; github.com/msrobotika/AB-MakerIDE-Projects)',
        'Content-Type':'application/json','Content-Length':Buffer.byteLength(body)},timeout:15000},r=>{
      const authorized=r.socket.authorized, chain=[]; let c=r.socket.getPeerCertificate(true);
      const seen=new Set();
      while(c?.raw&&!seen.has(c.fingerprint256)) {
        seen.add(c.fingerprint256);chain.push({subject:c.subject.CN,issuer:c.issuer.CN,sha256:c.fingerprint256});c=c.issuerCertificate;
      }
      r.resume(); r.on('end',()=>resolve({status:r.statusCode,tls:authorized,chain}));
    }); req.on('error',reject); req.on('timeout',()=>req.destroy(Error('timeout'))); req.end(body);
  });
  assert.equal(beacon.status,404);
  assert.equal(beacon.tls,true);
  const host='sedeaplicaciones.minetur.gob.es';
  const province=await new Promise((resolve,reject)=>{
    const s=tls.connect({host,port:443,servername:host,ca:certs[0].toString()},()=>s.write(
      'GET /ServiciosRESTCarburantes/PreciosCarburantes/EstacionesTerrestres/FiltroProvinciaProducto/28/1 HTTP/1.0\r\nHost: '+host+
      '\r\nAccept: application/json\r\nConnection: close\r\n\r\n'));
    const chunks=[]; s.on('data',c=>chunks.push(c)); s.on('error',reject);
    s.setTimeout(15000,()=>s.destroy(Error('timeout')));
    s.on('end',()=>{try {
      const b=Buffer.concat(chunks), split=b.indexOf('\r\n\r\n'), header=b.subarray(0,split).toString();
      assert.match(header,/^HTTP\/1\.[01] 200/); assert.doesNotMatch(header,/transfer-encoding:/i);
      const json=JSON.parse(b.subarray(split+4).toString('utf8').replace(/^\uFEFF/,''));
      assert.equal(json.ResultadoConsulta,'OK');
      const publicStations=json.ListaEESSPrecio.filter(s=>s['Tipo Venta']==='P' && /^\d{1,2},\d{1,3}$/.test(s.PrecioProducto));
      assert.ok(publicStations.length>500 && publicStations.length<=1200);
      assert.ok(publicStations.every(s=>Number.isFinite(Number(s.Latitud.replace(',','.'))) &&
        Number.isFinite(Number(s['Longitud (WGS84)'].replace(',','.')))));
      resolve({bytes:b.length-split-4,total:json.ListaEESSPrecio.length,publicas:publicStations.length,fecha:json.Fecha});
    } catch(e) {reject(e);}});
  });
  const evidence={utc:new Date().toISOString(),beacon,province,
    alcance:'Servicios y CA reales desde Node. Sin escaneo ni posición real; no ejecuta el parser C++ ni el ESP32.'};
  fs.writeFileSync(new URL('specs/near-me/network-verified.json',root),JSON.stringify(evidence,null,2)+'\n');
  console.log('OK: beaconDB TLS/404 sin redes, provincia Madrid HTTP/1.0 JSON:',province);
}
