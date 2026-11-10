// Comprobaciones de escritorio. No simulan una prueba física del ESP32.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import tls from 'node:tls';
import https from 'node:https';
import { X509Certificate } from 'node:crypto';

const root = new URL('../', import.meta.url);
const source = fs.readFileSync(new URL('GasolinaMS/GasolinaMS.ino', root), 'utf8');
const cert = source.match(/-----BEGIN CERTIFICATE-----[\s\S]*?-----END CERTIFICATE-----/)[0];
const x509 = new X509Certificate(cert);
assert.equal(x509.fingerprint256.replaceAll(':', ''), 'CB3CCBB76031E5E0138F8DD39A23F9DE47FFC35E43C1144CEA27D46A5AB1CB5F');
assert.ok(x509.verify(x509.publicKey));
assert.ok(!source.includes('setInsecure('));
assert.match(source, /String body = http.getString\(\)/); // Arreglo físico: listas enteras.
assert.doesNotMatch(source.match(/bool readPrices\([^]*?^}/m)[0], /getString\(/);
assert.match(source, /http\.addHeader\("Accept", "application\/json"\)/);
assert.match(source, /DeserializationOption::Filter\(filter\)/);
assert.match(source, /Station best\[5\]/);
assert.match(source, /Sample days\[14\]/);
assert.match(source, /IDPovincia/);
assert.match(source, /a >= 0 && a < S_COUNT/);
assert.deepEqual(fs.readFileSync(new URL('LICENSE', root)), fs.readFileSync(new URL('../kilovatio-ms-cyd/LICENSE', root)));

// Ejecutar el JavaScript real de la página, con DOM y transporte controlados.
const script = source.match(/<script>([\s\S]*?)<\/script>/)[1];
function createPage(handler) {
  const nodes = new Map();
  for (const id of ['estado', 'pais', 'es', 'fr', 'at', 'cp', 'distrito', 'provincia', 'municipio', 'producto', 'litros',
    'ubicacion', 'guardar', 'reintentar', 'ajustes']) {
    nodes.set(id, { value: '', disabled: false, textContent: '', children: [], handlers: {},
      replaceChildren() { this.children = []; this.value = ''; },
      add(option) { this.children.push(option); },
      addEventListener(name, fn) { this.handlers[name] = fn; }
    });
  }
  const context = vm.createContext({
    document: { getElementById: id => nodes.get(id) },
    Option: function (text, value) { this.textContent = text; this.value = value; },
    fetch: handler, URLSearchParams, console
  });
  vm.runInContext(script, context);
  return { nodes, context };
}
const response = body => ({ ok: true, json: async () => body });
const flush = async () => { for (let i = 0; i < 20; i++) await new Promise(resolve => setImmediate(resolve)); };
const calls = [];
const page = createPage(async (url, options) => {
  calls.push({ url, options });
  if (url === '/config') return response({ ok: true, token: calls.filter(c => c.url === '/config').length === 1 ? 'anterior' : 'nuevo',
    provincia: '28', municipio: '4354', producto: 4, litros: 50, ubicacion: 'town' });
  if (url === '/provincias') return response({ ok: true, items: [{ id: '28', nombre: 'MADRID' }] });
  if (url.startsWith('/municipios')) return response({ ok: true, items: [{ id: '4354', nombre: 'Madrid <texto>' }] });
  if (url === '/guardar') return response({ ok: true });
  throw Error('Ruta inesperada');
});
await flush();
assert.equal(page.nodes.get('litros').value, 50);
assert.equal(page.nodes.get('producto').value, '4');
assert.equal(page.nodes.get('municipio').value, '4354');
assert.equal(page.nodes.get('municipio').children[1].textContent, 'Madrid <texto>');
await page.nodes.get('ajustes').handlers.submit({ preventDefault() {} });
const saved = calls.find(c => c.url === '/guardar').options.body;
assert.equal(saved.get('municipio'), '4354');
assert.equal(saved.get('token'), 'nuevo'); // Reinicio entre abrir la página y guardar.
assert.equal(saved.get('litros'), '50');
assert.equal(saved.get('ubicacion'), 'town');
page.nodes.get('ubicacion').value = 'auto';
await page.nodes.get('ajustes').handlers.submit({ preventDefault() {} });
assert.equal(calls.filter(c => c.url === '/guardar').at(-1).options.body.get('ubicacion'), 'auto');
assert.match(page.nodes.get('estado').textContent, /Guardado/);

// Una lista cortada no puede habilitar el guardado.
const failed = createPage(async url => {
  if (url === '/config') return response({ ok: true, token: 'prueba', provincia: '28', producto: 1, litros: 50 });
  if (url === '/provincias') return response({ ok: true, items: [{ id: '28', nombre: 'MADRID' }] });
  return response({ ok: false, items: [{ id: '4354', nombre: 'Parcial' }], error: 'Lista cortada' });
});
await flush();
assert.equal(failed.nodes.get('guardar').disabled, true);
assert.equal(failed.nodes.get('municipio').children.length, 0);
assert.equal(failed.nodes.get('estado').textContent, 'Lista cortada');

// Al cambiar deprisa de provincia, descartar la respuesta anterior.
let oldResolve;
page.context.fetch = async url => url.includes('p=01')
  ? await new Promise(resolve => { oldResolve = resolve; })
  : response({ ok: true, items: [{ id: '200', nombre: 'Pueblo nuevo' }] });
page.nodes.get('provincia').value = '01';
const old = vm.runInContext("towns('')", page.context);
page.nodes.get('provincia').value = '02';
await vm.runInContext("towns('')", page.context);
oldResolve(response({ ok: true, items: [{ id: '100', nombre: 'Pueblo antiguo' }] }));
await old;
assert.equal(page.nodes.get('municipio').children[1].value, '200');
// Francia: código postal en `municipio`, combustible por índice.
const frCalls = [];
const fr = createPage(async (url, options) => {
  frCalls.push({ url, options });
  if (url === '/config') return response({ ok: true, token: 't', pais: 1, provincia: '', municipio: '75015', producto: 2,
    litros: 40, ubicacion: 'auto' });
  if (url === '/guardar') return response({ ok: true });
  throw Error('Ruta inesperada ' + url);
});
await flush();
assert.equal(fr.nodes.get('es').hidden, true);
assert.equal(fr.nodes.get('fr').hidden, false);
assert.equal(fr.nodes.get('cp').value, '75015');
assert.equal(fr.nodes.get('producto').value, '2');
assert.equal(fr.nodes.get('producto').children[0].textContent, 'Gazole (diésel)');
assert.equal(fr.nodes.get('guardar').disabled, false);
await fr.nodes.get('ajustes').handlers.submit({ preventDefault() {} });
const frBody = frCalls.find(c => c.url === '/guardar').options.body;
assert.equal(frBody.get('pais'), '1'); assert.equal(frBody.get('municipio'), '75015'); assert.equal(frBody.get('provincia'), '');

// Austria: distrito de la lista de la placa.
const atCalls = [];
const at = createPage(async (url, options) => {
  atCalls.push({ url, options });
  if (url === '/config') return response({ ok: true, token: 't', pais: 2, provincia: '', municipio: '910', producto: 0,
    litros: 50, ubicacion: 'auto' });
  if (url === '/distritos') return response({ ok: true, items: [{ id: '910', nombre: 'Wien 10.,Favoriten · Wien' }] });
  if (url === '/guardar') return response({ ok: true });
  throw Error('Ruta inesperada ' + url);
});
await flush();
assert.equal(at.nodes.get('at').hidden, false);
assert.equal(at.nodes.get('distrito').value, '910');
assert.equal(at.nodes.get('producto').children.length, 2);  // Sin GNC (se vende por kg).
await at.nodes.get('ajustes').handlers.submit({ preventDefault() {} });
const atBody = atCalls.find(c => c.url === '/guardar').options.body;
assert.equal(atBody.get('pais'), '2'); assert.equal(atBody.get('municipio'), '910');
console.log('OK: CA, licencia, invariantes de fuente y 5 escenarios del JavaScript real de ajustes (España, Francia, Austria).');

function getJson(path) {
  return new Promise((resolve, reject) => {
    const request = https.get('https://sedeaplicaciones.minetur.gob.es/ServiciosRESTCarburantes/PreciosCarburantes' + path,
      { ca: cert, headers: { Accept: 'application/json' }, timeout: 15000 }, r => {
        let body = '';
        r.on('data', chunk => body += chunk);
        r.on('end', () => {
          try { assert.equal(r.statusCode, 200); resolve(JSON.parse(body)); } catch (e) { reject(e); }
        });
      });
    request.on('error', reject);
    request.on('timeout', () => request.destroy(Error('Tiempo agotado')));
  });
}

// HTTP/1.0 real: comprobar la forma del flujo que recibirá HTTPClient.
function getHttp10(path) {
  return new Promise((resolve, reject) => {
    const host = 'sedeaplicaciones.minetur.gob.es';
    const socket = tls.connect({ host, port: 443, servername: host, ca: cert }, () => {
      socket.write('GET /ServiciosRESTCarburantes/PreciosCarburantes' + path + ' HTTP/1.0\r\nHost: ' + host +
        '\r\nAccept: application/json\r\nConnection: close\r\n\r\n');
    });
    let body = Buffer.alloc(0);
    socket.setTimeout(15000, () => socket.destroy(Error('Tiempo agotado')));
    socket.on('data', chunk => { body = Buffer.concat([body, chunk]); });
    socket.on('error', reject);
    socket.on('end', () => {
      try {
        const split = body.indexOf('\r\n\r\n');
        const headers = body.subarray(0, split).toString();
        assert.match(headers, /^HTTP\/1\.[01] 200/);
        assert.doesNotMatch(headers, /transfer-encoding:/i);
        resolve({ json: JSON.parse(body.subarray(split + 4).toString('utf8').replace(/^\uFEFF/, '')),
          bytes: body.length - split - 4 });
      } catch (e) { reject(e); }
    });
  });
}

if (process.argv.includes('--red')) {
  const products = await getJson('/Listados/ProductosPetroliferos/');
  const ids = [1, 3, 4, 5, 17];
  const verified = products.filter(p => ids.includes(Number(p.IDProducto)));
  assert.equal(verified.length, 5);
  const provinces = await getJson('/Listados/Provincias/');
  assert.ok(provinces.some(p => p.IDPovincia === '28' && p.Provincia === 'MADRID'));
  const towns = await getJson('/Listados/MunicipiosPorProvincia/28');
  assert.ok(towns.some(t => t.IDMunicipio === '4354' && t.Municipio === 'Madrid'));
  const { json, bytes } = await getHttp10('/EstacionesTerrestres/FiltroMunicipioProducto/4354/1');
  assert.equal(json.ResultadoConsulta, 'OK');
  const valid = json.ListaEESSPrecio.filter(s => s['Tipo Venta'] === 'P' && /^\d{1,2},\d{1,3}$/.test(s.PrecioProducto))
    .map(s => Number(s.PrecioProducto.replace(',', '.'))).filter(n => n > 0).sort((a, b) => a - b);
  assert.ok(valid.length > 5);
  const evidence = { obtenidoUTC: new Date().toISOString(), caSHA256: x509.fingerprint256,
    productos: verified, provincias: provinces.length, municipiosMadrid: towns.length,
    referenciaEscritorio: { municipio: '4354', producto: 1, fecha: json.Fecha, bytes,
      publicasConPrecio: valid.length, minimo: valid[0], media: valid.reduce((a, b) => a + b) / valid.length,
      maximo: valid.at(-1), cincoPrecios: valid.slice(0, 5) },
    alcance: 'API real desde Node con la CA del sketch; HTTP/1.0 sin chunked. No ejecuta el parser C++ ni el ESP32.' };
  fs.writeFileSync(new URL('specs/red-verificada.json', root), JSON.stringify(evidence, null, 2) + '\n');
  console.log('OK: productos, provincias, municipios y precios reales por TLS con la CA del sketch y Accept JSON.');
  console.log(JSON.stringify(evidence.referenciaEscritorio));
}
