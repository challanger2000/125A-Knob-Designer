import './style.css';
import { KnobPreviewRenderer } from './renderer.js';
import { SIMPLE_OPTIONS } from '../../core/project-model.js';

const app = document.querySelector('#app');

const labels = {
  studio: 'Studio',
  flat: 'Flach',
  tapered: 'Konisch',
  cylindrical: 'Zylindrisch',
  stepped: 'Gestuft',
  'plastic-black': 'Schwarzer Kunststoff',
  'metal-dark': 'Dunkles Metall',
  aluminium: 'Aluminium',
  steel: 'Stahl',
  brass: 'Messing',
  rubber: 'Gummi',
  soft: 'Weich',
  neutral: 'Neutral',
  contrast: 'Kontrastreich',
  dramatic: 'Dramatisch',
  line: 'Linie',
  dot: 'Punkt',
  notch: 'Kerbe',
  groove: 'Nut'
};

const project = {
  format: '125A-GUI',
  version: 1,
  name: 'Mein 125A Knob',
  asset: { type: 'knob' },
  design: {
    shape: 'studio',
    material: 'metal-dark',
    baseColor: '#242A31FF',
    accentColor: '#657A8FFF',
    indicator: { enabled: true, type: 'line', color: '#E8EEF4FF', length: 66, width: 3 },
    expert: {}
  },
  lighting: { preset: 'neutral', expert: {} },
  output: {
    frameWidth: 96,
    frameHeight: 96,
    frameCount: 128,
    startAngle: -135,
    endAngle: 135,
    layout: 'vertical',
    scaleExports: [1,1.5,2,3]
  }
};

const options = arr => arr.map(v => `<option value="${v}">${labels[v] || v}</option>`).join('');

app.innerHTML = `
  <main class="shell">
    <aside class="panel">
      <h1 class="brand">125A GUI Designer</h1>
      <p class="sub">v0.2.0 · Einfach-Modus · Knob Builder</p>

      <label>Name
        <input id="name" type="text" value="${project.name}">
      </label>

      <section class="section">
        <h2>1 · Form</h2>
        <label>Grundform
          <select id="shape">${options(SIMPLE_OPTIONS.shapes)}</select>
        </label>
      </section>

      <section class="section">
        <h2>2 · Material</h2>
        <label>Material
          <select id="material">${options(SIMPLE_OPTIONS.materials)}</select>
        </label>
        <label>Grundfarbe
          <input id="color" type="color" value="#242a31">
        </label>
      </section>

      <section class="section">
        <h2>3 · Licht</h2>
        <label>Lichtstimmung
          <select id="lighting">${options(SIMPLE_OPTIONS.lighting)}</select>
        </label>
      </section>

      <section class="section">
        <h2>4 · Zeiger</h2>
        <label>Zeigerart
          <select id="indicator">${options(SIMPLE_OPTIONS.indicators)}</select>
        </label>
        <label>Zeigerfarbe
          <input id="indicatorColor" type="color" value="#e8eef4">
        </label>
        <label>Länge <span id="lenOut">66</span> %
          <input id="length" type="range" min="15" max="95" value="66">
        </label>
      </section>

      <section class="section">
        <h2>5 · Export</h2>
        <div class="row">
          <label>Größe
            <select id="size">
              <option value="64">64 px</option>
              <option value="96" selected>96 px</option>
              <option value="128">128 px</option>
              <option value="192">192 px</option>
            </select>
          </label>
          <label>Frames
            <select id="frames">
              <option value="64">64</option>
              <option value="128" selected>128</option>
              <option value="256">256</option>
            </select>
          </label>
        </div>
      </section>

      <div class="actions">
        <button id="exportPng" class="primary" type="button">Filmstrip PNG exportieren</button>
        <button id="save" type="button">Projekt speichern</button>
        <button id="reset" type="button">Zurücksetzen</button>
      </div>
    </aside>

    <section class="stage">
      <div class="stage-head">
        <strong>Live-Vorschau</strong>
        <span id="status" class="status">Renderer bereit</span>
      </div>
      <div class="preview-wrap">
        <canvas id="preview"></canvas>
      </div>
      <div>
        <label>Vorschau drehen
          <input id="angle" type="range" min="-135" max="135" value="0">
        </label>
        <p class="hint">Der Einfach-Modus zeigt absichtlich nur verständliche Entscheidungen. Technische Material-, Geometrie- und Lichtwerte bleiben intern und sind später im Expertenmodus erreichbar.</p>
      </div>
    </section>
  </main>
`;

