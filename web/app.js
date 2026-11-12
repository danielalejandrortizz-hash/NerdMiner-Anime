'use strict';
const scenes = {
  goku: ['SON GOKU', 'Goku minando cristales junto a un lago al atardecer'],
  saiyan: ['SUPER SAIYAN', 'Super Saiyan en un paisaje de minería al atardecer'],
  pikachu: ['PIKACHU', 'Pikachu en un paisaje de minería al atardecer'],
  charmander: ['CHARMANDER', 'Charmander en un paisaje de minería al atardecer'],
};
document.querySelectorAll('[data-scene]').forEach((button, index) => {
  button.addEventListener('click', () => {
    const scene = button.dataset.scene;
    const art = document.querySelector('#hero-art');
    art.src = `assets/${scene}-sunset.png`;
    art.alt = scenes[scene][1];
    document.querySelector('#scene-name').textContent = scenes[scene][0];
    document.querySelector('.scene-label > span:last-child').textContent = `0${index + 1} / 04`;
    document.querySelectorAll('[data-scene]').forEach(item => {
      item.classList.toggle('selected', item === button);
      item.setAttribute('aria-pressed', String(item === button));
    });
  });
});
const board = document.querySelector('#board');
const mode = document.querySelector('#install-mode');
const installer = document.querySelector('#installer');
const state = document.querySelector('#build-state');
const download = document.querySelector('#download');
let requestId = 0;
async function selectFirmware() {
  const request = ++requestId;
  installer.hidden = true;
  installer.removeAttribute('manifest');
  download.hidden = true;
  download.removeAttribute('href');
  document.querySelector('#mode-help').textContent = mode.value === 'factory'
    ? 'Incluye arranque, particiones y firmware. Puede borrar la configuración anterior.'
    : 'Solo para placas que ya tienen KI Room y sus particiones. Desmarca borrar dispositivo si quieres conservar los ajustes.';
  if (!board.value) {
    state.textContent = 'Selecciona tu placa para comprobar el firmware.';
    return;
  }
  state.textContent = 'Comprobando firmware disponible…';
  try {
    const manifestUrl = `firmware/${board.value}-${mode.value}.json`;
    const response = await fetch(manifestUrl, { cache: 'no-store' });
    if (!response.ok) throw new Error('Firmware no disponible');
    const manifest = await response.json();
    const part = manifest.builds?.[0]?.parts?.[0];
    if (!part || manifest.builds[0].chipFamily !== 'ESP32') throw new Error('Manifest no válido');
    const binary = new URL(part.path, new URL(manifestUrl, location.href));
    const check = await fetch(binary, { method: 'HEAD', cache: 'no-store' });
    if (!check.ok || Number(check.headers.get('content-length')) === 0) throw new Error('Binario no disponible');
    if (request !== requestId) return;
    installer.setAttribute('manifest', manifestUrl);
    installer.hidden = false;
    download.href = binary.href;
    download.download = part.path;
    download.hidden = false;
    state.textContent = `Firmware compilado · ${manifest.version}. Pendiente de pruebas físicas en placa.`;
  } catch {
    if (request !== requestId) return;
    state.textContent = 'El firmware para esta placa aún no está disponible. Revisa el estado de la compilación en GitHub.';
  }
}
board.addEventListener('change', selectFirmware);
mode.addEventListener('change', selectFirmware);
