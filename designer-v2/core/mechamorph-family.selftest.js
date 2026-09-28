import fs from 'node:fs';
import path from 'node:path';
import { validateExportPlan, toRendererDesign } from './project-model.js';

function assert(ok,msg){ if(!ok) throw new Error(msg); }
const root=path.resolve('designer-v2/examples');
const cases=[
  ['mechamorph-main.125agui.json',170,96,'stepped'],
  ['mechamorph-scale.125agui.json',220,72,'stepped'],
  ['mechamorph-utility.125agui.json',104,128,'compact'],
  ['mechamorph-machine.125agui.json',250,6,'stepped']
];

for(const [file,size,frames,shape] of cases){
  const p=JSON.parse(fs.readFileSync(path.join(root,file),'utf8'));
  assert(p.output.frameWidth===size && p.output.frameHeight===size,`${file}: wrong cell size`);
  assert(p.output.frameCount===frames,`${file}: wrong frame count`);
  assert(p.output.startAngle===-135 && p.output.endAngle===135,`${file}: wrong rotation range`);
  assert(JSON.stringify(p.output.scaleExports)==='[1,1.25,1.5,2]',`${file}: wrong multires set`);
  const plan=validateExportPlan(p);
  assert(plan.ok,`${file}: unsafe base export plan: ${plan.errors.join(', ')}`);
  for(const scale of p.output.scaleExports){
    const q=structuredClone(p);
    q.output.frameWidth=Math.round(size*scale);
    q.output.frameHeight=Math.round(size*scale);
    const scaled=validateExportPlan(q);
    assert(scaled.ok,`${file}: unsafe ${scale}x plan: ${scaled.errors.join(', ')}`);
  }
  const d=toRendererDesign(p);
  assert(d.layers.length>=1,`${file}: no rendered layers`);
  assert(p.design.shape===shape,`${file}: wrong shape`);
  assert(d.layers[0].material.color==='#262A2CFF',`${file}: family base color mismatch`);
  assert(d.accentRing.enabled===true && d.accentRing.color==='#8A8170FF',`${file}: family accent mismatch`);
}
console.log('Mechamorph control family self-test: PASS');
