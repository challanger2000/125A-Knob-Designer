#!/usr/bin/env node
import fs from 'node:fs';
import path from 'node:path';
import { SIMPLE_OPTIONS, validateExportPlan, toRendererDesign } from '../core/project-model.js';
import { applyDesignPreset, listDesignPresets } from '../core/design-presets.js';

const argv = process.argv.slice(2);
const cmd = argv.shift();

function fail(msg, code = 1) {
  console.error('ERROR:', msg);
  process.exit(code);
}
function readJson(file) {
  try { return JSON.parse(fs.readFileSync(file, 'utf8')); }
  catch (e) { fail(`Cannot read JSON: ${file}: ${e.message}`); }
}
function writeJson(file, data) {
  fs.writeFileSync(file, JSON.stringify(data, null, 2) + '\n', 'utf8');
}
function isHex(v) { return typeof v === 'string' && /^#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?$/.test(v); }
function validate(p) {
  const errors = [];
  if (!p || p.format !== '125A-GUI') errors.push('format must be 125A-GUI');
  if (!Number.isInteger(p?.version) || p.version < 1) errors.push('version must be an integer >= 1');
  if (p?.asset?.type !== 'knob') errors.push('asset.type must currently be knob');

  const shapes = SIMPLE_OPTIONS.shapes;
  const materials = SIMPLE_OPTIONS.materials;
  const lights = SIMPLE_OPTIONS.lighting;
  const indicators = SIMPLE_OPTIONS.indicators;
  const layouts = ['vertical','horizontal','grid'];

  if (!shapes.includes(p?.design?.shape)) errors.push('unsupported design.shape');
  if (!materials.includes(p?.design?.material)) errors.push('unsupported design.material');
  if (!isHex(p?.design?.baseColor)) errors.push('design.baseColor must be #RRGGBB or #RRGGBBAA');
  if (!indicators.includes(p?.design?.indicator?.type)) errors.push('unsupported indicator.type');
  if (!isHex(p?.design?.indicator?.color)) errors.push('indicator.color must be #RRGGBB or #RRGGBBAA');
  if (!lights.includes(p?.lighting?.preset)) errors.push('unsupported lighting.preset');

  const o = p?.output ?? {};
  if (!Number.isInteger(o.frameWidth) || o.frameWidth < 16 || o.frameWidth > 1024) errors.push('output.frameWidth must be 16..1024');
  if (!Number.isInteger(o.frameHeight) || o.frameHeight < 16 || o.frameHeight > 1024) errors.push('output.frameHeight must be 16..1024');
  if (!Number.isInteger(o.frameCount) || o.frameCount < 2 || o.frameCount > 256) errors.push('output.frameCount must be 2..256');
  if (!layouts.includes(o.layout)) errors.push('unsupported output.layout');
  if (!Number.isFinite(o.startAngle) || !Number.isFinite(o.endAngle)) errors.push('output angles must be finite numbers');
  if (o.supersample != null && (!Number.isFinite(o.supersample) || o.supersample < 1 || o.supersample > 4)) {
    errors.push('output.supersample must be 1..4');
  }
  if (Number.isFinite(o.frameWidth) && Number.isFinite(o.frameHeight) && Number.isFinite(o.frameCount) && layouts.includes(o.layout)) {
    const plan = validateExportPlan(p);
    errors.push(...plan.errors.map(x => 'export: ' + x));
  }
  return errors;
}
function parseValue(raw) {
  if (raw === 'true') return true;
  if (raw === 'false') return false;
  if (raw === 'null') return null;
  if (/^-?\d+(\.\d+)?$/.test(raw)) return Number(raw);
  try { return JSON.parse(raw); } catch { return raw; }
}
function setPath(obj, dotted, value) {
  const parts = dotted.split('.').filter(Boolean);
  if (!parts.length) fail('Empty property path');
  let cur = obj;
  for (let i=0; i<parts.length-1; i++) {
    if (cur[parts[i]] == null || typeof cur[parts[i]] !== 'object') cur[parts[i]] = {};
    cur = cur[parts[i]];
  }
  cur[parts.at(-1)] = value;
}
function outputName(input, suffix) {
  const ext = path.extname(input);
  return input.slice(0, -ext.length) + suffix + ext;
}
if (!cmd || ['-h','--help','help'].includes(cmd)) {
  console.log(`125A GUI Designer CLI foundation
Usage:
  validate <project>
  set <project> <path> <value> [--in-place]
  preset <project> <preset-name> [--in-place]
  presets
  inspect <project>
`);
  process.exit(0);
}
if (cmd === 'presets') {
  for (const preset of listDesignPresets()) console.log(`${preset.id}\t${preset.name}`);
  process.exit(0);
}
if (cmd === 'validate') {
  const file = argv[0] ?? fail('Missing project file');
  const p = readJson(file);
  const errors = validate(p);
  if (errors.length) fail(errors.join('\n'));
  console.log('PASS:', file);
  process.exit(0);
}
if (cmd === 'set') {
  const [file, prop, raw] = argv;
  if (!file || !prop || raw == null) fail('Usage: set <project> <path> <value> [--in-place]');
  const p = readJson(file);
  setPath(p, prop, parseValue(raw));
  const errors = validate(p);
  if (errors.length) fail('Result would be invalid:\n' + errors.join('\n'));
  const out = argv.includes('--in-place') ? file : outputName(file, '.edited');
  writeJson(out, p);
  console.log('WROTE:', out);
  process.exit(0);
}
if (cmd === 'preset') {
  const [file, name] = argv;
  if (!file || !name) fail('Usage: preset <project> <preset-name> [--in-place]');
  const p = readJson(file);
  try { applyDesignPreset(p, name); }
  catch (e) { fail(e.message); }
  const errors = validate(p);
  if (errors.length) fail('Preset result invalid:\n' + errors.join('\n'));
  const out = argv.includes('--in-place') ? file : outputName(file, `.${name}`);
  writeJson(out, p);
  console.log('WROTE:', out);
  process.exit(0);
}
if (cmd === 'inspect') {
  const file = argv[0] ?? fail('Missing project file');
  const p = readJson(file);
  const errors = validate(p);
  if (errors.length) fail(errors.join('\n'));
  const plan = validateExportPlan(p);
  const design = toRendererDesign(p);
  console.log(JSON.stringify({
    file,
    name: p.name,
    shape: p.design.shape,
    material: p.design.material,
    layers: design.layers.map(x => ({ name:x.name, geometry:x.geometry, material:x.material.type })),
    indicator: design.indicator?.type ?? null,
    lighting: design.lighting,
    export: {
      layout: p.output.layout,
      frameWidth: p.output.frameWidth,
      frameHeight: p.output.frameHeight,
      frameCount: p.output.frameCount,
      supersample: plan.supersample,
      stripWidth: plan.dimensions.width,
      stripHeight: plan.dimensions.height,
      renderWidth: plan.renderWidth,
      renderHeight: plan.renderHeight
    }
  }, null, 2));
  process.exit(0);
}
fail(`Unknown command: ${cmd}`);
