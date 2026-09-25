import fs from 'node:fs';
import path from 'node:path';
import { calculateFilmstripDimensions, calculateFrameAngle, SIMPLE_OPTIONS, toRendererDesign } from './project-model.js';

const sample = JSON.parse(fs.readFileSync(path.resolve('designer-v2/examples/knob-dark-metal.125agui.json'),'utf8'));
const d = toRendererDesign(sample);

function assert(ok,msg){ if(!ok) throw new Error(msg); }

assert(d.layers.length===2,'studio shape should map to two layers');
assert(d.layers[0].material.type==='metallic','metal-dark should map to metallic');
assert(d.layers[0].material.color==='#242A31FF','project base color must override material preset');
assert(d.indicator?.type==='line','indicator mapping failed');
assert(d.lighting.azimuth===315 && d.lighting.elevation===58 && d.lighting.aoStrength===28,'expert lighting overrides must win');
assert(d.output.frameCount===128,'frame count mapping failed');
assert(d.output.sweepAngle===270,'sweep angle mapping failed');

const dim=calculateFilmstripDimensions(sample);
assert(dim.width===96 && dim.height===12288,'vertical filmstrip geometry wrong');
assert(calculateFrameAngle(sample,0)===-135,'first frame angle wrong');
assert(calculateFrameAngle(sample,127)===135,'last frame angle wrong');
const mid=calculateFrameAngle(sample,63.5);
assert(Math.abs(mid)<1e-9,'midpoint angle wrong');

assert(SIMPLE_OPTIONS.shapes.includes('stepped'),'simple shape list incomplete');
assert(SIMPLE_OPTIONS.materials.includes('brass'),'simple material list incomplete');
assert(SIMPLE_OPTIONS.lighting.includes('dramatic'),'simple lighting list incomplete');

const variants=['plastic-black','metal-dark','aluminium','steel','brass','rubber'];
for(const material of variants){
  const p=structuredClone(sample);
  p.design.material=material;
  const mapped=toRendererDesign(p);
  assert(mapped.layers.length>=1 && mapped.layers.length<=3,`bad layer count for ${material}`);
}

console.log('125A shared project -> renderer adapter self-test: PASS');
