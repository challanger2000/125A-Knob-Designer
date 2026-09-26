import { detectFilmstripGeometry } from './filmstrip-detect.js';

function assert(ok,msg){ if(!ok) throw new Error(msg); }

let r=detectFilmstripGeometry(96, 12288);
assert(r.best,'vertical strip not detected');
assert(r.best.layout==='vertical','vertical layout wrong');
assert(r.best.frameWidth===96 && r.best.frameHeight===96,'vertical frame size wrong');
assert(r.best.frameCount===128,'vertical frame count wrong');

r=detectFilmstripGeometry(8192, 64);
assert(r.best?.layout==='horizontal','horizontal layout wrong');
assert(r.best.frameWidth===64 && r.best.frameHeight===64,'horizontal frame size wrong');
assert(r.best.frameCount===128,'horizontal count wrong');

r=detectFilmstripGeometry(768, 512);
assert(r.candidates.some(x=>x.layout==='grid' && x.frameWidth===64 && x.columns===12 && x.rows===8 && x.frameCount===96),'grid candidate missing');

let threw=false;
try { detectFilmstripGeometry(0,100); } catch { threw=true; }
assert(threw,'invalid dimensions should throw');

console.log('125A filmstrip detector self-test: PASS');
