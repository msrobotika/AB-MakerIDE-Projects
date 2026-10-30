'use strict';
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
