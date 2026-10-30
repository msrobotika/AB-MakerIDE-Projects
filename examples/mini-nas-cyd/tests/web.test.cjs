// Unit tests for the actual browser script using minimal DOM/network doubles.
// These do not execute the ESP32 or prove the SD/radio/display works.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source = fs.readFileSync(require('node:path').join(__dirname, '../web/app.js'), 'utf8');
const flush = () => new Promise(resolve => setImmediate(resolve));
async function environment(initial = [], sd = true, settings = {}) {
  const elements = new Map(), created = [], files = new Map(initial.map(f => [f.name, f.size]));
  const requests = [], uploads = [], historyEdits = []; let confirmResult = true, uploadMode = 'success';
  class Element {
    constructor(tagName = 'DIV') { this.tagName = tagName.toUpperCase(); this.children = []; this.style = {}; this.value = ''; this.textContent = ''; this.disabled = false; this.hidden = false; this.classList = { add() {}, remove() {} }; }
    append(...items) { this.children.push(...items); }
    replaceChildren(...items) { this.children = items; }
    setAttribute(k, v) { this[k] = v; }
    removeAttribute(k) { delete this[k]; }
    click() { if (this.onclick) return this.onclick(); }
    showModal() { this.open = true; }
    close() { this.open = false; }
  }
  const element = id => { if (!elements.has(id)) elements.set(id, new Element()); return elements.get(id); };
  class XHR {
    constructor() { this.upload = {}; this.headers = {}; }
    open(method, path) { this.method = method; this.path = path; }
    setRequestHeader(k, v) { this.headers[k] = v; }
    send(file) {
      uploads.push({ file, headers: this.headers, method: this.method, path: this.path });
      if (uploadMode === 'hold') return;
      queueMicrotask(() => {
        if (files.has(file.name)) { this.status = 409; this.responseText = JSON.stringify({ error: 'Ese nombre ya existe.' }); }
        else { files.set(file.name, file.size); this.status = 201; this.responseText = '{"ok":true}'; }
        if (this.upload.onprogress) this.upload.onprogress({ lengthComputable: true, loaded: file.size, total: file.size });
        this.onload();
      });
    }
    abort() { this.onabort(); }
  }
  const context = vm.createContext({
    document: { getElementById: element, createElement(tag) { const e = new Element(tag); created.push(e); return e; }, querySelectorAll() { return created.filter(e => e.tagName === 'BUTTON' || e.tagName === 'A'); } },
    fetch: async (path, options) => {
      requests.push({ path, options }); const url = new URL(path, 'http://device'); let data;
      if (url.pathname === '/api/pair') {
        assert.equal(options.method, 'POST'); assert.equal(options.headers['X-NAS-Pair'], settings.hash.slice(6));
        if (settings.pairFails) return { ok: false, json: async () => ({ error: 'QR usado o caducado.' }) };
        data = { ok: true };
      }
      else if (url.pathname === '/api/status') data = { sd_ready: sd, token: 'test-token', total: 1e9, used: 1e8, free: 9e8, clients: 1 };
      else if (url.pathname === '/api/files') { const offset = Number(url.searchParams.get('offset')); const names = [...files].map(([name, size]) => ({ name, size })); data = { files: names.slice(offset, offset + 40), more: names.length > offset + 40 }; }
      else if (url.pathname === '/api/delete') { assert.equal(options.method, 'POST'); assert.equal(options.headers['X-NAS-Token'], 'test-token'); files.delete(url.searchParams.get('name')); data = { ok: true }; }
      else throw new Error(`Unexpected endpoint ${path}`);
      return { ok: true, json: async () => data };
    },
    location: { hash: settings.hash || '' }, history: { replaceState(...args) { historyEdits.push(args); } },
    XMLHttpRequest: XHR, setInterval: () => 1, confirm: () => confirmResult,
  });
  vm.runInContext(source, context); await flush();
  return { run: code => vm.runInContext(code, context), element, created, files, requests, uploads, historyEdits,
    confirm: value => { confirmResult = value; }, mode: value => { uploadMode = value; } };
}
test('names reject traversal, reserved names and size limit; accept camera/project names', async () => {
  const e = await environment();
  for (const name of ['IMG_1234.JPG', 'Mi proyecto-v2.zip', 'a', '-', 'CONcert.txt', 'a'.repeat(80)]) assert.equal(e.run(`validName(${JSON.stringify(name)})`), true, name);
  for (const name of ['', '../secret', 'a/b', 'a\\b', '%2e%2e', '.upload.part', 'file.', ' file', 'file ', 'CON.txt', 'LPT9', 'a\r\nHeader', 'a"b', 'a'.repeat(81)]) assert.equal(e.run(`validName(${JSON.stringify(name)})`), false, name);
});
test('pagination and search use server pages and render text elements', async () => {
  const e = await environment(Array.from({ length: 45 }, (_, i) => ({ name: `photo-${i}.jpg`, size: 100 })));
  assert.equal(e.element('files').children.length, 40); assert.equal(e.element('next').disabled, false);
  e.element('next').onclick(); await flush(); assert.equal(e.element('files').children.length, 5);
  e.element('search').value = 'photo-44'; e.element('search').oninput(); assert.equal(e.element('files').children.length, 1);
  assert.equal(e.element('files').children[0].children[1].children[0].textContent, 'photo-44.jpg');
});
test('multiple files go sequentially as authenticated binary requests; empty file supported', async () => {
  const e = await environment(); await e.run('upload([{name:"photo.jpg",size:12},{name:"empty.txt",size:0}])');
  assert.deepEqual([...e.files.keys()], ['photo.jpg', 'empty.txt']); assert.equal(e.uploads.length, 2);
  for (const r of e.uploads) { assert.equal(r.method, 'POST'); assert.equal(r.path, '/api/upload'); assert.equal(r.headers['Content-Type'], 'application/octet-stream'); assert.equal(r.headers['X-NAS-Token'], 'test-token'); assert.equal(r.headers['X-File-Name'], r.file.name); }
  assert.equal(e.run('busy'), false); assert.match(e.element('message').textContent, /2 archivo/);
});
test('duplicate stops queue, reports error and preserves original', async () => {
  const e = await environment([{ name: 'exists.txt', size: 44 }]); await e.run('upload([{name:"exists.txt",size:99},{name:"next.txt",size:1}])');
  assert.equal(e.files.get('exists.txt'), 44); assert.equal(e.files.has('next.txt'), false); assert.equal(e.element('message').className, 'error');
});
test('invalid name or excess size never sends upload', async () => {
  const e = await environment(); await e.run('upload([{name:"../bad",size:1}])'); await e.run('upload([{name:"large.bin",size:128*1024*1024+1}])'); assert.equal(e.uploads.length, 0);
});
test('cancel aborts XHR and stops remaining queue', async () => {
  const e = await environment(); e.mode('hold'); const pending = e.run('upload([{name:"a.txt",size:20},{name:"b.txt",size:20}])');
  e.element('cancel').onclick(); await pending; assert.equal(e.files.size, 0); assert.equal(e.uploads.length, 1); assert.match(e.element('message').textContent, /cancelada/); assert.equal(e.run('busy'), false);
});
test('delete requires confirmation and sends token to one named file', async () => {
  const e = await environment([{ name: 'a.txt', size: 1 }]); const remove = e.created.find(x => x.textContent === 'Borrar');
  e.confirm(false); await remove.onclick(); assert.equal(e.files.has('a.txt'), true);
  e.confirm(true); await remove.onclick(); assert.equal(e.files.has('a.txt'), false);
});
test('missing SD disables upload and displays absence', async () => {
  const e = await environment([], false); assert.equal(e.element('choose').disabled, true); assert.match(e.element('message').textContent, /no está disponible/); await e.run('upload([{name:"a.txt",size:1}])'); assert.equal(e.uploads.length, 0);
});
test('QR bootstrap removes fragment and exchanges it in header before reading private API', async () => {
  const hash = '#pair=0123456789ABCDEF0123456789ABCDEF';
  const e = await environment([], true, { hash });
  assert.equal(e.historyEdits.length, 1); assert.equal(e.historyEdits[0][2], '/');
  assert.equal(e.requests[0].path, '/api/pair'); assert.equal(e.requests[1].path, '/api/status');
  assert.equal(e.requests.some(r => r.path.includes(hash.slice(6))), false);
});
test('expired or consumed QR stops before private data requests', async () => {
  const e = await environment([], true, { hash: '#pair=0123456789ABCDEF0123456789ABCDEF', pairFails: true });
  assert.equal(e.requests.length, 1); assert.equal(e.requests[0].path, '/api/pair');
  assert.match(e.element('message').textContent, /caducado/);
});
test('malformed QR is removed from history and never sent', async () => {
  const e = await environment([], true, { hash: '#pair=not-a-token' });
  assert.equal(e.historyEdits.length, 1); assert.equal(e.requests.length, 0);
  assert.match(e.element('message').textContent, /no válido/);
});
