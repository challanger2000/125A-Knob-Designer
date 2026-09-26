import { applyDesignPreset, listDesignPresets } from './design-presets.js';

function assert(ok,msg){ if(!ok) throw new Error(msg); }

const presets=listDesignPresets();
assert(presets.length>=6,'expected at least six built-in design presets');
assert(presets.some(x=>x.id==='industrial-black'),'industrial preset missing');
assert(presets.some(x=>x.id==='brushed-aluminium'),'aluminium preset missing');

const project={
  design:{expert:{},indicator:{}},
  lighting:{expert:{}}
};
applyDesignPreset(project,'chrome-modern');
assert(project.design.shape==='cylindrical','chrome preset shape wrong');
assert(project.design.material==='chrome','chrome preset material wrong');
assert(project.design.expert.accentRing===true,'chrome preset accent ring wrong');
assert(project.design.expert.material.shininess===120,'chrome preset gloss wrong');
assert(project.lighting.expert.intensity===125,'chrome preset lighting wrong');

let threw=false;
try { applyDesignPreset(project,'does-not-exist'); } catch { threw=true; }
assert(threw,'unknown preset should throw');

console.log('125A built-in design presets self-test: PASS');