const $ = id => document.getElementById(id);
$('shape').value = project.design.shape;
$('material').value = project.design.material;
$('lighting').value = project.lighting.preset;
$('indicator').value = project.design.indicator.type;

const preview = new KnobPreviewRenderer($('preview'));

function syncAndRender() {
  project.name = $('name').value.trim() || '125A Knob';
  project.design.shape = $('shape').value;
  project.design.material = $('material').value;
  project.design.baseColor = $('color').value.toUpperCase() + 'FF';
  project.design.indicator.type = $('indicator').value;
  project.design.indicator.color = $('indicatorColor').value.toUpperCase() + 'FF';
  project.design.indicator.length = Number($('length').value);
  project.lighting.preset = $('lighting').value;
  const size = Number($('size').value);
  project.output.frameWidth = size;
  project.output.frameHeight = size;
  project.output.frameCount = Number($('frames').value);
  $('lenOut').textContent = String(project.design.indicator.length);
  preview.update(project);
  preview.setPreviewAngle(Number($('angle').value));
  $('status').textContent = `${labels[project.design.shape]} · ${labels[project.design.material]} · ${labels[project.lighting.preset]}`;
}

for (const id of ['name','shape','material','color','lighting','indicator','indicatorColor','length','size','frames']) {
  $(id).addEventListener('input', syncAndRender);
  $(id).addEventListener('change', syncAndRender);
}
$('angle').addEventListener('input', () => preview.setPreviewAngle(Number($('angle').value)));

function safeFileName(value) {
  return (value || '125A-Knob')
    .replace(/[<>:"/\\|?*\x00-\x1F]+/g, '_')
    .replace(/[. ]+$/g, '')
    .trim() || '125A-Knob';
}

function downloadBlob(blob, fileName) {
  const a = document.createElement('a');
  a.href = URL.createObjectURL(blob);
  a.download = fileName;
  a.click();
  setTimeout(() => URL.revokeObjectURL(a.href), 1000);
}

$('exportPng').addEventListener('click', async () => {
  syncAndRender();
  const button = $('exportPng');
  button.disabled = true;
  const oldText = button.textContent;
  try {
    $('status').textContent = 'Filmstrip wird gerendert …';
    const blob = await preview.exportFilmstrip(project, (done, total) => {
      $('status').textContent = `Filmstrip: ${done} / ${total} Frames`;
    });
    const name = safeFileName(project.name);
    downloadBlob(blob, `${name}_${project.output.frameWidth}px_${project.output.frameCount}f_vertical.png`);
    $('status').textContent = `Export fertig · ${project.output.frameWidth}px · ${project.output.frameCount} Frames`;
  } catch (error) {
    console.error(error);
    $('status').textContent = error.message || 'Export fehlgeschlagen';
    alert(error.message || 'Filmstrip-Export fehlgeschlagen.');
  } finally {
    button.disabled = false;
    button.textContent = oldText;
    preview.setPreviewAngle(Number($('angle').value));
  }
});

$('save').addEventListener('click', () => {
  syncAndRender();
  const blob = new Blob([JSON.stringify(project, null, 2) + '\n'], { type: 'application/json' });
  downloadBlob(blob, safeFileName(project.name) + '.125agui.json');
});

$('reset').addEventListener('click', () => {
  $('shape').value = 'studio';
  $('material').value = 'metal-dark';
  $('color').value = '#242a31';
  $('lighting').value = 'neutral';
  $('indicator').value = 'line';
  $('indicatorColor').value = '#e8eef4';
  $('length').value = '66';
  $('size').value = '96';
  $('frames').value = '128';
  $('angle').value = '0';
  syncAndRender();
});

syncAndRender();
