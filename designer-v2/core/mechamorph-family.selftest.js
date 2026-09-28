import fs from 'node:fs';
import path from 'node:path';
import { validateExportPlan, toRendererDesign } from './project-model.js';

function assert(ok,msg){ if(!ok) throw new Error(msg); }
const root=path.resolve('designer-v2/examples');
const cases=[
  ['mechamorph-main.125agui.json',170,'stepped'],
  ['mechamorph-scale.125agui.json',220,'stepped'],
  ['mechamorph-utility.125agui.json',104,'compact'],
  ['mechamorph-machine.125agui.json',250,'stepped']
];

for(const [file,size,shape] of cases){
  const p=JSON.parse(fs.readFileSync(path.join(root,file),'utf8'));
  assert(p.output.frameWidth===size && p.output.frameHeight===size,`${file}: wrong cell size`);
  assert(p.output.frameCount===128,`${file}: expected 128 frames`);
  assert(p.output.startAngle===-135 && p.output.endAngle===135,`${file}: wrong rotation range`);
  const plan=validateExportPlan(p);
  assert(plan.ok,`${file}: unsafe export plan: ${plan.errors.join(', ')}`);
  assert(plan.dimensions.height===size*128,`${file}: filmstrip height mismatch`);
  const d=toRendererDesign(p);
  assert(d.layers.length>=1,`${file}: no rendered layers`);
  assert(p.design.shape===shape,`${file}: wrong shape`);
  assert(d.layers[0].material.color==='#262A2CFF',`${file}: family base color mismatch`);
  assert(d.accentRing.enabled===true && d.accentRing.color==='#8A8170FF',`${file}: family accent mismatch`);
}
console.log('Mechamorph control family self-test: PASS');
