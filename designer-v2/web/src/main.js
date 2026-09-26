import './style.css';
import { KnobPreviewRenderer } from './renderer.js';
import { SIMPLE_OPTIONS } from '../../core/project-model.js';
import { detectFilmstripGeometry } from '../../core/filmstrip-detect.js';

const app = document.querySelector('#app');

const labels = {
  studio: 'Studio',
  flat: 'Flach',
  tapered: 'Konisch',
  cylindrical: 'Zylindrisch',
  stepped: 'Gestuft',
  domed: 'Gewölbt',
  compact: 'Kompakt',
  'plastic-black': 'Schwarzer Kunststoff',
  'metal-dark': 'Dunkles Metall',
  aluminium: 'Aluminium',
  steel: 'Stahl',
  brass: 'Messing',
  chrome: 'Chrom',
  gold: 'Gold',
  'soft-touch': 'Soft-Touch',
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
    supersample: 2,
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
        <h2>1b · Details</h2>
        <div class="row">
          <label>Kappe
            <select id="capEnabled">
              <option value="yes" selected>Mit Kappe</option>
              <option value="no">Ohne Kappe</option>
            </select>
          </label>
          <label>Griffstruktur
            <select id="sideDetail">
              <option value="smooth" selected>Glatt</option>
              <option value="grooved">Rillen</option>
              <option value="knurled">Gerändelt</option>
            </select>
          </label>
        </div>
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
        <div class="row">
          <label>Layout
            <select id="layout">
              <option value="vertical" selected>Vertikal (VSTGUI)</option>
              <option value="horizontal">Horizontal</option>
              <option value="grid">Sprite-Grid</option>
            </select>
          </label>
          <label>Qualität
            <select id="supersample">
              <option value="1">1× Schnell</option>
              <option value="2" selected>2× Hoch</option>
              <option value="3">3× Sehr hoch</option>
              <option value="4">4× Maximum</option>
            </select>
          </label>
        </div>
      </section>

      <div class="actions">
        <button id="exportPng" class="primary" type="button">Filmstrip PNG exportieren</button>
        <button id="exportScales" type="button">1× / 1.5× / 2× / 3× exportieren</button>
        <button id="centerQa" type="button">Center-/Wobble-QA</button>
        <button id="load" type="button">Projekt öffnen</button>
        <input id="loadFile" type="file" accept=".125agui,.json,.125agui.json,application/json" hidden>
        <button id="analyzeStrip" type="button">Filmstrip analysieren</button>
        <input id="stripFile" type="file" accept="image/png" hidden>
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
        <div class="row">
          <label>Frame <span id="frameOut">64 / 128</span>
            <input id="framePreview" type="range" min="0" max="127" value="63">
          </label>
          <label>Abspielen
            <button id="playPreview" type="button">▶ Vorschau</button>
          </label>
        </div>
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

function normalizeImportedProject(input) {
  if (!input || input.format !== '125A-GUI' || !input.design || !input.lighting || !input.output) {
    throw new Error('Keine gültige 125A-GUI-Projektdatei.');
  }

  const allowedShapes = new Set(SIMPLE_OPTIONS.shapes);
  const allowedMaterials = new Set(SIMPLE_OPTIONS.materials);
  const allowedLighting = new Set(SIMPLE_OPTIONS.lighting);
  const allowedIndicators = new Set(SIMPLE_OPTIONS.indicators);
  const allowedLayouts = new Set(['vertical', 'horizontal', 'grid']);

  return {
    format: '125A-GUI',
    version: Number.isInteger(input.version) ? input.version : 1,
    name: String(input.name || '125A Knob').slice(0, 120),
    asset: { type: 'knob' },
    design: {
      shape: allowedShapes.has(input.design.shape) ? input.design.shape : 'studio',
      material: allowedMaterials.has(input.design.material) ? input.design.material : 'metal-dark',
      baseColor: /^#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?$/.test(input.design.baseColor || '') ? input.design.baseColor : '#242A31FF',
      accentColor: /^#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?$/.test(input.design.accentColor || '') ? input.design.accentColor : '#657A8FFF',
      indicator: {
        enabled: input.design.indicator?.enabled !== false,
        type: allowedIndicators.has(input.design.indicator?.type) ? input.design.indicator.type : 'line',
        color: /^#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?$/.test(input.design.indicator?.color || '') ? input.design.indicator.color : '#E8EEF4FF',
        length: Math.max(15, Math.min(95, Number(input.design.indicator?.length ?? 66))),
        width: Math.max(0.5, Math.min(20, Number(input.design.indicator?.width ?? 3)))
      },
      expert: input.design.expert && typeof input.design.expert === 'object' ? input.design.expert : {}
    },
    lighting: {
      preset: allowedLighting.has(input.lighting.preset) ? input.lighting.preset : 'neutral',
      expert: input.lighting.expert && typeof input.lighting.expert === 'object' ? input.lighting.expert : {}
    },
    output: {
      frameWidth: Math.max(16, Math.min(1024, Number(input.output.frameWidth ?? 96))),
      frameHeight: Math.max(16, Math.min(1024, Number(input.output.frameHeight ?? input.output.frameWidth ?? 96))),
      frameCount: Math.max(2, Math.min(256, Number(input.output.frameCount ?? 128))),
      startAngle: Math.max(-720, Math.min(720, Number(input.output.startAngle ?? -135))),
      endAngle: Math.max(-720, Math.min(720, Number(input.output.endAngle ?? 135))),
      layout: allowedLayouts.has(input.output.layout) ? input.output.layout : 'vertical',
      supersample: Math.max(1, Math.min(4, Number(input.output.supersample ?? 2))),
      scaleExports: Array.isArray(input.output.scaleExports) ? input.output.scaleExports : [1, 1.5, 2, 3]
    }
  };
}

function setSelectValue(select, value, suffix='') {
  const text = String(value);
  if (![...select.options].some(option => option.value === text)) {
    const option = document.createElement('option');
    option.value = text;
    option.textContent = `${text}${suffix}`;
    option.dataset.imported = 'true';
    select.appendChild(option);
  }
  select.value = text;
}

function applyProjectToControls() {
  $('name').value = project.name;
  $('shape').value = project.design.shape;
  $('material').value = project.design.material;
  $('capEnabled').value = project.design.expert?.capEnabled === false ? 'no' : 'yes';
  $('sideDetail').value = ['smooth','grooved','knurled'].includes(project.design.expert?.sideDetail)
    ? project.design.expert.sideDetail : 'smooth';
  $('color').value = project.design.baseColor.slice(0, 7);
  $('lighting').value = project.lighting.preset;
  $('indicator').value = project.design.indicator.type;
  $('indicatorColor').value = project.design.indicator.color.slice(0, 7);
  $('length').value = String(project.design.indicator.length);
  const size = Math.round(project.output.frameWidth);
  setSelectValue($('size'), size, ' px');
  setSelectValue($('frames'), Math.round(project.output.frameCount));
  $('layout').value = project.output.layout;
  $('supersample').value = String(project.output.supersample);
  $('angle').min = String(Math.min(project.output.startAngle, project.output.endAngle));
  $('angle').max = String(Math.max(project.output.startAngle, project.output.endAngle));
  $('angle').value = String((project.output.startAngle + project.output.endAngle) * 0.5);
}

function syncAndRender() {
  project.name = $('name').value.trim() || '125A Knob';
  project.design.shape = $('shape').value;
  project.design.material = $('material').value;
  project.design.expert = project.design.expert || {};
  project.design.expert.capEnabled = $('capEnabled').value !== 'no';
  project.design.expert.sideDetail = $('sideDetail').value;
  project.design.baseColor = $('color').value.toUpperCase() + 'FF';
  project.design.indicator.type = $('indicator').value;
  project.design.indicator.color = $('indicatorColor').value.toUpperCase() + 'FF';
  project.design.indicator.length = Number($('length').value);
  project.lighting.preset = $('lighting').value;
  const size = Number($('size').value);
  project.output.frameWidth = size;
  project.output.frameHeight = size;
  project.output.frameCount = Number($('frames').value);
  project.output.layout = $('layout').value;
  project.output.supersample = Number($('supersample').value);
  $('lenOut').textContent = String(project.design.indicator.length);
  const frameMax = Math.max(1, project.output.frameCount - 1);
  $('framePreview').max = String(frameMax);
  if (Number($('framePreview').value) > frameMax) $('framePreview').value = String(frameMax);
  updateFramePreview(false);
  preview.update(project);
  preview.setPreviewAngle(Number($('angle').value));
  $('status').textContent = `${labels[project.design.shape]} · ${labels[project.design.material]} · ${labels[project.lighting.preset]}`;
}

for (const id of ['name','shape','capEnabled','sideDetail','material','color','lighting','indicator','indicatorColor','length','size','frames','layout','supersample']) {
  $(id).addEventListener('input', syncAndRender);
  $(id).addEventListener('change', syncAndRender);
}
let previewTimer = null;

function angleForFrame(index) {
  const count = Math.max(2, project.output.frameCount);
  const i = Math.max(0, Math.min(count - 1, Number(index) || 0));
  const t = i / (count - 1);
  return project.output.startAngle + (project.output.endAngle - project.output.startAngle) * t;
}

function updateFramePreview(render=true) {
  const frame = Math.max(0, Math.min(project.output.frameCount - 1, Number($('framePreview').value) || 0));
  $('frameOut').textContent = `${frame + 1} / ${project.output.frameCount}`;
  const angle = angleForFrame(frame);
  $('angle').value = String(angle);
  if (render) preview.setPreviewAngle(angle);
}

function stopPreviewPlayback() {
  if (previewTimer !== null) {
    clearInterval(previewTimer);
    previewTimer = null;
  }
  $('playPreview').textContent = '▶ Vorschau';
}

$('angle').addEventListener('input', () => {
  stopPreviewPlayback();
  preview.setPreviewAngle(Number($('angle').value));
});

$('framePreview').addEventListener('input', () => {
  stopPreviewPlayback();
  updateFramePreview(true);
});

$('playPreview').addEventListener('click', () => {
  if (previewTimer !== null) {
    stopPreviewPlayback();
    return;
  }
  $('playPreview').textContent = '■ Stop';
  previewTimer = setInterval(() => {
    const max = Math.max(1, project.output.frameCount - 1);
    let next = Number($('framePreview').value) + 1;
    if (next > max) next = 0;
    $('framePreview').value = String(next);
    updateFramePreview(true);
  }, 50);
});

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
    downloadBlob(blob, `${name}_${project.output.frameWidth}px_${project.output.frameCount}f_${project.output.layout}_${project.output.supersample}x.png`);
    $('status').textContent = `Export fertig · ${project.output.frameWidth}px · ${project.output.frameCount} Frames · ${project.output.supersample}×`;
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

$('exportScales').addEventListener('click', async () => {
  syncAndRender();
  stopPreviewPlayback();
  const button = $('exportScales');
  button.disabled = true;
  const originalWidth = project.output.frameWidth;
  const originalHeight = project.output.frameHeight;
  const originalSizeValue = $('size').value;
  const scales = Array.isArray(project.output.scaleExports) && project.output.scaleExports.length
    ? project.output.scaleExports : [1, 1.5, 2, 3];

  try {
    const name = safeFileName(project.name);
    for (let i = 0; i < scales.length; i++) {
      const scale = Number(scales[i]);
      if (!Number.isFinite(scale) || scale <= 0 || scale > 4) continue;
      const width = Math.max(16, Math.round(originalWidth * scale));
      const height = Math.max(16, Math.round(originalHeight * scale));
      project.output.frameWidth = width;
      project.output.frameHeight = height;
      $('status').textContent = `Mehrfach-Export ${i + 1} / ${scales.length} · ${scale}×`;
      const blob = await preview.exportFilmstrip(project, (done, total) => {
        $('status').textContent = `Mehrfach-Export ${scale}× · ${done} / ${total} Frames`;
      });
      downloadBlob(blob, `${name}_${width}px_${project.output.frameCount}f_${project.output.layout}_scale-${scale}x.png`);
      await new Promise(resolve => setTimeout(resolve, 150));
    }
    $('status').textContent = `Mehrfach-Export fertig · ${scales.length} Größen`;
  } catch (error) {
    console.error(error);
    $('status').textContent = error.message || 'Mehrfach-Export fehlgeschlagen';
    alert(error.message || 'Mehrfach-Export fehlgeschlagen.');
  } finally {
    project.output.frameWidth = originalWidth;
    project.output.frameHeight = originalHeight;
    setSelectValue($('size'), originalSizeValue, ' px');
    button.disabled = false;
    preview.update(project);
    updateFramePreview(true);
  }
});

$('centerQa').addEventListener('click', async () => {
  syncAndRender();
  const button = $('centerQa');
  button.disabled = true;
  try {
    $('status').textContent = 'Center-/Wobble-QA läuft …';
    const qa = await preview.measureCenterWobble(project, 9);
    const px = qa.maxDrift.toFixed(2);
    $('status').textContent = qa.pass
      ? `Center-QA PASS · Drift ${px} px`
      : `Center-QA CHECK · Drift ${px} px`;
  } catch (error) {
    console.error(error);
    $('status').textContent = error.message || 'Center-QA fehlgeschlagen';
  } finally {
    button.disabled = false;
    preview.setPreviewAngle(Number($('angle').value));
  }
});

$('load').addEventListener('click', () => {
  $('loadFile').value = '';
  $('loadFile').click();
});

$('loadFile').addEventListener('change', async () => {
  const file = $('loadFile').files?.[0];
  if (!file) return;
  try {
    const text = await file.text();
    const imported = normalizeImportedProject(JSON.parse(text));
    Object.keys(project).forEach(key => delete project[key]);
    Object.assign(project, imported);
    applyProjectToControls();
    syncAndRender();
    $('status').textContent = `Projekt geladen · ${project.name}`;
  } catch (error) {
    console.error(error);
    $('status').textContent = error.message || 'Projekt konnte nicht geladen werden';
    alert(error.message || 'Projekt konnte nicht geladen werden.');
  }
});

$('analyzeStrip').addEventListener('click', () => {
  $('stripFile').value = '';
  $('stripFile').click();
});

$('stripFile').addEventListener('change', async () => {
  const file = $('stripFile').files?.[0];
  if (!file) return;

  try {
    $('status').textContent = 'Filmstrip wird analysiert …';
    const bitmap = await createImageBitmap(file);
    const result = detectFilmstripGeometry(bitmap.width, bitmap.height);
    bitmap.close?.();

    if (!result.best) {
      throw new Error('Filmstrip-Geometrie konnte nicht sicher erkannt werden.');
    }

    const b = result.best;
    setSelectValue($('size'), Math.round(b.frameWidth), ' px');
    setSelectValue($('frames'), Math.round(b.frameCount));
    $('layout').value = b.layout;
    syncAndRender();

    const label = b.layout === 'vertical' ? 'vertikal'
      : b.layout === 'horizontal' ? 'horizontal'
      : 'Sprite-Grid';

    $('status').textContent =
      `Filmstrip erkannt · ${b.frameWidth}×${b.frameHeight}px · ${b.frameCount} Frames · ${label} · ${b.confidence}`;
  } catch (error) {
    console.error(error);
    $('status').textContent = error.message || 'Filmstrip-Analyse fehlgeschlagen';
    alert(error.message || 'Filmstrip-Analyse fehlgeschlagen.');
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
  $('capEnabled').value = 'yes';
  $('sideDetail').value = 'smooth';
  $('color').value = '#242a31';
  $('lighting').value = 'neutral';
  $('indicator').value = 'line';
  $('indicatorColor').value = '#e8eef4';
  $('length').value = '66';
  $('size').value = '96';
  $('frames').value = '128';
  $('layout').value = 'vertical';
  $('supersample').value = '2';
  $('angle').value = '0';
  $('framePreview').value = '63';
  stopPreviewPlayback();
  syncAndRender();
});

applyProjectToControls();
syncAndRender();
