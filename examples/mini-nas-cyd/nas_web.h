#pragma once
#include <pgmspace.h>
const char NAS_HTML[] PROGMEM = R"NASRESOURCE(<!doctype html>
<html lang="es"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="theme-color" content="#091322"><title>Mini NAS · MS Robotika</title>
<style>
:root{color-scheme:dark;--bg:#091322;--card:#112239;--line:#263e58;--text:#edf5fc;--muted:#93abc2;--cyan:#50decc;--blue:#66b5ff;--red:#ff9b9b}*{box-sizing:border-box}body{margin:0;background:radial-gradient(ellipse at top right,#12324a,transparent 55%),var(--bg);color:var(--text);font:15px system-ui,-apple-system,sans-serif;min-height:100vh}main{max-width:1050px;margin:auto;padding:26px 20px 50px}header,.row,.tools,.actions{display:flex;align-items:center;gap:12px;flex-wrap:wrap}header{justify-content:space-between;border-bottom:1px solid var(--line);padding-bottom:20px}.brand{font-size:11px;font-weight:800;letter-spacing:2px;color:var(--cyan)}h1{font-size:30px;margin:6px 0}h2{font-size:19px;margin:0}p{color:var(--muted);line-height:1.6}.badge{border:1px solid #285951;color:var(--cyan);padding:8px 12px;border-radius:30px;font-size:12px}.intro{margin:27px 0 20px}.intro h2{font-size:26px}.stats{display:grid;grid-template-columns:repeat(3,1fr);gap:12px}.stat,.panel{background:var(--card);border:1px solid var(--line);border-radius:18px;padding:20px}.stat small{color:var(--muted)}.stat strong{display:block;margin-top:8px;font-size:24px}.bar{height:6px;background:#243b51;border-radius:8px;overflow:hidden;margin-top:15px}.bar>div{height:100%;width:0;background:linear-gradient(90deg,var(--blue),var(--cyan));transition:width .2s}.grid{display:grid;grid-template-columns:1fr 1.9fr;gap:16px;margin-top:20px}.panel p{font-size:13px}.drop{border:1px dashed #3b697e;border-radius:13px;padding:24px 14px;text-align:center;margin-top:18px}.drop.drag{background:#1b4550}button,.link{background:#1c354e;color:var(--text);border:1px solid #3b556d;border-radius:10px;padding:11px 15px;font:inherit;cursor:pointer;text-decoration:none;display:inline-block}button.primary{background:var(--cyan);color:#07312f;font-weight:800;border-color:var(--cyan)}button:hover,.link:hover{filter:brightness(1.15)}button:disabled{opacity:.45;cursor:default}.danger{color:var(--red)}.tools{justify-content:space-between;margin:18px 0 8px}input[type=search]{background:#0a182a;border:1px solid var(--line);border-radius:10px;padding:11px;color:var(--text);font:inherit;width:100%;margin:15px 0}.file{display:grid;grid-template-columns:40px minmax(0,1fr) auto;gap:10px;align-items:center;border-top:1px solid var(--line);padding:16px 0}.icon{width:36px;height:42px;border-radius:7px;background:#1d3f54;color:var(--cyan);display:grid;place-items:center;font-weight:900;font-size:11px}.file-name{display:block;overflow-wrap:anywhere;font-weight:650}.file small{color:var(--muted)}.actions{gap:6px}.actions button,.actions a{font-size:12px;padding:8px}.empty{padding:30px 0;text-align:center;color:var(--muted)}#message{padding:12px 16px;margin:16px 0;border:1px solid var(--line);border-radius:12px;overflow-wrap:anywhere}#message.error{border-color:#8c4c58;color:#ffb8bc}#transfer{margin-top:16px}#transfer p{margin:8px 0}#preview{max-width:94vw;background:var(--bg);color:var(--text);border:1px solid var(--line);border-radius:16px}#preview img{display:block;max-width:85vw;max-height:72vh;margin:14px auto}footer{margin-top:24px;color:var(--muted);font-size:12px}.pagination{display:flex;justify-content:space-between;align-items:center;margin-top:18px;gap:8px}label{cursor:pointer}.note{font-size:12px;color:var(--muted)}[hidden]{display:none!important}@media(max-width:720px){.grid{grid-template-columns:1fr}.stat{padding:14px}.stat strong{font-size:20px}.stats{gap:8px}.actions{justify-content:flex-end;max-width:110px}.intro h2{font-size:23px}main{padding:20px 14px}.file{grid-template-columns:30px minmax(0,1fr) auto}.icon{width:28px}.panel{padding:17px}h1{font-size:27px}}
</style></head><body><main>
<header><div><div class="brand">MS ROBOTIKA / CYD</div><h1>Mini NAS<span style="color:var(--cyan)">.</span></h1></div><span class="badge" id="connection">Conectando…</span></header>
<section class="intro"><h2>Tus archivos. Tu propia nube local.</h2><p>Del móvil a la microSD de tu ESP32, sin Internet ni aplicaciones adicionales.</p></section>
<div class="stats"><div class="stat"><small>Espacio libre</small><strong id="free">—</strong><div class="bar"><div id="storage-bar"></div></div></div><div class="stat"><small>Capacidad / usada</small><strong id="capacity">—</strong><small id="used">Esperando tarjeta</small></div><div class="stat"><small>Móviles conectados</small><strong id="clients">—</strong><small>Red local de la placa</small></div></div>
<div id="message" role="status" aria-live="polite">Conecta a la Wi-Fi indicada en la pantalla. Usuario: maker; clave: la de la pantalla.</div>
<div class="grid"><section class="panel"><h2>Sube algo tuyo</h2><p>Fotos, documentos, proyectos o cualquier archivo de hasta 128 MiB. Se envían de uno en uno.</p><div class="drop" id="drop"><div style="font-size:32px;color:var(--cyan)">↑</div><p>Elige archivos o arrástralos aquí</p><input type="file" id="picker" multiple hidden><button class="primary" id="choose">Elegir archivos</button></div>
<div id="transfer" hidden><p id="transfer-name"></p><div class="bar" role="progressbar" aria-label="Progreso de subida" aria-valuemin="0" aria-valuemax="100" id="progress"><div id="upload-bar"></div></div><p id="transfer-percent">0 %</p><button id="cancel" class="danger">Cancelar subida</button></div><p class="note">Los archivos existentes se conservan. Si hay un nombre repetido, renómbralo antes de subirlo. Nombres sin tildes ni caracteres especiales.</p></section>
<section class="panel"><div class="tools"><h2>Tu microSD</h2><button id="refresh">Actualizar</button></div><span class="note">Carpeta MSR_NAS · 40 archivos por página</span><input type="search" id="search" placeholder="Buscar en esta página…" aria-label="Buscar archivos en la página"><div id="files" aria-live="polite"><div class="empty">Cargando archivos…</div></div><div class="pagination"><button id="previous" disabled>← Anterior</button><span id="page-label" class="note">Página 1</span><button id="next" disabled>Siguiente →</button></div></section></div>
<footer>AB MakerIDE SUPER · Código abierto · Datos guardados en tu tarjeta, sin envío a servicios externos. No extraigas la microSD durante una transferencia.</footer>
</main><dialog id="preview"><button id="close-preview">Cerrar imagen</button><img id="preview-image" alt="Vista previa del archivo seleccionado"></dialog><script src="/app.js" defer></script></body></html>
)NASRESOURCE";
const char NAS_JS[] PROGMEM = R"NASRESOURCE('use strict';
const $ = id => document.getElementById(id);
const MAX_FILE = 128 * 1024 * 1024;
let token = '', page = 0, items = [], more = false, busy = false, cancelRequested = false, activeXHR = null;
let ready = false, refreshTimer = null, bootstrapped = false;
function bytes(n) {
  n = Number(n);
  if (!Number.isFinite(n) || n < 0) return '—';
  const units = ['B', 'KiB', 'MiB', 'GiB']; let i = 0;
  while (n >= 1024 && i < units.length - 1) { n /= 1024; i++; }
  return `${n.toFixed(i ? 1 : 0)} ${units[i]}`;
}
function validName(name) {
  return typeof name === 'string' && name.length <= 80 && /^[A-Za-z0-9_-][A-Za-z0-9_. -]*$/.test(name) &&
    !/[. ]$/.test(name) && !/^(con|prn|aux|nul|com[1-9]|lpt[1-9])(?:\.|$)/i.test(name);
}
function message(text, error = false) { $('message').textContent = text; $('message').className = error ? 'error' : ''; }
async function api(path, options = {}) {
  const response = await fetch(path, { credentials: 'same-origin', cache: 'no-store', ...options });
  let data;
  try { data = await response.json(); } catch { throw new Error('La placa no ha devuelto una respuesta válida.'); }
  if (!response.ok) throw new Error(data.error || `Error ${response.status}`);
  return data;
}
function controls() {
  $('choose').disabled = busy || !ready; $('refresh').disabled = busy;
  $('previous').disabled = busy || page === 0; $('next').disabled = busy || !more;
  $('search').disabled = busy;
  document.querySelectorAll('.actions button,.actions a').forEach(e => {
    if (e.tagName === 'BUTTON') e.disabled = busy;
    else { e.style.pointerEvents = busy ? 'none' : ''; e.setAttribute('aria-disabled', String(busy)); }
  });
}
async function status() {
  const data = await api('/api/status'); token = data.token; ready = data.sd_ready;
  $('connection').textContent = ready ? '● microSD disponible' : '● microSD no disponible';
  $('free').textContent = ready ? bytes(data.free) : 'Sin tarjeta';
  $('capacity').textContent = ready ? bytes(data.total) : '—';
  $('used').textContent = ready ? `${bytes(data.used)} usados` : 'Inserta una FAT32 con la placa apagada';
  $('clients').textContent = String(data.clients);
  $('storage-bar').style.width = ready && data.total ? `${Math.min(100, data.used / data.total * 100)}%` : '0%';
  controls(); return data;
}
function render() {
  const list = $('files'); list.replaceChildren();
  const query = $('search').value.toLowerCase();
  const filtered = items.filter(f => f.name.toLowerCase().includes(query));
  for (const file of filtered) {
    const row = document.createElement('div'); row.className = 'file';
    const icon = document.createElement('div'); icon.className = 'icon';
    icon.textContent = /\.(jpg|jpeg|png|webp)$/i.test(file.name) ? 'IMG' : 'FILE';
    const info = document.createElement('div'); const title = document.createElement('span');
    title.className = 'file-name'; title.textContent = file.name;
    const size = document.createElement('small'); size.textContent = bytes(file.size); info.append(title, size);
    const actions = document.createElement('div'); actions.className = 'actions';
    const download = document.createElement('a'); download.className = 'link'; download.textContent = 'Bajar';
    download.href = `/api/download?name=${encodeURIComponent(file.name)}`; download.download = file.name;
    actions.append(download);
    if (/\.(jpg|jpeg|png|webp)$/i.test(file.name)) {
      const preview = document.createElement('button'); preview.textContent = 'Ver'; preview.setAttribute('aria-label', `Ver ${file.name}`);
      preview.onclick = () => {
        if (busy) return;
        $('preview-image').src = `/api/preview?name=${encodeURIComponent(file.name)}`;
        $('preview-image').alt = file.name; $('preview').showModal();
      }; actions.append(preview);
    }
    const remove = document.createElement('button'); remove.className = 'danger'; remove.textContent = 'Borrar';
    remove.setAttribute('aria-label', `Borrar ${file.name}`);
    remove.onclick = async () => {
      if (busy || !confirm(`¿Borrar definitivamente "${file.name}" de la microSD?`)) return;
      busy = true; controls();
      try {
        await api(`/api/delete?name=${encodeURIComponent(file.name)}`, { method: 'POST', headers: { 'X-NAS-Token': token } });
        message(`Archivo borrado: ${file.name}`); await load();
      } catch (e) { message(e.message, true); } finally { busy = false; controls(); }
    }; actions.append(remove); row.append(icon, info, actions); list.append(row);
  }
  if (!filtered.length) {
    const empty = document.createElement('div'); empty.className = 'empty';
    empty.textContent = query ? 'Sin coincidencias en esta página.' : ready ? 'Tu espacio está listo. Sube tu primer archivo.' : 'La tarjeta no está disponible.';
    list.append(empty);
  }
  $('page-label').textContent = `Página ${page + 1}`; controls();
}
async function load() {
  await status();
  if (!ready) { items = []; more = false; render(); return; }
  const data = await api(`/api/files?offset=${page * 40}`); items = data.files; more = data.more;
  if (!items.length && page > 0) { page--; return load(); }
  render();
}
function send(file) {
  return new Promise((resolve, reject) => {
    const xhr = new XMLHttpRequest(); activeXHR = xhr;
    xhr.open('POST', '/api/upload'); xhr.timeout = 15 * 60 * 1000;
    xhr.setRequestHeader('Content-Type', 'application/octet-stream');
    xhr.setRequestHeader('X-NAS-Token', token); xhr.setRequestHeader('X-File-Name', file.name);
    xhr.upload.onprogress = e => {
      if (!e.lengthComputable) return;
      const percent = Math.round(e.loaded / e.total * 100);
      $('upload-bar').style.width = `${percent}%`; $('progress').setAttribute('aria-valuenow', String(percent));
      $('transfer-percent').textContent = percent === 100 ? 'Verificando en la microSD…' : `${percent} % · ${bytes(e.loaded)} / ${bytes(e.total)}`;
    };
    xhr.onload = () => {
      let result; try { result = JSON.parse(xhr.responseText); } catch { reject(new Error('Respuesta de subida no válida.')); return; }
      if (xhr.status >= 200 && xhr.status < 300 && result.ok) resolve(result);
      else reject(new Error(result.error || `Error ${xhr.status}`));
    };
    xhr.onerror = () => reject(new Error('Conexión perdida. Comprueba la Wi-Fi de la placa.'));
    xhr.ontimeout = () => reject(new Error('Tiempo de subida agotado.'));
    xhr.onabort = () => reject(new Error('Subida cancelada. Si llegó a completarse, el archivo aparecerá al actualizar.'));
    xhr.send(file);
  });
}
async function upload(fileList) {
  if (busy || !ready || !fileList.length) return;
  const files = Array.from(fileList);
  for (const f of files) {
    if (!validName(f.name)) { message(`Renombra "${f.name}": máximo 80 caracteres, letras sin tildes, números, puntos, guiones y espacios interiores.`, true); return; }
    if (f.size > MAX_FILE) { message(`"${f.name}" supera 128 MiB.`, true); return; }
  }
  busy = true; cancelRequested = false; controls(); $('transfer').hidden = false;
  let count = 0;
  try {
    for (const file of files) {
      if (cancelRequested) break;
      $('transfer-name').textContent = `${count + 1}/${files.length} · ${file.name}`;
      $('upload-bar').style.width = '0%'; $('transfer-percent').textContent = 'Preparando…';
      await send(file); count++;
    }
    message(cancelRequested ? `Cola cancelada. ${count} archivos guardados.` : `${count} archivo(s) guardado(s) en tu microSD.`);
  } catch (e) { message(`${e.message} ${count} archivo(s) completados.`, true); }
  finally {
    activeXHR = null; busy = false; $('transfer').hidden = true; $('picker').value = '';
    try { page = 0; await load(); } catch (e) { message(e.message, true); } controls();
  }
}
$('choose').onclick = () => $('picker').click(); $('picker').onchange = () => upload($('picker').files);
$('cancel').onclick = () => { cancelRequested = true; if (activeXHR) activeXHR.abort(); };
$('refresh').onclick = () => load().catch(e => message(e.message, true));
$('previous').onclick = () => { if (!busy && page > 0) { page--; load().catch(e => message(e.message, true)); } };
$('next').onclick = () => { if (!busy && more) { page++; load().catch(e => message(e.message, true)); } };
$('search').oninput = render;
$('close-preview').onclick = () => { $('preview').close(); $('preview-image').removeAttribute('src'); };
$('preview-image').onerror = () => message('No se puede visualizar esta imagen. Puedes descargarla.', true);
$('drop').ondragover = e => { e.preventDefault(); if (!busy) $('drop').classList.add('drag'); };
$('drop').ondragleave = () => $('drop').classList.remove('drag');
$('drop').ondrop = e => { e.preventDefault(); $('drop').classList.remove('drag'); upload(e.dataTransfer.files); };
async function bootstrap() {
  if (location.hash.startsWith('#pair=')) {
    const pairing = location.hash.slice(6);
    history.replaceState(null, '', '/');
    if (!/^[A-Fa-f0-9]{32}$/.test(pairing)) throw new Error('QR no válido. Pulsa QR WEB en la placa.');
    await api('/api/pair', { method: 'POST', headers: { 'X-NAS-Pair': pairing } });
  }
  await load();
  bootstrapped = true;
  message(ready ? 'Conectado. Tus archivos se guardan únicamente en la microSD.' : 'Conectado a la placa. La microSD no está disponible.');
}
bootstrap().catch(e => message(e.message, true));
refreshTimer = setInterval(() => { if (bootstrapped && !busy) status().catch(() => { $('connection').textContent = '● Sin conexión'; ready = false; controls(); }); }, 10000);
)NASRESOURCE";
