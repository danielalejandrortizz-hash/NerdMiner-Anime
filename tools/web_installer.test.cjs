const { test } = require('node:test');
const assert = require('node:assert/strict');
const vm = require('node:vm');
const fs = require('node:fs');
const path = require('node:path');
const source = fs.readFileSync(path.join(__dirname, '../web/app.js'), 'utf8');

function setup(fetch) {
  const elements = {};
  for (const id of ['board', 'install-mode', 'installer', 'build-state', 'download', 'mode-help']) {
    elements['#' + id] = {
      value: id === 'install-mode' ? 'factory' : '', hidden: true, attrs: {},
      setAttribute(key, value) { this.attrs[key] = value; },
      removeAttribute(key) { delete this.attrs[key]; }, addEventListener() {},
    };
  }
  const context = vm.createContext({ document: { querySelector: id => elements[id], querySelectorAll: () => [] },
    fetch, URL, location: { href: 'https://example.com/NerdMiner-Anime/' } });
  vm.runInContext(source, context);
  return { elements, select: () => vm.runInContext('selectFirmware()', context) };
}
const manifest = { version: 'ki-test', builds: [{ chipFamily: 'ESP32', parts: [{ path: 'board.bin', offset: 0 }] }] };
const ok = () => ({ ok: true, headers: new Map([['content-length', '1024']]), json: async () => manifest });

test('missing firmware never exposes install or download', async () => {
  const app = setup(async () => ({ ok: false }));
  app.elements['#board'].value = 'ESP32-2432S028R-Room';
  await app.select();
  assert.equal(app.elements['#installer'].hidden, true);
  assert.equal(app.elements['#download'].hidden, true);
  assert.match(app.elements['#build-state'].textContent, /aún no está disponible/);
});
test('available update resolves binary relative to manifest under Pages subpath', async () => {
  const urls = [];
  const app = setup(async url => { urls.push(String(url)); return ok(); });
  app.elements['#board'].value = 'ESP32_2432S028_2USB-Room';
  app.elements['#install-mode'].value = 'update';
  await app.select();
  assert.equal(app.elements['#installer'].hidden, false);
  assert.equal(app.elements['#installer'].attrs.manifest, 'firmware/ESP32_2432S028_2USB-Room-update.json');
  assert.equal(urls[1], 'https://example.com/NerdMiner-Anime/firmware/board.bin');
  assert.match(app.elements['#mode-help'].textContent, /ya tienen KI Room/);
});
test('missing binary keeps installation disabled even with a manifest', async () => {
  let calls = 0;
  const app = setup(async () => ++calls === 1 ? ok() : { ok: false });
  app.elements['#board'].value = 'ESP32-2432S028R-Room';
  await app.select();
  assert.equal(app.elements['#installer'].hidden, true);
});
test('late response from previous selection cannot install wrong board', async () => {
  let resolveFirst;
  const app = setup(() => new Promise(resolve => { resolveFirst = resolve; }));
  app.elements['#board'].value = 'ESP32-2432S028R-Room';
  const first = app.select();
  app.elements['#board'].value = '';
  await app.select();
  // First request resumes, then its binary HEAD uses the same deferred fetch.
  resolveFirst(ok());
  await new Promise(resolve => setImmediate(resolve));
  resolveFirst(ok());
  await first;
  assert.equal(app.elements['#installer'].hidden, true);
  assert.match(app.elements['#build-state'].textContent, /Selecciona tu placa/);
});
