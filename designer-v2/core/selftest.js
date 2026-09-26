import fs from 'node:fs';
import path from 'node:path';
import { calculateFilmstripDimensions, calculateFrameAngle, validateExportPlan, SIMPLE_OPTIONS, toRendererDesign } from './project-model.js';

const sample = JSON.parse(fs.readFileSync(path.resolve('designer-v2/examples/knob-dark-metal.125agui.json'),'utf8'));
const d = toRendererDesign(sample);

function assert(ok,msg){ if(!ok) throw new Error(msg); }

assert(d.layers.length===2,'studio shape should map to two layers');
assert(d.layers[0].material.type==='metallic','metal-dark should map to metallic');
assert(d.layers[0].material.color==='#242A31FF','project base color must override material preset');
assert(d.indicator?.type==='line','indicator mapping failed');
assert(d.accentRing?.enabled===true,'accent ring should default to enabled');
assert(d.accentRing?.color==='#657A8FFF','accent ring color mapping failed');
assert(d.lighting.azimuth===315 && d.lighting.elevation===58 && d.lighting.aoStrength===28,'expert lighting overrides must win');
assert(d.lighting.intensity===100,'default light intensity wrong');
assert(d.output.frameCount===128,'frame count mapping failed');
assert(d.output.sweepAngle===270,'sweep angle mapping failed');

const dim=calculateFilmstripDimensions(sample);
assert(dim.width===96 && dim.height===12288,'vertical filmstrip geometry wrong');
assert(calculateFrameAngle(sample,0)===-135,'first frame angle wrong');
assert(calculateFrameAngle(sample,127)===135,'last frame angle wrong');

const horizontal=structuredClone(sample);
horizontal.output.layout='horizontal';
let hdim=calculateFilmstripDimensions(horizontal);
assert(hdim.width===12288 && hdim.height===96,'horizontal filmstrip geometry wrong');

const grid=structuredClone(sample);
grid.output.layout='grid';
let gdim=calculateFilmstripDimensions(grid);
assert(gdim.columns===12 && gdim.rows===11,'grid topology wrong');
assert(gdim.width===1152 && gdim.height===1056,'grid filmstrip geometry wrong');

const plan=validateExportPlan(sample);
assert(plan.ok,'default export plan must be safe');
assert(plan.supersample===2,'default supersample should fall back to 2');

const unsafe=structuredClone(sample);
unsafe.output.frameHeight=192;
unsafe.output.frameWidth=192;
unsafe.output.frameCount=256;
unsafe.output.layout='vertical';
const unsafePlan=validateExportPlan(unsafe);
assert(!unsafePlan.ok,'oversized vertical filmstrip should fail safety validation');
const mid=calculateFrameAngle(sample,63.5);
assert(Math.abs(mid)<1e-9,'midpoint angle wrong');

assert(SIMPLE_OPTIONS.shapes.includes('stepped'),'simple shape list incomplete');
assert(SIMPLE_OPTIONS.shapes.includes('domed') && SIMPLE_OPTIONS.shapes.includes('compact'),'new shape presets missing');
assert(SIMPLE_OPTIONS.materials.includes('brass'),'simple material list incomplete');
assert(SIMPLE_OPTIONS.materials.includes('chrome') && SIMPLE_OPTIONS.materials.includes('soft-touch'),'new material presets missing');
assert(SIMPLE_OPTIONS.lighting.includes('dramatic'),'simple lighting list incomplete');

const variants=['plastic-black','metal-dark','aluminium','steel','brass','chrome','gold','soft-touch','rubber'];
for(const material of variants){
  const p=structuredClone(sample);
  p.design.material=material;
  const mapped=toRendererDesign(p);
  assert(mapped.layers.length>=1 && mapped.layers.length<=3,`bad layer count for ${material}`);
}

console.log('125A shared project -> renderer adapter self-test: PASS');

const detailed=structuredClone(sample);
detailed.design.expert={...(detailed.design.expert||{}),capEnabled:false,sideDetail:'knurled'};
const detailMapped=toRendererDesign(detailed);
assert(!detailMapped.layers.some(x=>x.name==='Cap'),'cap disable mapping failed');
assert(detailMapped.layers.some(x=>x.geometry.sideDetail==='knurled'),'side detail mapping failed');

const noAccent=structuredClone(sample);
noAccent.design.expert={...(noAccent.design.expert||{}),accentRing:false};
assert(toRendererDesign(noAccent).accentRing.enabled===false,'accent ring disable mapping failed');

const lit=structuredClone(sample);
lit.lighting.expert={...(lit.lighting.expert||{}),intensity:175};
lit.design.expert={...(lit.design.expert||{}),material:{shininess:111}};
const litMapped=toRendererDesign(lit);
assert(litMapped.lighting.intensity===175,'light intensity mapping failed');
assert(litMapped.layers[0].material.shininess===111,'gloss mapping failed');
